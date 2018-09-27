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
#include <fstream>
#include <sstream>
#include "iproto.h"
#include "script_order.h"

struct DataStorage
{
    std::unordered_map<std::string, MktDataL1> symbol2BestBidAsk;
    std::unordered_map<int32_t, MktDataL1 *> l1;

    void UpdateL1(MktDataL1 &data)
    {
        /*
        std::cout << "try update" << std::endl;
        MktDataL1 *quote = &l1[data.isin_id];
        quote->bid = data.bid;
        quote->ask = data.ask;
        quote->is_ready = 1;*/
        //std::cout << "Start Update L1" << std::endl;
        MktDataL1 *quote = l1[data.isin_id];
        quote->bid = data.bid;
        quote->ask = data.ask;
        quote->is_ready = 1;
        //std::cout << "Update L1 " << data.isin_id << " " << data.bid << " " << data.ask << std::endl;
        //getchar();
    }

    MktDataL1 &RegisterL1(MktDataL1 &data)
    {
        /*MktDataL1 &quote = l1[data.isin_id];
        quote.isin_id = data.isin_id;
        quote.bid = 0;
        quote.ask = 0;
        quote.is_ready = 0;
        return quote;*/
        //MktDataL1 l1;
        //return l1;
        return data;
    }

    MktDataL1 &RegisterL1(std::string symbol)
    {
        if (symbol2BestBidAsk.count(symbol) == 0)
        {
            MktDataL1 l1;
            symbol2BestBidAsk[symbol] = l1;
            return symbol2BestBidAsk[symbol];
            //symbol2BestBidAsk.insert(std::pair<std::string, MktDataL1>(symbol))
        }
        else
        {
            return symbol2BestBidAsk[symbol];
        }
    }

    //write to out all request for marketdata
    //ext_id = 0, this is for data storage
    void RequestInstrumentInfo(std::stringstream &out)
    {
        for (auto &kvp : symbol2BestBidAsk)
        {
            InstrumentInfoRequest info_request;
            std::string symbol = kvp.first;
            symbol.copy(info_request.symbol, symbol.length());
            info_request.symbol[symbol.length()] = '\0';
            info_request.ext_id = 0;
            int msg_type = INSTRUMENT_INFO_REQUEST;
            out.write((char *)&msg_type, sizeof(msg_type));
            out.write((char *)&info_request, sizeof(info_request));
        }
    }

    void ReplyInstrumentInfo(InstrumentInfoReply info_reply)
    {
        std::string symbol(info_reply.symbol);
        //std::cout << "[data-storeage] ReplyInstrumentInfo " << symbol << std::endl;
        if (symbol2BestBidAsk.count(symbol) == 0)
        {
            MktDataL1 bidask;
            bidask.isin_id = info_reply.isin_id;

            symbol2BestBidAsk[symbol] = bidask;
            bidask.min_step_price = info_reply.min_step_price;

            if (l1.count(info_reply.isin_id) == 0)
            {
                l1[info_reply.isin_id] = &symbol2BestBidAsk[symbol];
            }
        }
        else
        {
            symbol2BestBidAsk[symbol].isin_id = info_reply.isin_id;
            symbol2BestBidAsk[symbol].min_step_price = info_reply.min_step_price;
            if (l1.count(info_reply.isin_id) == 0)
            {
                l1[info_reply.isin_id] = &symbol2BestBidAsk[symbol];
            }
        }
        //getchar();
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
    virtual void RequestSettings(){};

    void GetMetaInfo()
    {
    }
};

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

    int64_t cycle_count = 0;
    std::string symbol;

    Spreader(DataStorage &storage, std::stringstream &out) : storage(&storage)
    {
        //this->storage = &storage;
        buy = new ScriptOrder("Si-12.16", BUY, out);
        sell = new ScriptOrder("Si-12.16", SELL, out);
        //si = &sid_l1(1);
        si = &sid("Si-12.16");
    }

    Spreader(std::string symbol, DataStorage &storage, std::stringstream &out) : storage(&storage)
    {
        //this->storage = &storage;
        buy = new ScriptOrder(symbol, BUY, out);
        sell = new ScriptOrder(symbol, SELL, out);
        //si = &sid_l1(1);
        this->symbol = symbol;
        si = &sid(symbol);
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
        cycle_count++;

        if (cycle_count > 20)
        {
            status = START;
        }

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

        /*if (si->is_ready)
        {
            std::cout << "[spreader] si.bid / si.ask = " << si->bid << " " << si->ask << "  status = " << status << std::endl;
            //getchar();
        }*/

        if (si->is_ready && si->bid < si->ask)
        {
            //std::cout << "[node] " << si->bid << " " << si->ask << std::endl;
            //std::cout << "MinStep " << si->min_step_price << std::endl;

            if (position > 0)
            {
                buy->Update(0, 0);
                sell->Update(si->ask + spread * si->min_step_price, max_buy);
            }
            else if (position < 0)
            {
                buy->Update(si->bid - spread * si->min_step_price, max_sell);
                sell->Update(0, 0);
            }
            else
            {
                buy->Update(si->bid - spread * si->min_step_price, max_buy);
                sell->Update(si->ask + spread * si->min_step_price, max_sell);
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

    MktDataL1 &sid(std::string symbol)
    {
        return storage->RegisterL1(symbol);
    }

    void sid(int type, std::string str_id)
    {
    }

    void RequestSettings()
    {
        buy->RequestInstrumentInfo();
        sell->RequestInstrumentInfo();
    }

    void Print()
    {
        buy->Print();
        sell->Print();
        int32_t position = buy->total_trades - sell->total_trades;
        int64_t profit = buy->total_money + sell->total_money + position * (si->bid + si->ask) / 2;
        int64_t total_trades = buy->total_trades + sell->total_trades;
        profit /= si->min_step_price;
        printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@ Profit = %ld, position = %d, total_trades = %ld @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n",
               profit, position, total_trades);
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
        Spreader *spreader = new Spreader("USD000UTSTOM", *storage, *out);
        //Spreader *spreader2 = new Spreader("RTS-12.16", *storage, *out);
        //SpreaderSber *spreader2 = new SpreaderSber(*storage, *out);
        //Spreader *spreader2 = new Spreader("SBRF-12.16", *storage, *out);
        //Spreader *spreader3 = new Spreader("RTS-12.16", *storage, *out);

        spreader->SetProperty("spread", "4");
        //spreader2->SetProperty("spread", "10");
        //spreader2->SetProperty("spread", "15");
        //spreader3->SetProperty("spread", "100");

        Mount(*spreader);
        //Mount(*spreader2);
        // Mount(*spreader2);
        // Mount(*spreader3);
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

    void RequestSettings()
    {
        for (auto &child : childs)
        {
            child->RequestSettings();
        }
    }

    void Print()
    {
        for (auto &child : childs)
        {
            child->Print();
        }
    }
};

struct VirtSocket
{
};

struct ScriptBroker : public BasePipe
{

    struct OrderSession
    {
        ScriptOrder *order;
        int64_t ext_id;
        int64_t external_ext_id;
        int64_t ts_new;
        int64_t ts_new_reply;
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
    std::ofstream log;

    ScriptBroker()
    {
        //spreader = new Spreader(storage, orders);
        root = new RootNode(storage, orders);
        root->RequestSettings();
        ext_id = 1000;

        log.open("result/robo.csv");

        //
        storage.RequestInstrumentInfo(out);
    }

    void LogToFile(std::string line)
    {
        if (log.is_open())
        {
            log << line;
        }
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

    int64_t GetOrderDelay()
    {
        return 120 * 10;
    }

    void ReadOrders()
    {
        int msg_type;
        while ((msg_type = ReadMessageType(orders)) != EMPTY)
        {
            if (msg_type == NEW_ORDER)
            {
                //std::cout << "READ NEW ORDER" << std::endl;
                ScriptOrder *pointer;
                orders.read((char *)&pointer, sizeof(pointer));

                int new_ext_id = GenereateExtId();
                NewOrder new_order;
                orders.read((char *)&new_order, sizeof(new_order));
                new_order.ext_id = new_ext_id;
                new_order.ts = ts + GetOrderDelay();
                //std::cout << "Start Create Session" << std::endl;
                OrderSession *session = &order2session[pointer];

                session->order = pointer;
                //std::cout << "session" << std::endl;
                session->external_ext_id = new_ext_id;
                session->ext_id = new_order.ext_id;
                session->ts_new = ts;
                extid2session.insert(std::pair<int64_t, OrderSession *>(new_ext_id, session));

                /*extid2session.insert(std::pair<int64_t, OrderSession>(new_ext_id, {.order = pointer,
                                                                                   .ext_id = new_order.ext_id,
                                                                                   .external_ext_id = new_ext_id}));*/
                //LogToFile(new_order.to_csv());
                //std::cout << new_order << std::endl;
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
                cancel_order.ts = ts + GetOrderDelay();

                //std::cout << cancel_order << std::endl;
                WriteMsgType(CANCEL_ORDER);
                out.write((char *)&cancel_order, sizeof(cancel_order));
            }
            else if (msg_type == INSTRUMENT_INFO_REQUEST)
            {
                //std::cout << "READ REQUEST INFO" << std::endl;
                ScriptOrder *pointer;
                orders.read((char *)&pointer, sizeof(pointer));

                int new_ext_id = GenereateExtId();

                InstrumentInfoRequest info_request;
                orders.read((char *)&info_request, sizeof(info_request));
                info_request.ext_id = new_ext_id;

                OrderSession *session = &order2session[pointer];
                session->order = pointer;
                extid2session.insert(std::pair<int64_t, OrderSession *>(new_ext_id, session));

                WriteMsgType(INSTRUMENT_INFO_REQUEST);
                out.write((char *)&info_request, sizeof(info_request));
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
            //std::cout << "put " << in.tellp() << "  get = " << in.tellg() << std::endl;
            //std::cout << "[broker]!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!ReadInput msg_type " << msg_type << std::endl;
            if (msg_type == NEW_REPLY_MSG)
            {
                NewReply new_reply;
                in.read((char *)&new_reply, sizeof(new_reply));
                OrderSession *session = extid2session[new_reply.ext_id];
                new_reply.ext_id = session->ext_id;
                session->ts_new_reply = new_reply.ts;
                //std::cout << "ts new       " << session->ts_new << std::endl;
                //std::cout << "ts new reply " << session->ts_new_reply << std::endl;
                //std::cout << "diff         " << (session->ts_new_reply - session->ts_new) / 10 << std::endl;
                //getchar();
                orderid2session.insert(std::pair<int64_t, OrderSession *>((int64_t)new_reply.orderid, session));
                //std::cout << new_reply << std::endl;
                session->order->ReplyNew(new_reply);
            }
            else if (msg_type == CANCEL_REPLY_MSG)
            {
                CancelReply cancel_reply;
                in.read((char *)&cancel_reply, sizeof(cancel_reply));
                //OrderSession *session
                OrderSession *session = orderid2session[cancel_reply.orderid];
                //std::cout << cancel_reply << std::endl;
                session->order->ReplyCancel(cancel_reply);
            }
            else if (msg_type == TRADE_MSG)
            {
                Trade trade;
                in.read((char *)&trade, sizeof(trade));
                OrderSession *session = orderid2session[trade.orderid];
                LogToFile(trade.to_csv());
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
            else if (msg_type == INSTRUMENT_INFO_REPLY)
            {
                //std::cout << "[broker] INSTRUMENT_INFO_REPLY\n";
                InstrumentInfoReply info_reply;
                in.read((char *)&info_reply, sizeof(info_reply));

                if (info_reply.ext_id != 0)
                {
                    OrderSession *session = extid2session[info_reply.ext_id];
                    session->order->ReplyInstrumentInfo(info_reply);
                    extid2session.erase(info_reply.ext_id);
                }
                else
                {
                    //idea about reserved ext_id
                    //global info (usually for storage)

                    storage.ReplyInstrumentInfo(info_reply);
                }
            }
            else
            {
                std::cout << "unknow msg_type" << msg_type << std::endl;
                throw "Unknown msg_type";
            }
        }
        //std::cout << "[broker] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! status = " << in.rdstate() << std::endl;
        //std::cout << "[broker] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! isbad = " << in.bad() << std::endl;
        //std::cout << "[broker] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! iseof = " << in.eof() << std::endl;
        //std::cout << "[broker] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! isfail = " << in.fail() << std::endl;
        //getchar();

        //in->seekg(0, std::ios::beg);
        //in->seekp(0, std::ios::beg);
    }

    void UpdateRoot()
    {
    }

    int64_t count = 0;

    void Do()
    {
        ReadInput();
        //spreader->Do();
        root->Do();
        ReadOrders();
        //spreader->Print();
        //root->Print();
        count++;
        if (count > 10000)
        {
            root->Print();
            count = 0;
        }
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
        //std::cout << "[script-broket] data from storage " << storage.l1[1].bid << " " << storage.l1[1].ask << " " << &storage << std::endl;
        if (out.tellp() > out.tellg())
        {
            stream << out.rdbuf();
        }
        return *this;
    }

    std::stringstream &In() override
    {
        //std::cout << "GET IN STREAM" << std::endl;
        return in;
    };

    ~ScriptBroker()
    {
        log.close();
        std::cout << "~ScriptBroker" << std::endl;
    }
};

#endif