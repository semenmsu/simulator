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

#define MAX_DEPTH 10

#define USER_CODE

#define ORDER_NOT_FOUND 14
#define CROSS_ORDER_ERR 31 //
#define PRICE_OUTSIDE_LIMIT 32
#define INPUT_PARAM_ERR 35
#define PRICE_STEP_ERR 39
#define EXCEEDED_TRANSACTION_LIMIT 54

#define TIMESTAMP_MSG 1
#define NEW_REPLY_MSG 2
#define CANCEL_REPLY_MSG 3
#define TRADE_MSG 4

#define MKT_DATA_L1

//for matching only need price, orderid, amount, action

struct BasePipe
{
    virtual BasePipe &operator|(BasePipe &) = 0;
    virtual BasePipe &operator|(std::stringstream &) = 0;
    virtual std::stringstream &In() = 0;
};

//send input to script and get answers + pass timestamps
struct Script : BasePipe
{
    std::stringstream in;
    std::stringstream *out = nullptr;

    void Do()
    {
        //send request to server
        //zmq_send(socket, data, sizeof(ts), 0);
        //char buffer[64];
        //int sz = zmq_recv(socket, buffer, 64, 0);
        //std::cout << "zmq_recv " << sz << std::endl;
    }

    BasePipe &operator|(BasePipe &to) override
    {
        std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
        //out = &to.In();
        //Do();
        //to.In() << in.rdbuf();
        /*
        std::cout << "Pipeline script " << std::endl;
        int msg_type;
        in.read((char *)&msg_type, sizeof(msg_type));
        if (msg_type == TIMESTAMP_MSG)
        {
            int64_t ts;
            in.read((char *)&ts, sizeof(ts));
            std::cout << "timestamp " << ts << std::endl;
        }*/
        return to;
    }

    BasePipe &operator|(std::stringstream &stream)
    {
        std::cout << "Pipeline script ----- in = " << in.tellp() << std::endl;
        int msg_type;
        in.read((char *)&msg_type, sizeof(msg_type));
        if (msg_type == TIMESTAMP_MSG)
        {
            int64_t ts;
            in.read((char *)&ts, sizeof(ts));
            std::cout << ":::::::::::::::::::::::timestamp " << ts << std::endl;

            stream.write((char *)&msg_type, sizeof(msg_type));
            stream.write((char *)&ts, sizeof(ts));
        }
        return *this;
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

struct NewReply
{
    int64_t ext_id;
    int64_t orderid;
    int32_t code;
} __attribute__((packed, aligned(4)));

struct CancelReply
{
    int64_t ext_id;
    int64_t orderid;
    int32_t code;
};

struct Trade
{
    int64_t orderid;
    int64_t deal_price;
    int32_t amount;
    int32_t user_code;
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
    int64_t ts;
    Market(){};
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

    //handle script
    BasePipe &operator|(BasePipe &to) override
    {
        //----------------------------------------
        if (in.tellg() < in.tellp())
        {
            int msg_type;
            in.read((char *)&msg_type, sizeof(msg_type));
            if (msg_type == TIMESTAMP_MSG)
            {
                int64_t ts;
                in.read((char *)&ts, sizeof(ts));
                std::cout << "@@@@@@@@@@@@@@@@@@@ MARKET RECEIVE timestamp " << ts << std::endl;
            }
        }

        //---------------------------------
        std::cout << "operator | ts " << ts << std::endl;
        //out = &to.In();
        //Do();
        int msg_type = TIMESTAMP_MSG;
        to.In().write((char *)&msg_type, sizeof(msg_type));
        to.In().write((char *)&ts, sizeof(ts));
        std::cout << "WRITE TO SCRIPT :::::::::::::::::::::::::::::::" << std::endl;
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
