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

class VNode
{
  public:
    VNode *parent;
    std::vector<VNode *> childs;
    int id;
    VNode *ref;

    VNode()
    {
        ref = this;
    }

    VNode *GetRoot()
    {

        if (parent == nullptr)
        {
            return parent;
        }
        VNode *root = parent;
        while (root->parent != nullptr)
        {
            root = root->parent;
        }
        return root;
    }

    virtual void Do(){};
    virtual void Print(){};
    virtual void SetProperty(std::string name, std::string value) {}

    void GetMetaInfo()
    {
    }
};

#define STOP 0
#define START 1
#define CLOSE 2
#define HARD_CLOSE 3 //use market order?
class Spreader : public VNode
{
  public:
    DataStorage *storage;
    ScriptOrder *buy;
    ScriptOrder *sell;

    int32_t status = STOP;
    MktDataL1 *si;
    int32_t spread = 4;
    int32_t max_buy = 1;
    int32_t max_sell = 1;

    Spreader(DataStorage &storage, std::stringstream &out) : storage(&storage)
    {
        //this->storage = &storage;
        buy = new ScriptOrder(BUY, out);
        sell = new ScriptOrder(SELL, out);
        si = &sid_l1(1);
    }

    void Do() override
    {
        Strategy();
        Update();
    }

    void SetProperty(std::string name, std::string value) override
    {
        if (name == "spread")
        {
            try
            {
                int32_t _spread = std::stoi(value);
                spread = _spread;
            }
            catch (const std::exception &e)
            {
                std::cout << e.what();
            }
        }
        else if (name == "status")
        {
            try
            {
                int32_t _status = std::stoi(value);
                status = _status;
            }
            catch (const std::exception &e)
            {
                std::cout << e.what();
            }
        }
    }

    void Strategy()
    {
        int32_t position = buy->total_trades - sell->total_trades;

        if (status == STOP)
        {
            max_buy = 0;
            max_sell = 0;
        }
        else if (status == START)
        {
            max_buy = 1;
            max_sell = 1;
        }
        else if (status == CLOSE)
        {
            if (position > 0)
            {
                max_buy = 0;
            }
            else if (position < 0)
            {
                max_sell = 0;
            }
            else
            {
                max_buy = 0;
                max_sell = 0;
            }
        }

        if (si->is_ready && si->bid < si->ask)
        {
            //std::cout << "[node] " << si->bid << " " << si->ask << std::endl;

            if (position > 0)
            {
                buy->Update(0, 0);
                sell->Update(si->ask + spread * 1000000, max_buy);
            }
            else if (position < 0)
            {
                buy->Update(si->bid - spread * 1000000, max_sell);
                sell->Update(0, 0);
            }
            else
            {
                buy->Update(si->bid - spread * 1000000, max_buy);
                sell->Update(si->ask + spread * 1000000, max_sell);
            }
        }
        else
        {
            buy->Update(0, 0);
            sell->Update(0, 0);
        }
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
        profit /= 1000000;
        printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@ Profit = %ld, position = %d @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n", profit, position);
    }
};

struct RootNode : public VNode
{
    DataStorage *storage;
    std::stringstream *out;
    std::unordered_map<int32_t, VNode *> nodes;

    //Spreader *spreader;
    //Spreader *spreader2;

    RootNode(DataStorage &storage, std::stringstream &out) : storage(&storage), out(&out)
    {
        id = 1;
        nodes.insert(std::pair<int32_t, VNode *>(id, this));
        Initialization();
        Build();
    }

    void Initialization()
    {
        Spreader *spreader = new Spreader(*storage, *out);
        Spreader *spreader2 = new Spreader(*storage, *out);

        spreader->SetProperty("spread", "6");

        Mount(*spreader);
        Mount(*spreader2);
    }

    void Mount(VNode &node)
    {
        nodes.insert(std::pair<int32_t, VNode *>(++id, &node));
        childs.push_back(&node);
    }

    void Build()
    {
    }

    void Do()
    {
        for (auto &child : childs)
        {
            child->Do();
        }
    }

    void SetProperty(std::string name, std::string value) override
    {

        if (name == "status")
        {
            SetPropertyForAll(name, value);
        }
    }

    void SetPropertyForAll(std::string name, std::string value)
    {

        for (auto &kvp : nodes)
        {
            if (kvp.second->id > 1)
            {
                kvp.second->SetProperty(name, value);
            }
        }
    }

    void SetStrategyProperty(int id, std::string name, std::string value)
    {
        if (nodes.count(id))
        {
            nodes[id]->SetProperty(name, value);
        }
        else
        {
            //response fail
        }
    }

    void GetSchema()
    {
        std::string schema = R"(
        
        si-spreader:
            type: spreader
            symbol:
                Si-12.16
                    - OrderBook
                    - 1minut
            isin:
                - 148131
            params:
                MaxAmount:
                    initial: 20
                    type: i
                Limit: 10000
                User: semen
                Risk: Non

        )";
    }

    void Print()
    {
        for (auto &child : childs)
        {
            child->Print();
        }
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

    Spreader *spreader;
    RootNode *root;
    ScriptBroker()
    {
        //spreader = new Spreader(storage, orders);
        root = new RootNode(storage, orders);
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
        //spreader->Do();
        root->Do();
        ReadOrders();
        //spreader->Print();
        root->Print();

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