#ifndef __MARKET_H__
#define __MARKET_H__

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
#include "../include/reader.h"

#define MAX_DEPTH 10

#define USER_CODE

#define ORDER_NOT_FOUND 14
#define CROSS_ORDER_ERR 31 //
#define PRICE_OUTSIDE_LIMIT 32
#define INPUT_PARAM_ERR 35
#define PRICE_STEP_ERR 39
#define EXCEEDED_TRANSACTION_LIMIT 54

#define EMPTY 0
#define TIMESTAMP_MSG 1
#define NEW_REPLY_MSG 2
#define CANCEL_REPLY_MSG 3
#define TRADE_MSG 4
#define NEW_ORDER 101
#define CANCEL_ORDER 102

#define MKT_DATA_L1 10001

#define ORDERID_MULT 1000

#define Reset "\x1b[0m"
//#define Bright "\x1b[1m"
//#define Dim \x1b[2m
//#define Underscore \x1b[4m
//#define Blink \x1b[5m
//#define Reverse \x1b[7m
//#define Hidden \x1b[8m

//#define FgBlack \x1b[30m
#define FgRed "\x1b[31m"
#define FgGreen "\x1b[32m"
//#define FgYellow \x1b[33m
//#define FgBlue \x1b[34m
#define FgMagenta "\x1b[35m"
//#define FgCyan \x1b[36m
//#define FgWhite \x1b[37m

//#define BgBlack \x1b[40m
//#define BgRed \x1b[41m
//#define BgGreen \x1b[42m
//#define BgYellow \x1b[43m
//#define BgBlue \x1b[44m
//#define BgMagenta \x1b[45m
//#define BgCyan \x1b[46m
//#define BgWhite \x1b[47m

//for matching only need price, orderid, amount, action

struct NewOrder
{
    int64_t ts;
    int32_t user_code;
    int32_t isin_id;
    int64_t ext_id;
    int64_t price;
    int32_t amount;
    int32_t dir;

    friend std::ostream &operator<<(std::ostream &stream, NewOrder &new_order)
    {
        std::cout << FgGreen << "[NEW_ORDER] " << new_order.user_code << " " << new_order.ext_id << " " << new_order.price << " " << new_order.amount << Reset << std::endl;
        return stream;
    }
} __attribute__((packed, aligned(4)));

struct CancelOrder
{
    int64_t ts;
    int32_t user_code;
    int32_t isin_id;
    int64_t orderid;

    friend std::ostream &operator<<(std::ostream &stream, CancelOrder &cancel_order)
    {
        std::cout << FgGreen << "[CANCEL_ORDER] " << cancel_order.user_code << " " << cancel_order.isin_id << " " << cancel_order.orderid << Reset << std::endl;
        return stream;
    }

} __attribute__((packed, aligned(4)));

struct NewReply
{
    int64_t ext_id;
    int64_t orderid;
    int32_t code;

    friend std::ostream &operator<<(std::ostream &stream, NewReply &reply)
    {
        std::cout << FgRed << "[NEW_REPLY] " << reply.ext_id << " " << reply.orderid << " " << reply.code << Reset << std::endl;
        return stream;
    }

} __attribute__((packed, aligned(4)));

struct CancelReply
{
    int64_t ext_id;
    int64_t orderid;
    int32_t code;
    int32_t amount;

    friend std::ostream &operator<<(std::ostream &stream, CancelReply &reply)
    {
        std::cout << FgRed << "[CANCEL_REPLY] " << reply.ext_id << " " << reply.orderid << " " << reply.code << " " << reply.amount << Reset << std::endl;
        return stream;
    }
} __attribute__((packed, aligned(4)));

struct Trade
{
    int64_t orderid;
    int64_t deal_price;
    int32_t amount;
    int32_t user_code;
} __attribute__((packed, aligned(4)));

struct MktDataL1
{
    int32_t isin_id;
    int64_t bid;
    int64_t ask;
} __attribute__((packed, aligned(4)));

struct BasePipe
{
    virtual BasePipe &operator|(BasePipe &) = 0;
    virtual BasePipe &operator|(std::stringstream &) = 0;
    virtual std::stringstream &In() = 0;
};

//send input to script and get answers + pass timestamps
#define FREE 0
#define PENDING_NEW 1
#define NEW 2
#define PENDING_CANCEL 3
#define CANCELED 4

template <typename T>
struct Script : BasePipe
{
    struct State
    {
        int status;
        int64_t ext_id;
        int64_t orderid;
        int64_t price;
        int32_t amout;
    } state;

  public:
    std::stringstream in;
    std::stringstream out;
    int64_t ts;
    int64_t desired_price = 0;
    int64_t bid;
    int64_t ask;

    Script()
    {
        state.status = FREE;
        state.ext_id = 10;
        desired_price = 1;
    };

    void WriteNewOrder()
    {
        assert(state.status == FREE);
        int msg_type = NEW_ORDER;
        out.write((char *)&msg_type, sizeof(msg_type));

        NewOrder new_order = {.ts = ts, .user_code = 1, .isin_id = 1, .ext_id = state.ext_id++, .price = desired_price, .amount = 1, .dir = 1};
        out.write((char *)&new_order, sizeof(new_order));

        std::cout << "WRITE NEW ORDER \n";
        state.status = PENDING_NEW;
    }

    void WriteCancelOrder()
    {
        assert(state.status == NEW);
        int msg_type = CANCEL_ORDER;
        out.write((char *)&msg_type, sizeof(msg_type));

        /*int64_t ts;
    int32_t user_code;
    int32_t isin_id;
    int64_t orderid;*/
        CancelOrder cancel_order = {.ts = ts, .user_code = 1, .isin_id = 1, .orderid = state.orderid};
        out.write((char *)&cancel_order, sizeof(cancel_order));

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
        else if (cancel_reply.code == ORDER_NOT_FOUND)
        {
            state.status = CANCELED;
        }
    }

    void ReadTrade()
    {
        Trade trade;
        in.read((char *)&trade, sizeof(trade));
        std::cout << "new trade: " << trade.deal_price << " " << trade.amount << std::endl;
        getchar();
    }

    void Do()
    {
        std::cout << "DO DO DO DO ....................................... state " << state.status << std::endl;
        switch (state.status)
        {
        case FREE:
            WriteNewOrder();
            break;
        case PENDING_NEW:
            break;
        case NEW:
            WriteCancelOrder();
            break;
        case PENDING_CANCEL:
            break;
        case CANCELED:
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
            desired_price = bid - 100 * 1000000L;
        }
        if (mkt_data_l1.ask > 0)
        {
            ask = mkt_data_l1.ask;
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
};

template <typename T>
struct LessComparator
{
    bool operator()(const T &left, const T &right)
    {
        if (left.price != right.price)
        {
            return left.price < right.price;
        }
        return left.orderid < right.orderid;
    };
};

template <typename T>
struct GreaterComparator
{
    bool operator()(const T &left, const T &right)
    {
        if (left.price != right.price)
        {
            return left.price > right.price;
        }
        return left.orderid < right.orderid;
    };
};

template <typename T>
struct IdComparator
{
    bool operator()(const T &left, const T &right)
    {
        return left.orderid < right.orderid;
    };
};

template <typename T>
class Market : BasePipe
{
    typedef typename std::set<T, IdComparator<T>> OrderSet;
    typedef typename std::set<T, GreaterComparator<T>> BuySet;
    typedef typename std::set<T, LessComparator<T>> SellSet;
    typedef typename std::unordered_map<std::pair<int32_t, int64_t>, T> Ccid2Order; //cliet client id

    //typedef typename std::unordered_map<int64_t, int64_t> Id2Cid;
    //typedef typename std::unordered_map<int64_t, int64_t> Cid2Id;

    OrderSet orders;
    BuySet buyOrders;
    SellSet sellOrders;

  public:
    std::stringstream out;
    std::stringstream in;
    Reader *reader;
    int64_t ts;
    int64_t last_order_id;
    Market(){

    };

    Market(Reader &rdr)
    {
        reader = &rdr;
    }

    Market(std::string order_book)
    {
        this->operator<<(order_book);
    }
    void PlaceOrder(T &order);

    Market &operator<<(std::string order_book)
    {

        std::istringstream f(order_book);
        std::string line;
        while (std::getline(f, line))
        {
            line.erase(line.find_last_not_of(" \n\r\t") + 1);
            if (line.size() > 0)
            {
                T t;
                t << line;
                PlaceOrder(t);
            }
        }
        return *this;
    }

    bool operator==(const Market &mkt)
    {
        if (buyOrders.size() != mkt.buyOrders.size())
        {
            return false;
        }
        if (sellOrders.size() != mkt.sellOrders.size())
        {
            return false;
        }
        typename BuySet::iterator buy_it = buyOrders.begin();
        typename BuySet::iterator mkt_buy_it = mkt.buyOrders.begin();
        while (buy_it != buyOrders.end())
        {
            T t = *buy_it;
            T mkt_t = *mkt_buy_it;
            if (t != mkt_t)
            {

                std::cout << "return false \n";
                return false;
            }
            buy_it++;
            mkt_buy_it++;
        }

        typename SellSet::iterator sell_it = sellOrders.begin();
        typename SellSet::iterator mkt_sell_it = mkt.sellOrders.begin();
        while (sell_it != sellOrders.end())
        {
            T t = *sell_it;
            T mkt_t = *mkt_sell_it;
            if (t != mkt_t)
            {
                return false;
            }
            sell_it++;
            mkt_sell_it++;
        }
        return true;
    }

    void ResetIn()
    {
        in.seekg(0, std::ios::beg);
        in.seekp(0, std::ios::beg);
        std::cout << "[market] RESET IN \n";
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
        std::cout << "[market] read timestamp " << ists << std::endl;
        //std::cout << ":::::::::::::::::::::::timestamp " << ts << std::endl;
    }

    void ReadNewOrder()
    {

        NewOrder new_order;
        in.read((char *)&new_order, sizeof(new_order));
        T t;
        /*int64_t ts;
    int32_t user_code;
    int32_t isin_id;
    int64_t ext_id;
    int64_t price;
    int32_t amount;
    int32_t dir;*/
        t.action = 1;
        t.dir = (int)new_order.dir;
        t.orderid = 0;
        t.price = new_order.price;
        t.amount = new_order.amount;
        t.ext_id = new_order.ext_id;
        t.user_code = new_order.user_code;
        //std::cout << "[script] READ NEW ORDER ext_id " << t.ext_id << std::endl;
        std::cout << "[script] " << new_order;
        PlaceOrder(t);
    }

    void ReadCancelOrder()
    {
        /*int64_t ts;
    int32_t user_code;
    int32_t isin_id;
    int64_t orderid;*/
        CancelOrder cancel_order;
        in.read((char *)&cancel_order, sizeof(cancel_order));
        T t;
        t.action = 0;
        t.orderid = cancel_order.orderid;
        t.user_code = cancel_order.user_code;
        std::cout << "[script] " << cancel_order;
        PlaceOrder(t);
    }

    void ReadInputStream()
    {
        //read input stream
        while (true)
        {
            int msg_type = ReadMessageType();
            switch (msg_type)
            {
            case EMPTY:
                std::cout << "[market] EMPTY\n";
                ResetIn();
                break;
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

    void WriteMarketDataL1(int64_t bid, int64_t ask)
    {
        //not found
        struct MktDataL1 mkt_data_l1 = {.isin_id = 1, .bid = bid, .ask = ask};
        int msg_type = MKT_DATA_L1;
        out.write((char *)&msg_type, sizeof(msg_type));
        out.write((char *)&mkt_data_l1, sizeof(mkt_data_l1));
    }

    void ReadOrderFile()
    {
        int count = 0;
        while (count++ < 5000)
        {
            T order;
            reader->Read(order);
            PlaceOrder(order);
        }
        //Print();
        std::cout << GetBid() << " | " << GetAsk() << std::endl;
        WriteMarketDataL1(GetBid(), GetAsk());
        //getchar();
    }

    void WriteTimeStamp(std::stringstream &stream)
    {
        int msg_type = TIMESTAMP_MSG;
        stream.write((char *)&msg_type, sizeof(msg_type));
        stream.write((char *)&ts, sizeof(ts));
        std::cout << "WRITE TO SCRIPT :::::::::::::::::::::::::::::::" << std::endl;
    }

    //handle script !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    BasePipe &operator|(BasePipe &to) override
    {
        std::cout << "[market-start] mkt.in.tellp = " << in.tellp() << " mkt.in.tellg = " << in.tellg() << std::endl;
        ReadOrderFile();
        ReadInputStream();
        WriteTimeStamp(to.In());

        std::cout << "[market-before-copy] script.in.tellp = " << to.In().tellp() << " script.in.tellg = " << to.In().tellg() << std::endl;
        if (out.tellp() > out.tellg())
        {
            to.In() << out.rdbuf(); //copy
        }

        std::cout << "[market-after-copy] script.in.tellp = " << to.In().tellp() << " script.in.tellg = " << to.In().tellg() << std::endl;
        return to;
    }

    BasePipe &operator|(std::stringstream &stream)
    {
        return *this;
    }

    //make matching
    friend Market &operator>>(int ts, Market &market)
    {
        std::cout << "update ts " << ts << std::endl;
        market.ts = ts;
        return market;
    }

    std::stringstream &In() override
    {
        return in;
    };

    int64_t GetBid()
    {
        if (buyOrders.size() > 0)
        {
            auto it = buyOrders.begin();
            return it->price;
        }
        else
        {
            return 0;
        }
    }

    int64_t GetAsk()
    {
        if (sellOrders.size() > 0)
        {
            auto it = sellOrders.begin();
            return it->price;
        }
        else
        {
            return 0;
        }
    }

    void Print()
    {
        typename SellSet::reverse_iterator it = sellOrders.rbegin();
        //td::set<Order, Comparator>::reverse_iterator it = sellOrders.rbegin();
        int count = 0;

        while (it != sellOrders.rend())
        {
            std::cout << *it;
            it++;
        }
        std::cout << "-----" << std::endl;
        count = 0;
        for (auto &ord : buyOrders)
        {
            std::cout << ord;
#ifdef MAX_DEPTH
            count++;
            if (count > MAX_DEPTH)
            {
                break;
            }
#endif
        }
        std::cout << std::endl;
        std::cout << std::endl;
    }

  private:
    void eraseSell(typename SellSet::iterator &order)
    {
        sellOrders.erase(order);
        T t;
        t.orderid = order->orderid;
        typename OrderSet::iterator it = orders.find({t});
        //typename OrderSet::iterator it = orders.find({.orderid = order->orderid});
        orders.erase(it);
    }

    void eraseBuy(typename BuySet::iterator &order)
    {
        buyOrders.erase(order);
        T t;
        t.orderid = order->orderid;
        typename OrderSet::iterator it = orders.find(t);
        //typename OrderSet::iterator it = orders.find({.orderid = order->orderid});
        orders.erase(it);
    }

    int eraseOrder(const T &order)
    {
        T t;
        t.orderid = order.orderid;
        typename OrderSet::iterator it = orders.find(t);
        //typename OrderSet::iterator it = orders.find({.orderid = order.orderid});
        if (it == orders.end())
        {
            //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1
            //std::cout << "CANT FIND THIS ORDER!!! " << order << std::endl;
            //getchar();
            return -1;
        }
        if (it->dir == 1)
        {
            T t;
            t.orderid = it->orderid;
            t.price = it->price;
            typename BuySet::iterator buy_iter = buyOrders.find(t);
            //typename BuySet::iterator buy_iter = buyOrders.find({.orderid = it->orderid, .price = it->price});
            eraseBuy(buy_iter);
            return 0;
        }
        else
        {
            T t;
            t.orderid = it->orderid;
            t.price = it->price;
            typename SellSet::iterator sell_iter = sellOrders.find(t);
            //typename SellSet::iterator sell_iter = sellOrders.find({.orderid = it->orderid, .price = it->price});
            eraseSell(sell_iter);
            return 0;
        }
    }

    void updateSellAmount(typename SellSet::iterator &order, int new_amount)
    {
        T copy = *order;
        copy.amount = new_amount;
        //eraseSell(order);
        eraseOrder(*order);
        insertSellOrder(copy);
    }

    void updateBuyAmount(typename BuySet::iterator &order, int new_amount)
    {
        T copy = *order;
        copy.amount = new_amount;
        //eraseBuy(order);
        eraseOrder(*order);
        insertBuyOrder(copy);
    }

    void insertSellOrder(T &order)
    {
        sellOrders.insert(order);
        orders.insert(order);
    }

    void insertBuyOrder(T &order)
    {
        buyOrders.insert(order);
        orders.insert(order);
    }

    bool IsSimulatedUser(T &order)
    {
        //magic numbers:)
        if (order.user_code > 0 && order.user_code < 1000)
        {
            return true;
        }
        return false;
    }

    void PreOrderPlace(T &order)
    {
        order.orderid = order.orderid * 1000000;
    }
};

template <typename T>
void Market<T>::PlaceOrder(T &order)
{
    //PreOrderPlace(order);
    //printf("placeorder\n");
    //std::cout << order;
    if (order.user_code > 0 && order.user_code < 1000)
    {
        if (order.orderid == 0)
        {
            assert(order.action == 1);
            last_order_id++;
            order.orderid = last_order_id;
        }
    }
    else
    {
        order.orderid *= ORDERID_MULT;
    }

    if (order.action == 1 && order.orderid > last_order_id)
    {
        last_order_id = order.orderid + 1;
    }

    if (order.action == 0)
    {

        int erase_error = eraseOrder(order);
        if (IsSimulatedUser(order))
        {
            if (erase_error == -1)
            {
                //not found
                struct CancelReply reply = {.ext_id = order.ext_id, .orderid = order.orderid, .code = ORDER_NOT_FOUND};
                int msg_type = CANCEL_REPLY_MSG;
                out.write((char *)&msg_type, sizeof(msg_type));
                out.write((char *)&reply, sizeof(reply));
            }
            else
            {
                //found
                struct CancelReply reply = {.ext_id = order.ext_id, .orderid = order.orderid, .code = 0};
                std::cout << reply;
                int msg_type = CANCEL_REPLY_MSG;
                out.write((char *)&msg_type, sizeof(msg_type));
                out.write((char *)&reply, sizeof(reply));
            }
        }
        //printf("erased order\n");
        return;
    }

    if (order.dir == 1)
    {
        int remainingAmount = order.amount;
        int status = 0;
        typename SellSet::iterator head;

        std::vector<std::pair<Trade, Trade>> trades;

        while ((head = sellOrders.begin()) != sellOrders.end() && order.price >= head->price)
        {

            if (order.user_code > 0 && order.user_code == head->user_code)
            {
                status = 1;
                break;
            }

            if (head->amount > remainingAmount)
            {
                struct Trade taker = {.orderid = order.orderid, .deal_price = head->price, .amount = remainingAmount, .user_code = order.user_code};
                struct Trade maker = {.orderid = head->orderid, .deal_price = head->price, .amount = remainingAmount, .user_code = head->user_code};
                trades.push_back(std::pair<Trade, Trade>(taker, maker));

                updateSellAmount(head, head->amount - remainingAmount);
                remainingAmount = 0;
                status = 0;

                break;
            }
            else if (head->amount < remainingAmount)
            {
                struct Trade taker = {.orderid = order.orderid, .deal_price = head->price, .amount = head->amount, .user_code = order.user_code};
                struct Trade maker = {.orderid = head->orderid, .deal_price = head->price, .amount = head->amount, .user_code = head->user_code};
                trades.push_back(std::pair<Trade, Trade>(taker, maker));

                remainingAmount -= head->amount;
                eraseSell(head);
            }
            else if (head->amount == remainingAmount)
            {
                struct Trade taker = {.orderid = order.orderid, .deal_price = head->price, .amount = remainingAmount, .user_code = order.user_code};
                struct Trade maker = {.orderid = head->orderid, .deal_price = head->price, .amount = remainingAmount, .user_code = head->user_code};
                trades.push_back(std::pair<Trade, Trade>(taker, maker));

                remainingAmount = 0;
                status = 0;
                eraseSell(head);
                break;
            }
        }

        if (IsSimulatedUser(order))
        {
            if (status == 0)
            {
                struct NewReply reply = {.ext_id = order.ext_id, .orderid = order.orderid, .code = 0};
                std::cout << "[market] SEND NEW_REPLY " << reply << std::endl;
                int msg_type = NEW_REPLY_MSG;
                out.write((char *)&msg_type, sizeof(msg_type));
                out.write((char *)&reply, sizeof(reply));
            }
            else if (status == 1)
            {
                struct NewReply reply = {.ext_id = order.ext_id, .orderid = order.orderid, .code = CROSS_ORDER_ERR};
                int msg_type = NEW_REPLY_MSG;
                out.write((char *)&msg_type, sizeof(msg_type));
                out.write((char *)&reply, sizeof(reply));
            }
        }

        //
        for (auto &i : trades)
        {
            if (i.first.user_code > 0 && i.first.user_code < 1000)
            {
                int msg_type = TRADE_MSG;
                out.write((char *)&msg_type, sizeof(msg_type));
                out.write((char *)&i.first, sizeof(i.first));
            }
            if (i.second.user_code > 0 && i.second.user_code < 1000)
            {
                int msg_type = TRADE_MSG;
                out.write((char *)&msg_type, sizeof(msg_type));
                out.write((char *)&i.second, sizeof(i.second));
            }
        }

        if (remainingAmount > 0 && status == 0)
        {
            order.amount = remainingAmount;
            insertBuyOrder(order);
        }
    }
    else if (order.dir == 2)
    {
        int remainingAmount = order.amount;
        int status = 0;
        typename BuySet::iterator head;
        std::vector<std::pair<Trade, Trade>> trades;

        while ((head = buyOrders.begin()) != buyOrders.end() && order.price <= head->price)
        {
            if (order.user_code > 0 && order.user_code == head->user_code)
            {
                status = 1;
                break;
            }
            //#endif
            if (head->amount > remainingAmount)
            {
                struct Trade taker = {.orderid = order.orderid, .deal_price = head->price, .amount = remainingAmount, .user_code = order.user_code};
                struct Trade maker = {.orderid = head->orderid, .deal_price = head->price, .amount = remainingAmount, .user_code = head->user_code};
                trades.push_back(std::pair<Trade, Trade>(taker, maker));

                updateBuyAmount(head, head->amount - remainingAmount);
                remainingAmount = 0;
                status = 0;
                break;
            }
            else if (head->amount < remainingAmount)
            {
                struct Trade taker = {.orderid = order.orderid, .deal_price = head->price, .amount = head->amount, .user_code = order.user_code};
                struct Trade maker = {.orderid = head->orderid, .deal_price = head->price, .amount = head->amount, .user_code = head->user_code};
                trades.push_back(std::pair<Trade, Trade>(taker, maker));

                remainingAmount -= head->amount;
                eraseBuy(head);
            }
            else if (head->amount == remainingAmount)
            {
                struct Trade taker = {.orderid = order.orderid, .deal_price = head->price, .amount = remainingAmount, .user_code = order.user_code};
                struct Trade maker = {.orderid = head->orderid, .deal_price = head->price, .amount = remainingAmount, .user_code = head->user_code};
                trades.push_back(std::pair<Trade, Trade>(taker, maker));

                remainingAmount = 0;
                status = 0;
                eraseBuy(head);
                break;
            }
        }

        if (IsSimulatedUser(order))
        {
            if (status == 0)
            {
                struct NewReply reply = {.ext_id = order.ext_id, .orderid = order.orderid, .code = 0};
                int msg_type = NEW_REPLY_MSG;
                out.write((char *)&msg_type, sizeof(msg_type));
                out.write((char *)&reply, sizeof(reply));
            }
            else if (status == 1)
            {
                struct NewReply reply = {.ext_id = order.ext_id, .orderid = order.orderid, .code = CROSS_ORDER_ERR};
                int msg_type = NEW_REPLY_MSG;
                out.write((char *)&msg_type, sizeof(msg_type));
                out.write((char *)&reply, sizeof(reply));
            }
        }

        //
        for (auto &i : trades)
        {
            if (i.first.user_code > 0 && i.first.user_code < 1000)
            {
                int msg_type = TRADE_MSG;
                out.write((char *)&msg_type, sizeof(msg_type));
                out.write((char *)&i.first, sizeof(i.first));
            }
            if (i.second.user_code > 0 && i.second.user_code < 1000)
            {
                int msg_type = TRADE_MSG;
                out.write((char *)&msg_type, sizeof(msg_type));
                out.write((char *)&i.second, sizeof(i.second));
            }
        }

        if (remainingAmount > 0 && status == 0)
        {
            order.amount = remainingAmount;
            insertSellOrder(order);
        }
    }
    else
    {
        throw "unknow";
    }
    //printf("END\n");
}

#endif // !__MARKET_H__
