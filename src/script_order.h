#ifndef __SCRIPT_ORDER_H__
#define __SCRIPT_ORDER_H__

#include <stdio.h>
#include <set>
#include <unordered_map>
#include <iostream>
#include <assert.h>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>
#include <sstream>
#include "iproto.h"

struct ScriptOrder
{
    struct State
    {
        int status;
        int64_t ext_id;
        int64_t orderid;
        int64_t price;
        int32_t amount;
        int32_t remainingAmount;
        int32_t dir = 1;
    } state;

    int64_t desired_price = 0;
    int32_t desired_amount = 0;
    int64_t session_id = 0;

    std::stringstream *out;
    ScriptOrder *ref;

    ScriptOrder()
    {
        ref = this;
        Free();
        session_id = 0;
    }

    ScriptOrder(std::stringstream &out)
    {
        ref = this;
        Free();
        session_id = 0;
        this->out = &out;
    }

    void WriteMsgType(int msg_type)
    {
        out->write((char *)&msg_type, sizeof(msg_type));
    }

    void WriteOrderPointer()
    {
        out->write((char *)&ref, sizeof(ref));
    }

    void Write(NewOrder &new_order)
    {
        WriteMsgType(NEW_ORDER);
        WriteOrderPointer();
        out->write((char *)&new_order, sizeof(new_order));
    }

    void Write(CancelOrder &cancel_order)
    {
        WriteMsgType(CANCEL_ORDER);
        WriteOrderPointer();
        out->write((char *)&cancel_order, sizeof(cancel_order));
    }

    void InitNewOrder(NewOrder &new_order)
    {
        new_order.user_code = 1;
        new_order.isin_id = 1;
        new_order.ext_id = 0;
        new_order.price = desired_price;
        new_order.amount = desired_amount;
        new_order.dir = state.dir;
    }

    void InitCancelOrder(CancelOrder &cancel_order)
    {
        cancel_order.user_code = 1;
        cancel_order.isin_id = 1;
        cancel_order.orderid = state.orderid;
    }

    void WriteNewOrder()
    {
        assert(state.status == FREE);

        NewOrder new_order;
        InitNewOrder(new_order);

        state.status = PENDING_NEW;
        state.price = desired_price;
        state.amount = desired_amount;
        state.remainingAmount = desired_amount;

        Write(new_order);

        std::cout << "WRITE NEW ORDER \n";
    }

    void WriteCancelOrder()
    {
        assert(state.status == NEW);

        CancelOrder cancel_order;
        InitCancelOrder(cancel_order);
        Write(cancel_order);

        std::cout << FgGreen << "[script] CANCEL_ORDER " << state.orderid << Reset << std::endl;
        state.status = PENDING_CANCEL;
    }

    void HandleNewState()
    {
        if (desired_price != state.price)
        {
            WriteCancelOrder();
        }
    }

    void Do()
    {
        switch (state.status)
        {
        case FREE:
            WriteNewOrder();
            break;
        case PENDING_NEW:
            break;
        case NEW:
            HandleNewState();
            break;
        case PENDING_CANCEL:
            break;
        case CANCELED:
            break;
        }
    }

    void operator()(int64_t price, int32_t amount)
    {
        desired_price = price;
        desired_amount = amount;
    }

    void Update(int64_t price, int32_t amount)
    {
        desired_price = price;
        desired_amount = amount;
    }

    void ReplyNew(NewReply &new_reply)
    {
        assert(state.status == PENDING_NEW);

        if (new_reply.code != 0)
        {
            throw "new_reply.code != 0";
        }

        assert(new_reply.orderid != 0);
        state.orderid = new_reply.orderid;
        state.status = NEW;
    }

    void ReplyCancel(CancelReply &cancel_reply)
    {
        std::cout << "[REPLY_CANCEL] amount " << cancel_reply.amount << std::endl;
        assert(state.status == PENDING_CANCEL || state.status == NEW);
        if (cancel_reply.code == 0)
        {
            state.remainingAmount -= cancel_reply.amount;
            assert(state.remainingAmount >= 0);
            if (state.remainingAmount == 0)
            {
                Free();
            }
            else
            {
                state.status = CANCELED;
            }
        }
        else
        {
            if (state.remainingAmount > 0)
            {
                state.status = CANCELED; //wait trades
            }
        }
    }

    void ReplyTrade(Trade &trade)
    {
        assert(state.status == PENDING_CANCEL || state.status == NEW || state.status == CANCELED);

        state.remainingAmount -= trade.amount;

        assert(state.remainingAmount >= 0);

        if (state.remainingAmount == 0)
        {
            if (state.status == NEW)
            {
                Free();
            }
            else if (state.status == CANCELED)
            {
                Free();
            }
        }
    }

    void ProcessStringInput(std::string line)
    {

        if (line[0] == 'u')
        {
            line.erase(0, 1);
            std::istringstream linestream(line);
            int64_t _desired_price;
            int32_t _desired_amount;
            linestream >> _desired_price;
            linestream >> _desired_amount;
            Update(_desired_price, _desired_amount);
        }
        else if (line[0] == 'n')
        {
            line.erase(0, 1);
            NewReply new_reply;
            new_reply << line;
            ReplyNew(new_reply);
        }
        else if (line[0] == 'c')
        {

            line.erase(0, 1);
            CancelReply cancel_reply;
            cancel_reply << line;
            ReplyCancel(cancel_reply);
        }
        else if (line[0] == 't')
        {
            line.erase(0, 1);
            Trade trade;
            trade << line;
            ReplyTrade(trade);
        }
    }

    ScriptOrder &operator<<(std::string input)
    {

        std::istringstream f(input);
        std::string line;
        while (std::getline(f, line))
        {
            line.erase(line.find_last_not_of(" \n\r\t") + 1);
            if (line.size() > 0)
            {
                ProcessStringInput(line);
            }
        }
        return *this;
    }

    void Free()
    {
        state.status = FREE;
        state.orderid = 0;
        state.remainingAmount = 0;
    }
};

struct ScriptBroker
{

    struct OrderSession
    {
        ScriptOrder *order;
        int64_t ext_id;
        int64_t external_ext_id;
    };

    std::stringstream *in;
    std::stringstream *out;
    std::stringstream orders;
    std::stringstream response;

    std::unordered_map<ScriptOrder *, OrderSession> order2session;
    std::unordered_map<int64_t, OrderSession *> extid2session;
    std::unordered_map<int64_t, OrderSession *> orderid2session;
    int64_t ext_id;
    int64_t ts;

    int64_t GenereateExtId()
    {
        return ++ext_id;
    }

    void WriteMsgType(int msg_type)
    {
        out->write((char *)&msg_type, sizeof(msg_type));
    }

    //system
    int ReadMessageType(std::stringstream &stream)
    {
        if (stream.tellg() < stream.tellp())
        {
            int msg_type;
            in->read((char *)&msg_type, sizeof(msg_type));
            return msg_type;
        }
        else
        {
            return EMPTY;
        }
    }

    void ReadOrders()
    {
        int msg_type;
        while ((msg_type = ReadMessageType(orders)) != EMPTY)
        {
            if (msg_type == NEW_ORDER)
            {
                ScriptOrder *pointer;
                out->read((char *)&pointer, sizeof(pointer));

                int new_ext_id = GenereateExtId();
                NewOrder new_order;
                out->read((char *)&new_order, sizeof(new_order));
                new_order.ext_id = new_ext_id;

                OrderSession *session = &order2session[pointer];
                session->order = pointer;
                session->external_ext_id = new_ext_id;
                session->ext_id = new_order.ext_id;
                extid2session.insert(std::pair<int64_t, OrderSession *>(new_ext_id, session));

                /*extid2session.insert(std::pair<int64_t, OrderSession>(new_ext_id, {.order = pointer,
                                                                                   .ext_id = new_order.ext_id,
                                                                                   .external_ext_id = new_ext_id}));*/

                WriteMsgType(NEW_ORDER);
                out->write((char *)&new_order, sizeof(new_order));
            }
            else if (msg_type == CANCEL_ORDER)
            {

                ScriptOrder *pointer;
                out->read((char *)&pointer, sizeof(pointer));

                //OrderSession *session = &order2session[pointer];

                CancelOrder cancel_order;
                out->read((char *)&cancel_order, sizeof(cancel_order));
                WriteMsgType(CANCEL_ORDER);
                out->write((char *)&cancel_order, sizeof(cancel_order));
            }
        }
    }

    void ReadMarketDataL1()
    {
        MktDataL1 mkt_data_l1;
        in->read((char *)&mkt_data_l1, sizeof(mkt_data_l1));
    }

    void ReadInput()
    {
        int msg_type;
        while ((msg_type = ReadMessageType(orders)) != EMPTY)
        {
            /*           #define TIMESTAMP_MSG 1
#define NEW_REPLY_MSG 2
#define CANCEL_REPLY_MSG 3
#define TRADE_MSG 4*/
            if (msg_type == NEW_REPLY_MSG)
            {
                NewReply new_reply;
                in->read((char *)&new_reply, sizeof(new_reply));
                OrderSession *session = extid2session[new_reply.ext_id];
                new_reply.ext_id = session->ext_id;
                orderid2session.insert(std::pair<int64_t, OrderSession *>((int64_t)new_reply.orderid, session));
                session->order->ReplyNew(new_reply);
            }
            else if (msg_type == CANCEL_REPLY_MSG)
            {
                CancelReply cancel_reply;
                in->read((char *)&cancel_reply, sizeof(cancel_reply));
                //OrderSession *session
                OrderSession *session = orderid2session[cancel_reply.orderid];
                session->order->ReplyCancel(cancel_reply);
            }
            else if (msg_type == TRADE_MSG)
            {
                Trade trade;
                in->read((char *)&trade, sizeof(trade));
                OrderSession *session = orderid2session[trade.orderid];
                session->order->ReplyTrade(trade);
            }
            else if (msg_type == TIMESTAMP_MSG)
            {
                in->read((char *)&ts, sizeof(ts));
            }
            else if (msg_type == MKT_DATA_L1)
            {
                ReadMarketDataL1();
            }
        }
    }

    void Do()
    {
        ReadInput();
        ReadOrders();
    }
};

#endif // !__SCRIPT_ORDER_H__
