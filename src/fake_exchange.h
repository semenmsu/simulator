#ifndef __FAKE_EXCHANGE_H__
#define __FAKE_EXCHANGE_H__
#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <unordered_map>
#include "script_broker.h"
#include "iproto.h"

template <typename T>
class FakeExchange : BasePipe
{
    typedef typename std::unordered_map<int64_t, T> Orders;
    typedef typename std::set<T *, GreaterComparator<T *>> BuySet;
    typedef typename std::set<T *, LessComparator<T *>> SellSet;
    Orders orders;
    BuySet buyOrders;
    SellSet sellOrders;

  public:
    int64_t bid = 1000000;
    int64_t ask = 1000001;
    std::stringstream out;
    std::stringstream in;
    int64_t ts;
    int64_t global_order_id;
    FakeExchange()
    {
        srand(time(NULL));
        global_order_id = 1;
    }

    void WriteMarketDataL1()
    {
        //not found
        struct MktDataL1 mkt_data_l1;
        mkt_data_l1.isin_id = 1;
        mkt_data_l1.bid = bid;
        mkt_data_l1.ask = ask;
        mkt_data_l1.is_ready = 1;
        int msg_type = MKT_DATA_L1;
        out.write((char *)&msg_type, sizeof(msg_type));
        out.write((char *)&mkt_data_l1, sizeof(mkt_data_l1));
    }

    void Generate()
    {
        bid = bid + rand() % 3 - 1;
        ask = bid + rand() % 3 + 1;
        printf("bid = %ld, ask = %ld\n", bid, ask);
    }

    int ReadMessageType()
    {
        if (in.tellg() < in.tellp())
        {
            int msg_type;
            in.read((char *)&msg_type, sizeof(msg_type));
            return msg_type;
        }
        else
        {
            return EMPTY;
        }
    }

    void ReadTimeStamp()
    {
        int64_t ists;
        in.read((char *)&ists, sizeof(ists));
        std::cout << "[fake-exchange] read timestamp " << ists << std::endl;
        //std::cout << ":::::::::::::::::::::::timestamp " << ts << std::endl;
    }

    void ReadNewOrder()
    {

        NewOrder new_order;
        in.read((char *)&new_order, sizeof(new_order));
        T t;
        t.action = 1;
        t.dir = (int)new_order.dir;
        t.orderid = 0;
        t.price = new_order.price;
        t.amount = new_order.amount;
        t.ext_id = new_order.ext_id;
        std::cout << "[fake-exchange] " << new_order;
        PlaceOrder(t);
    }

    void ReadCancelOrder()
    {
        CancelOrder cancel_order;
        in.read((char *)&cancel_order, sizeof(cancel_order));
        T t;
        t.action = 0;
        t.orderid = cancel_order.orderid;
        t.user_code = cancel_order.user_code;
        std::cout << "[fake-exchange] " << cancel_order;
        PlaceOrder(t);
    }

    void PlaceOrder(T &order)
    {
        if (order.action == 0)
        {
            //cancel
            if (orders.count(order.orderid) > 0)
            {
                int amount = orders[order.orderid].amount;
                orders.erase(order.orderid);
                struct CancelReply reply = {.ext_id = order.ext_id, .orderid = order.orderid, .code = 0, .amount = amount};
                std::cout << reply;
                int msg_type = CANCEL_REPLY_MSG;
                out.write((char *)&msg_type, sizeof(msg_type));
                out.write((char *)&reply, sizeof(reply));
            }
            else
            {
                struct CancelReply reply = {.ext_id = order.ext_id, .orderid = order.orderid, .code = ORDER_NOT_FOUND, .amount = 0};
                int msg_type = CANCEL_REPLY_MSG;
                out.write((char *)&msg_type, sizeof(msg_type));
                out.write((char *)&reply, sizeof(reply));
                //not found
            }
        }
        else if (order.action == 1)
        {
            order.orderid = ++global_order_id;
            orders.insert(std::pair<int64_t, T>((int64_t)order.orderid, order));
            struct NewReply reply = {.ext_id = order.ext_id, .orderid = order.orderid, .code = 0};
            std::cout << "[market] SEND NEW_REPLY " << reply << std::endl;
            int msg_type = NEW_REPLY_MSG;
            out.write((char *)&msg_type, sizeof(msg_type));
            out.write((char *)&reply, sizeof(reply));
        }
    }

    void ReadInputStream()
    {
        //read input stream
        int msg_type;
        while ((msg_type = ReadMessageType()) != EMPTY)
        {
            switch (msg_type)
            {
            case TIMESTAMP_MSG:
                std::cout << "[market] TIMESTAMP_MSG\n";
                ReadTimeStamp();
                break;
            case NEW_ORDER:
                std::cout << "[market] NEW_ORDER\n";
                ReadNewOrder();
                break;
            case CANCEL_ORDER:
                std::cout << "[market] CANCEL_ORDER\n";
                ReadCancelOrder();
                break;
            default:
                throw "unknow state";
            }

            if (msg_type == EMPTY)
            {
                break;
            }
        }
    }

    void WriteTimeStamp(std::stringstream &stream)
    {
        int msg_type = TIMESTAMP_MSG;
        stream.write((char *)&msg_type, sizeof(msg_type));
        stream.write((char *)&ts, sizeof(ts));
        std::cout << "WRITE TO SCRIPT :::::::::::::::::::::::::::::::" << std::endl;
    }

    //handle script !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    BasePipe &operator|(BasePipe &to)
    {
        /*
        std::cout << "[market-start] mkt.in.tellp = " << in.tellp() << " mkt.in.tellg = " << in.tellg() << std::endl;
        ReadOrderFile();
        std::cout << "ReadInputStream \n";
        ReadInputStream();
        WriteTimeStamp(to.In());

        std::cout << "[market-before-copy] script.in.tellp = " << to.In().tellp() << " script.in.tellg = " << to.In().tellg() << std::endl;
        if (out.tellp() > out.tellg())
        {
            to.In() << out.rdbuf(); //copy
        }

        std::cout << "[market-after-copy] script.in.tellp = " << to.In().tellp() << " script.in.tellg = " << to.In().tellg() << std::endl;*/

        Generate();

        ReadInputStream();
        WriteMarketDataL1();

        WriteTimeStamp(to.In());

        if (out.tellp() > out.tellg())
        {
            to.In() << out.rdbuf(); //copy
        }
        //out.seekg(0, std::ios::beg);
        //out.seekp(0, std::ios::beg);

        return to;
    }

    BasePipe &operator|(std::stringstream &stream)
    {
        return *this;
    }

    friend FakeExchange &operator>>(int ts, FakeExchange &exchange)
    {
        exchange.ts = ts;
        return exchange;
    }

    std::stringstream &In() override
    {
        return in;
    };
};

#endif // !__FAKE_EXCHANGE_H__