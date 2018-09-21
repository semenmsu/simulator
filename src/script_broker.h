#ifndef __SCRIPT_BROKER_H__
#define __SCRIPT_BROKER_H__

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
#include "script_order.h"

struct DataStorage
{
    std::unordered_map<int32_t, MktDataL1> l1;

    void UpdateL1(MktDataL1 &data)
    {
        std::cout << "try update" << std::endl;
        MktDataL1 *quote = &l1[data.isin_id];
        quote->bid = data.bid;
        quote->ask = data.ask;
        quote->is_ready = 1;
    }

    MktDataL1 &RegisterL1(MktDataL1 &data)
    {
        MktDataL1 &quote = l1[data.isin_id];
        quote.isin_id = data.isin_id;
        quote.bid = 0;
        quote.ask = 0;
        quote.is_ready = 0;
        return quote;
    }
};

struct Node
{
    DataStorage *storage;
    ScriptOrder *buy;
    ScriptOrder *sell;

    MktDataL1 *si;

    Node(DataStorage &storage, std::stringstream &out) : storage(&storage)
    {
        //this->storage = &storage;
        buy = new ScriptOrder(BUY, out);
        sell = new ScriptOrder(SELL, out);
        si = &sid_l1(1);
    }

    void Do()
    {
        if (si->is_ready)
        {
            std::cout << "[node] " << si->bid << " " << si->ask << std::endl;
            buy->Update(si->bid, 1);
            sell->Update(si->ask, 1);
        }
        else
        {
            buy->Update(0, 0);
            sell->Update(0, 0);
        }

        Update();
    }

    void Update()
    {
        buy->Do();
        sell->Do();
    }

    MktDataL1 &sid_l1(int id)
    {

        MktDataL1 l1;
        l1.isin_id = id;
        return storage->RegisterL1(l1);
        //MktDataL1 &sid_ref = storage->RegisterL1(l1);
        //return sid_ref;
    }

    void sid(int type, std::string str_id)
    {
    }

    void Print()
    {
        buy->Print();
        sell->Print();
        int32_t position = buy->total_trades - sell->total_trades;
        int64_t profit = buy->total_money + sell->total_money + position * (si->bid + si->ask) / 2;
        printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@ Profit = %ld, position = %d @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n", profit, position);
    }
};

struct ScriptBroker : BasePipe
{

    struct OrderSession
    {
        ScriptOrder *order;
        int64_t ext_id;
        int64_t external_ext_id;
    };

    DataStorage storage;

    std::stringstream in;
    std::stringstream out;
    std::stringstream orders;
    std::stringstream response;

    std::unordered_map<ScriptOrder *, OrderSession> order2session;
    std::unordered_map<int64_t, OrderSession *> extid2session;
    std::unordered_map<int64_t, OrderSession *> orderid2session;
    int64_t ext_id;
    int64_t ts;

    Node *node;
    ScriptBroker()
    {
        node = new Node(storage, orders);
    }

    int64_t GenereateExtId()
    {
        return ++ext_id;
    }

    void WriteMsgType(int msg_type)
    {
        out.write((char *)&msg_type, sizeof(msg_type));
    }

    //system
    int ReadMessageType(std::stringstream &stream)
    {
        if (stream.tellg() < stream.tellp())
        {
            int msg_type;
            stream.read((char *)&msg_type, sizeof(msg_type));
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
                std::cout << "READ NEW ORDER" << std::endl;
                ScriptOrder *pointer;
                orders.read((char *)&pointer, sizeof(pointer));

                int new_ext_id = GenereateExtId();
                NewOrder new_order;
                orders.read((char *)&new_order, sizeof(new_order));
                new_order.ext_id = new_ext_id;
                std::cout << "Start Create Session" << std::endl;
                OrderSession *session = &order2session[pointer];

                session->order = pointer;
                std::cout << "session" << std::endl;
                session->external_ext_id = new_ext_id;
                session->ext_id = new_order.ext_id;
                extid2session.insert(std::pair<int64_t, OrderSession *>(new_ext_id, session));

                /*extid2session.insert(std::pair<int64_t, OrderSession>(new_ext_id, {.order = pointer,
                                                                                   .ext_id = new_order.ext_id,
                                                                                   .external_ext_id = new_ext_id}));*/

                WriteMsgType(NEW_ORDER);
                out.write((char *)&new_order, sizeof(new_order));
            }
            else if (msg_type == CANCEL_ORDER)
            {

                ScriptOrder *pointer;
                orders.read((char *)&pointer, sizeof(pointer));

                //OrderSession *session = &order2session[pointer];

                CancelOrder cancel_order;
                orders.read((char *)&cancel_order, sizeof(cancel_order));
                WriteMsgType(CANCEL_ORDER);
                out.write((char *)&cancel_order, sizeof(cancel_order));
            }
        }
    }

    void ReadMarketDataL1()
    {
        MktDataL1 mkt_data_l1;
        in.read((char *)&mkt_data_l1, sizeof(mkt_data_l1));
        //std::cout << "[script-broker] bid/ask: " << mkt_data_l1.bid << " " << mkt_data_l1.ask << std::endl;
        storage.UpdateL1(mkt_data_l1);
    }

    void ReadInput()
    {
        int msg_type;
        while ((msg_type = ReadMessageType(in)) != EMPTY)
        {
            if (msg_type == NEW_REPLY_MSG)
            {
                NewReply new_reply;
                in.read((char *)&new_reply, sizeof(new_reply));
                OrderSession *session = extid2session[new_reply.ext_id];
                new_reply.ext_id = session->ext_id;
                orderid2session.insert(std::pair<int64_t, OrderSession *>((int64_t)new_reply.orderid, session));
                session->order->ReplyNew(new_reply);
            }
            else if (msg_type == CANCEL_REPLY_MSG)
            {
                CancelReply cancel_reply;
                in.read((char *)&cancel_reply, sizeof(cancel_reply));
                //OrderSession *session
                OrderSession *session = orderid2session[cancel_reply.orderid];
                session->order->ReplyCancel(cancel_reply);
            }
            else if (msg_type == TRADE_MSG)
            {
                Trade trade;
                in.read((char *)&trade, sizeof(trade));
                OrderSession *session = orderid2session[trade.orderid];
                session->order->ReplyTrade(trade);
            }
            else if (msg_type == TIMESTAMP_MSG)
            {
                in.read((char *)&ts, sizeof(ts));
            }
            else if (msg_type == MKT_DATA_L1)
            {
                ReadMarketDataL1();
            }
        }

        //in->seekg(0, std::ios::beg);
        //in->seekp(0, std::ios::beg);
    }

    void UpdateRoot()
    {
    }

    void Do()
    {
        ReadInput();
        node->Do();
        ReadOrders();
        node->Print();
        //UpdateRoot();
    }

    //handle script !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    BasePipe &operator|(BasePipe &to)
    {

        /*Generate();
        WriteMarketDataL1();
        if (out.tellp() > out.tellg())
        {
            to.In() << out.rdbuf(); //copy
        }*/
        //Do();
        return to;
    }

    BasePipe &operator|(std::stringstream &stream)
    {
        Do();
        std::cout << "[script-broket] data from storage " << storage.l1[1].bid << " " << storage.l1[1].ask << " " << &storage << std::endl;
        if (out.tellp() > out.tellg())
        {
            stream << out.rdbuf();
        }
        return *this;
    }

    std::stringstream &In() override
    {
        return in;
    };
};

#endif