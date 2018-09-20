#ifndef __SCRIPT_H__
#define __SCRIPT_H__

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

struct SimpleOrder
{
    struct State
    {
        int status;
        int64_t ext_id;
        int64_t orderid;
        int64_t price;
        int32_t amout;
        int32_t remainingAmount;
        int32_t dir = 1;
    } state;

    int64_t desired_price = 0;
    int32_t desired_amount = 0;

    SimpleOrder()
    {
    }

    //reaction
    void WriteNewOrder()
    {
        //new_cb(*this)
    }

    void WriteCancelOrder()
    {
        //cancel_cb(*this);
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
};

template <typename T>
struct Script : BasePipe
{
  public:
    struct State
    {
        int status;
        int64_t ext_id;
        int64_t orderid;
        int64_t price;
        int32_t amout;
        int32_t remainingAmount;
        int32_t dir = 1;
    } state;

    std::stringstream in;
    std::stringstream out;
    int64_t ts;
    int64_t desired_price = 0;
    int64_t bid;
    int64_t ask;
    int32_t position = 0;
    int64_t money = 0;
    double profit = 0;
    std::ofstream f;

    SimpleOrder buy;

    Script()
    {
        state.status = FREE;
        state.ext_id = 10;
        desired_price = 1;
    };

    Script(std::string file_name)
    {
        state.status = FREE;
        state.ext_id = 10;
        desired_price = 1;
        if (file_name.size() > 0)
        {
            f.open(file_name);
        }
    }

    void WriteMsgType(int msg_type)
    {
        //int msg_type = NEW_ORDER;
        out.write((char *)&msg_type, sizeof(msg_type));
    }

    void WriteNewOrder()
    {
        assert(state.status == FREE);
        int msg_type = NEW_ORDER;
        out.write((char *)&msg_type, sizeof(msg_type));

        NewOrder new_order = {.ts = ts, .user_code = 1, .isin_id = 1, .ext_id = state.ext_id++, .price = desired_price, .amount = 1, .dir = state.dir};
        out.write((char *)&new_order, sizeof(new_order));

        std::cout << "WRITE NEW ORDER \n";
        state.status = PENDING_NEW;
        state.remainingAmount = 1;
    }

    void WriteNewOrder1()
    {
        assert(buy.state.status == FREE);
        WriteMsgType(NEW_ORDER);

        NewOrder new_order = {.ts = ts,
                              .user_code = 1,
                              .isin_id = 1,
                              .ext_id = state.ext_id++,
                              .price = buy.desired_price,
                              .amount = buy.desired_amount,
                              .dir = buy.state.dir};

        buy.state.status = PENDING_NEW;
    }

    void WriteCancelOrder()
    {
        assert(state.status == NEW);
        int msg_type = CANCEL_ORDER;
        out.write((char *)&msg_type, sizeof(msg_type));

        CancelOrder cancel_order = {.ts = ts, .user_code = 1, .isin_id = 1, .orderid = state.orderid};
        out.write((char *)&cancel_order, sizeof(cancel_order));
        std::cout << FgGreen << "[script] CANCEL_ORDER " << state.orderid << Reset << std::endl;

        state.status = PENDING_CANCEL;
    }

    void ReadNewReply()
    {
        NewReply new_reply;
        in.read((char *)&new_reply, sizeof(new_reply));

        std::cout << "[script] NEW_REPLY" << new_reply;

        if (new_reply.code == 0)
        {
            state.orderid = new_reply.orderid;
            state.status = NEW;
        }
        else
        {
            throw "new_reply.code != 0";
        }
    }

    void ReadCancelReply()
    {
        CancelReply cancel_reply;
        in.read((char *)&cancel_reply, sizeof(cancel_reply));

        if (cancel_reply.code == 0)
        {
            state.status = CANCELED;
            state.status = FREE;
            state.orderid = 0;
        }
        else if (cancel_reply.code == ORDER_NOT_FOUND && state.remainingAmount == 0)
        {
            state.status = FREE;
        }
        else if (cancel_reply.code == ORDER_NOT_FOUND && state.remainingAmount > 0)
        {
            std::cout << "[script]  ORDER_NOT_FOUND" << std::endl;
            //getchar();
            state.status = CANCELED;
        }
    }

    void ReadTrade()
    {
        Trade trade;
        in.read((char *)&trade, sizeof(trade));
        if (state.dir == 1)
        {
            position += trade.amount;
            money -= trade.amount * trade.deal_price;
        }
        else
        {
            position -= trade.amount;
            money += trade.amount * trade.deal_price;
        }
        profit = money + position * trade.deal_price;
        if (position < 0)
        {
            state.dir = 1;
        }

        if (position > 0)
        {
            state.dir = 2;
        }
        std::cout << FgGreen << "profit = " << profit / 1000000 << Reset << std::endl;
        if (f.is_open())
        {
            f << profit / 1000000 << std::endl;
        }

        std::cout << "[script] new trade position " << position << " " << trade.deal_price << " " << trade.amount << " " << trade.orderid << std::endl;
        state.remainingAmount = 0;
        //state.status = FREE;
        // getchar();
    }

    void Do()
    {
        std::cout << "DO DO DO DO ....................................... state " << state.status << std::endl;
        switch (state.status)
        {
        case FREE:
            WriteNewOrder();
            //getchar();
            break;
        case PENDING_NEW:
            break;
        case NEW:
            if (desired_price != state.price)
            {
                WriteCancelOrder();
            }

            break;
        case PENDING_CANCEL:
            break;
        case CANCELED:
            //getchar();
            break;
        }
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

    void WriteTimeStamp(std::stringstream &to)
    {
        int msg_type = TIMESTAMP_MSG;
        to.write((char *)&msg_type, sizeof(msg_type));
        to.write((char *)&ts, sizeof(ts));
        //std::cout << "WRITE TO SCRIPT :::::::::::::::::::::::::::::::" << std::endl;
    }

    void ResetIn()
    {
        in.seekg(0, std::ios::beg);
        in.seekp(0, std::ios::beg);
        //std::cout << "SCRIPT RESET IN \n";
    }

    void ReadTimeStamp()
    {
        in.read((char *)&ts, sizeof(ts));
        //std::cout << ":::::::::::::::::::::::timestamp " << ts << std::endl;
    }

    void ReadMarketDataL1()
    {
        MktDataL1 mkt_data_l1;
        in.read((char *)&mkt_data_l1, sizeof(mkt_data_l1));
        std::cout << "[script] market_data_l1 " << mkt_data_l1.bid << " | " << mkt_data_l1.ask << std::endl;

        if (mkt_data_l1.bid > 0)
        {
            bid = mkt_data_l1.bid;
            if (state.dir == 1)
            {
                desired_price = bid - 4 * 1000000L;
            }
        }
        if (mkt_data_l1.ask > 0)
        {
            ask = mkt_data_l1.ask;
            if (state.dir == 2)
            {
                desired_price = ask + 4 * 1000000L;
            }
        }
    }

    BasePipe &operator|(std::stringstream &to)
    {
        //read input stream
        while (true)
        {
            int msg_type = ReadMessageType();
            switch (msg_type)
            {
            case EMPTY:
                ResetIn();
                break;
            case TIMESTAMP_MSG:
                ReadTimeStamp();
                break;
            case NEW_REPLY_MSG:
                ReadNewReply();
                break;
            case CANCEL_REPLY_MSG:
                ReadCancelReply();
                break;
            case TRADE_MSG:
                ReadTrade();
                break;
            case MKT_DATA_L1:
                ReadMarketDataL1();
                break;
            default:
                throw "unknow state";
            }

            if (msg_type == EMPTY)
            {
                break;
            }
        }

        Do();
        std::cout << "[script-start] mkt.in.tellp = " << to.tellp() << " mkt.in.tellg = " << to.tellg() << std::endl;
        //write output stream
        WriteTimeStamp(to);

        if (out.tellp() > out.tellg())
        {
            to << out.rdbuf(); //copy
        }

        std::cout << "[script-end] mkt.in.tellp = " << to.tellp() << " mkt.in.tellg = " << to.tellg() << std::endl;
        return *this;
    }

    BasePipe &operator|(BasePipe &to) override
    {
        return to;
    }

    std::stringstream &In() override
    {
        return in;
    };

    ~Script()
    {
        if (f.is_open())
        {
            f.close();
        }
    };
};

#endif