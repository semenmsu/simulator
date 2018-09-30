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
#include <queue>
#include <functional>
#include "iproto.h"
#include "script_order.h"
#include "data_storage.h"
#include "root_node.h"

using std::placeholders::_1;
using std::placeholders::_2;

//One way direction
struct VirtSocket
{
    std::stringstream *stream;

    std::function<void(int64_t)> timestamp_function;
    std::function<void(MktDataL1 &)> mkt_data_l1_function;
    std::function<void(InstrumentInfoReply &)> instrument_info_reply_function;

    std::function<void(NewReply &)> new_reply_function;
    std::function<void(CancelReply &)> cancel_reply_function;
    std::function<void(Trade &)> trade_reply_function;

    // void HandleNewOrder(ScriptOrder *pointer, NewOrder &new_order)
    std::function<void(ScriptOrder *, NewOrder &)> new_order_function;
    std::function<void(ScriptOrder *, CancelOrder &)> cancel_order_function;
    std::function<void(ScriptOrder *, InstrumentInfoRequest &)> instrument_info_request_function;

    VirtSocket(std::stringstream *stream)
    {
        this->stream = stream;
    }

    //timestamp
    void On(std::function<void(int64_t)> cb)
    {
        timestamp_function = cb;
    }

    template <class T>
    void On(T &t, void (T::*f)(int64_t))
    {
        timestamp_function = std::bind(f, &t, _1);
    }

    void On(std::function<void(MktDataL1 &)> cb)
    {
        mkt_data_l1_function = cb;
    }

    template <class T>
    void On(T &t, void (T::*f)(MktDataL1 &))
    {
        mkt_data_l1_function = std::bind(f, &t, _1);
    }

    void On(std::function<void(InstrumentInfoReply &)> cb)
    {
        instrument_info_reply_function = cb;
    }

    template <class T>
    void On(T &t, void (T::*f)(InstrumentInfoReply &))
    {
        instrument_info_reply_function = std::bind(f, &t, _1);
    }

    template <class T>
    void On(T &t, void (T::*f)(NewReply &))
    {
        new_reply_function = std::bind(f, &t, _1);
    }

    template <class T>
    void On(T &t, void (T::*f)(CancelReply &))
    {
        cancel_reply_function = std::bind(f, &t, _1);
    }

    template <class T>
    void On(T &t, void (T::*f)(Trade &))
    {
        trade_reply_function = std::bind(f, &t, _1);
    }

    template <class T>
    void On(T &t, void (T::*f)(ScriptOrder *, NewOrder &))
    {
        new_order_function = std::bind(f, &t, _1, _2);
    }

    template <class T>
    void On(T &t, void (T::*f)(ScriptOrder *, CancelOrder &))
    {
        cancel_order_function = std::bind(f, &t, _1, _2);
    }

    template <class T>
    void On(T &t, void (T::*f)(ScriptOrder *, InstrumentInfoRequest &))
    {
        instrument_info_request_function = std::bind(f, &t, _1, _2);
    }

    //system
    int ReadMessageType(std::stringstream *stream)
    {
        if (stream->tellg() < stream->tellp())
        {
            int msg_type;
            stream->read((char *)&msg_type, sizeof(msg_type));
            return msg_type;
        }
        else
        {
            return EMPTY;
        }
    }

    void ReadTimeStamp()
    {
        int64_t ts;
        stream->read((char *)&ts, sizeof(ts));
        //std::cout << "receive timestamp" << std::endl;
        timestamp_function(ts);
    }

    void ReadMarketDataL1()
    {
        MktDataL1 mkt_data_l1;
        stream->read((char *)&mkt_data_l1, sizeof(mkt_data_l1));
        //std::cout << "receive Market data l1" << std::endl;
        mkt_data_l1_function(mkt_data_l1);
    }

    void ReadInstrumentInfoReply()
    {
        //std::cout << "[broker] INSTRUMENT_INFO_REPLY\n";
        InstrumentInfoReply info_reply;
        stream->read((char *)&info_reply, sizeof(info_reply));
        //std::cout << "receive Instrument Info Reply" << std::endl;
        instrument_info_reply_function(info_reply);
    }

    void ReadNewReplyMsg()
    {
        NewReply new_reply;
        stream->read((char *)&new_reply, sizeof(new_reply));
        new_reply_function(new_reply);
    }

    void ReadCancelRepyMsg()
    {
        CancelReply cancel_reply;
        stream->read((char *)&cancel_reply, sizeof(cancel_reply));
        cancel_reply_function(cancel_reply);
    }

    void ReadTradeMsg()
    {
        Trade trade;
        stream->read((char *)&trade, sizeof(trade));
        trade_reply_function(trade);
    }

    void ReadNewOrder()
    {
        ScriptOrder *pointer;
        stream->read((char *)&pointer, sizeof(pointer));

        NewOrder new_order;
        stream->read((char *)&new_order, sizeof(new_order));

        new_order_function(pointer, new_order);
    }

    void ReadCancelOrder()
    {
        ScriptOrder *pointer;
        stream->read((char *)&pointer, sizeof(pointer));

        CancelOrder cancel_order;
        stream->read((char *)&cancel_order, sizeof(cancel_order));

        cancel_order_function(pointer, cancel_order);
    }

    void ReadInstrumentInfoRequest()
    {
        //std::cout << "READ REQUEST INFO" << std::endl;
        ScriptOrder *pointer;
        stream->read((char *)&pointer, sizeof(pointer));
        InstrumentInfoRequest info_request;
        stream->read((char *)&info_request, sizeof(info_request));

        instrument_info_request_function(pointer, info_request);
    }

    void Do()
    {
        int msg_type;

        while ((msg_type = ReadMessageType(stream)) != EMPTY)
        {
            switch (msg_type)
            {
            case TIMESTAMP_MSG:
                ReadTimeStamp();
                break;
            case MKT_DATA_L1:
                ReadMarketDataL1();
                break;
            case INSTRUMENT_INFO_REPLY:
                ReadInstrumentInfoReply();
                break;
            case NEW_REPLY_MSG:
                ReadNewReplyMsg();
                break;
            case CANCEL_REPLY_MSG:
                ReadCancelRepyMsg();
                break;
            case TRADE_MSG:
                ReadTradeMsg();
                break;
            case NEW_ORDER:
                ReadNewOrder();
                break;
            case CANCEL_ORDER:
                ReadCancelOrder();
                break;
            case INSTRUMENT_INFO_REQUEST:
                ReadInstrumentInfoRequest();
                break;
            default:
                break;
            }
        }
    }
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
    int64_t count = 0;

    //Spreader *spreader;
    RootNode *root;
    std::ofstream log;

    VirtSocket *socket;
    VirtSocket *orders_socket;

    ScriptBroker()
    {
        ConnectProtocolSocket();

        SetupExtId();

        CreateRootScript();

        CreateLog();

        CreateStorage();
    }

    OrderSession *GetSessionByExtId(int id)
    {
        return extid2session[id];
    }

    //Handlers
    void HandleTimestamp(int64_t ts)
    {
        //std::cout << "receive ts " << ts << std::endl;

        root->UpdateTime(ts);
    }

    void HandleMktDataL1(MktDataL1 &mkt_data)
    {
        //std::cout << "mkt_data " << mkt_data.bid << " " << mkt_data.ask << std::endl;

        storage.UpdateL1(mkt_data);
    }

    void HandleInstrumentInfoReply(InstrumentInfoReply &info_reply)
    {
        std::cout << "Instrument Info Reply " << std::endl;
        //getchar();
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

    void HandleNewReplyMsg(NewReply &new_reply)
    {
        OrderSession *session = extid2session[new_reply.ext_id];
        new_reply.ext_id = session->ext_id;
        session->ts_new_reply = new_reply.ts;
        orderid2session.insert(std::pair<int64_t, OrderSession *>((int64_t)new_reply.orderid, session));
        session->order->ReplyNew(new_reply);
    }

    void HandleCancelReplyMsg(CancelReply &cancel_reply)
    {
        OrderSession *session = orderid2session[cancel_reply.orderid];
        session->order->ReplyCancel(cancel_reply);
    }

    void HandleTradeMsg(Trade &trade)
    {
        OrderSession *session = orderid2session[trade.orderid];
        LogToFile(trade.to_csv());
        session->order->ReplyTrade(trade);
    }

    void HandleNewOrder(ScriptOrder *pointer, NewOrder &new_order)
    {

        OrderSession *session = &order2session[pointer];
        new_order.ext_id = GenereateExtId();
        new_order.ts = ts + GetOrderDelay();
        session->order = pointer;
        session->external_ext_id = new_order.ext_id;
        session->ext_id = new_order.ext_id;
        session->ts_new = ts;
        extid2session.insert(std::pair<int64_t, OrderSession *>((int64_t)new_order.ext_id, session));

        WriteMsgType(NEW_ORDER);
        out.write((char *)&new_order, sizeof(new_order));
    }

    void HandleCancelOrder(ScriptOrder *pointer, CancelOrder &cancel_order)
    {
        cancel_order.ts = ts + GetOrderDelay();

        WriteMsgType(CANCEL_ORDER);
        out.write((char *)&cancel_order, sizeof(cancel_order));
    }

    void HandleInstrumentInfoRequest(ScriptOrder *pointer, InstrumentInfoRequest &info_request)
    {
        info_request.ext_id = GenereateExtId();
        OrderSession *session = &order2session[pointer];
        session->order = pointer;
        extid2session.insert(std::pair<int64_t, OrderSession *>((int64_t)info_request.ext_id, session));

        WriteMsgType(INSTRUMENT_INFO_REQUEST);
        out.write((char *)&info_request, sizeof(info_request));
    }

    void ConnectProtocolSocket()
    {
        socket = new VirtSocket(&in);
        orders_socket = new VirtSocket(&orders);

        socket->On(*this, &ScriptBroker::HandleTimestamp);
        socket->On(*this, &ScriptBroker::HandleMktDataL1);
        socket->On(*this, &ScriptBroker::HandleInstrumentInfoReply);
        socket->On(*this, &ScriptBroker::HandleNewReplyMsg);
        socket->On(*this, &ScriptBroker::HandleCancelReplyMsg);
        socket->On(*this, &ScriptBroker::HandleTradeMsg);

        orders_socket->On(*this, &ScriptBroker::HandleNewOrder);
        orders_socket->On(*this, &ScriptBroker::HandleCancelOrder);
        orders_socket->On(*this, &ScriptBroker::HandleInstrumentInfoRequest);
    }

    void CreateRootScript()
    {
        //spreader = new Spreader(storage, orders);
        root = new RootNode(storage, orders);
        root->RequestSettings();
    }

    void CreateLog()
    {
        log.open("result/robo.csv");
    }

    void CreateStorage()
    {
        storage.RequestInstrumentInfo(out);
    }

    void SetupExtId()
    {
        ext_id = 1000;
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
                root->UpdateTime(ts);
                //std::cout << ts << std::endl;
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

        PrintStreamStatus(in);
    }

    void Do()
    {
        socket->Do();
        //ReadInput(); //read input stream

        root->Do();

        //ReadOrders(); //read output stream
        orders_socket->Do();

        Print();
    }

    void Print()
    {
        count++;
        if (count > 10000)
        {
            root->Print();
            count = 0;
        }
    }

    void PrintStreamStatus(std::stringstream &in)
    {
        //std::cout << "[broker] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! status = " << in.rdstate() << std::endl;
        //std::cout << "[broker] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! isbad = " << in.bad() << std::endl;
        //std::cout << "[broker] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! iseof = " << in.eof() << std::endl;
        //std::cout << "[broker] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! isfail = " << in.fail() << std::endl;
        //getchar();

        //in->seekg(0, std::ios::beg);
        //in->seekp(0, std::ios::beg);
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