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
    int64_t total_money = 0;
    int64_t total_trades = 0;
    int64_t volume = 0;

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

    ScriptOrder(int32_t dir, std::stringstream &out)
    {
        ref = this;
        Free();
        session_id = 0;
        state.dir = dir;
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

    void HandleFreeState()
    {
        if (desired_amount < 0)
        {
            throw "desired_amount < 0";
        }

        //check some limits

        if (desired_amount == 0)
        {
            return;
        }
        WriteNewOrder();
    }

    void HandleNewState()
    {
        if (desired_price != state.price)
        {
            return WriteCancelOrder();
        }
        if (desired_amount == 0)
        {
            return WriteCancelOrder();
        }
    }

    void Do()
    {
        switch (state.status)
        {
        case FREE:
            HandleFreeState();
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

        if (state.dir == 1)
        {
            total_money -= trade.amount * trade.deal_price;
        }
        else
        {
            total_money += trade.amount * trade.deal_price;
        }

        total_trades += trade.amount;

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

    void Print()
    {
        printf("[script-order] %s, dir = %d, price = %ld, amount = %d, remaingingAmount = %d \n",
               GetStringOrderStatus(state.status).c_str(), state.dir, state.price, state.amount, state.remainingAmount);
    }
};

#endif // !__SCRIPT_ORDER_H__
