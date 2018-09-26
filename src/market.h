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
#include <functional>
#include "../include/reader.h"
#include "iproto.h"
#include "script.h"
#include "../tests/testorder.h"
#include "db_resolver.h"

#define MAX_DEPTH 10

#define USER_CODE

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
    std::stringstream *out;
    std::stringstream in;
    Reader<FortsFutOrderBook> *reader;
    int64_t ts;
    int64_t last_order_id;
    bool eof = false;
    int isin_id = 1;
    int64_t min_step_price = 1;
    int64_t next_time = 0;
    std::function<int(T &, int64_t)> read_func;
    std::function<int64_t(void)> get_next_time;
    Market()
    {
        out = new std::stringstream();
    };

    Market(Reader<FortsFutOrderBook> &rdr)
    {
        reader = &rdr;
        out = new std::stringstream();
    }

    Market(Reader<FortsFutOrderBook> *rdr)
    {
        reader = rdr;
        out = new std::stringstream();
    }

    Market(Reader<FortsFutOrderBook> *rdr, std::stringstream *out_stream)
    {
        reader = rdr;
        out = out_stream;
        //std::cout << "############## Market constructor" << std::endl;
        isin_id = reader->Isin();
        //getchar();
    }

    void *super_reader;
    //Reader<FortsFutOrderBook> *reader2;
    Market(SymbolSettings &settings, std::stringstream *out_stream)
    {
        std::string path = DataPathResolver.ResolveDb(settings.symbol, settings.date);
        std::string settings_path = DataPathResolver.ResolveSettings(settings.symbol, settings.date);
        out = out_stream;

        Reader<FortsFutOrderBook> *rdr = new Reader<FortsFutOrderBook>(settings.symbol, path, settings_path);
        //reader2 = new Reader<FortsFutOrderBook>(settings.symbol, path, settings_path);

        isin_id = rdr->Isin();
        min_step_price = rdr->MinStep();
        //std::cout << "isin_id " << isin_id << std::endl;

        read_func = [=](T &t, int64_t moment) {
            return rdr->Read(t, moment);
        };
        get_next_time = [=]() {
            return rdr->GetNextTimeStamp();
        };
    }

    int32_t Isin()
    {
        return isin_id;
    }

    int64_t MinStep()
    {
        return min_step_price;
    }

    Market(std::string order_book)
    {
        out = new std::stringstream();
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
        //std::cout << "[market] RESET IN \n";
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

        in.read((char *)&ts, sizeof(ts));
        //std::cout << "[market] read timestamp " << ts << std::endl;
        //std::cout << ":::::::::::::::::::::::timestamp " << ts << std::endl;
    }

    void ReadNewOrder()
    {

        NewOrder new_order;
        in.read((char *)&new_order, sizeof(new_order));
    }

    void ReadNewOrder(NewOrder &new_order)
    {
        T t;
        /*int64_t ts;
    int32_t user_code;
    int32_t isin_id;
    int64_t ext_id;
    int64_t price;
    int32_t amount;
    int32_t dir;*/
        //????
        if (ts < new_order.ts)
        {
            ts = new_order.ts;
        }

        t.action = 1;
        t.dir = (int)new_order.dir;
        t.orderid = 0;
        t.price = new_order.price;
        t.amount = new_order.amount;
        t.ext_id = new_order.ext_id;
        t.user_code = new_order.user_code;
        //std::cout << "[market   ]--> new " << ts << std::endl;
        //std::cout << "[script] READ NEW ORDER ext_id " << t.ext_id << std::endl;
        //std::cout << "[script] " << new_order;
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
        ReadCancelOrder(cancel_order);
    }

    void ReadCancelOrder(CancelOrder &cancel_order)
    {
        if (ts < cancel_order.ts)
        {
            ts = cancel_order.ts;
        }
        T t;
        t.action = 0;
        t.orderid = cancel_order.orderid;
        t.user_code = cancel_order.user_code;
        //std::cout << "[script] " << cancel_order;
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
                //std::cout << "[market] EMPTY\n";
                ResetIn();
                break;
            case TIMESTAMP_MSG:
                //std::cout << "[market] TIMESTAMP_MSG\n";
                ReadTimeStamp();
                break;
            case NEW_ORDER:
                //std::cout << "[market] NEW_ORDER\n";
                ReadNewOrder();
                break;
            case CANCEL_ORDER:
                //std::cout << "[market] CANCEL_ORDER\n";
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
        //struct MktDataL1 mkt_data_l1 = {.isin_id = 1, .bid = bid, .ask = ask};
        struct MktDataL1 mkt_data_l1;
        mkt_data_l1.isin_id = isin_id;
        mkt_data_l1.bid = bid;
        mkt_data_l1.ask = ask;
        int msg_type = MKT_DATA_L1;
        out->write((char *)&msg_type, sizeof(msg_type));
        out->write((char *)&mkt_data_l1, sizeof(mkt_data_l1));
        //std::cout << "[market] $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ WRITE_MARKET_DATA isin_id" << isin_id << std::endl;
    }

    void ReadOrderFile()
    {
        int count = 0;
        while (count++ < 1000)
        {
            T order;
            int res = reader->Read(order);
            if (res == -1)
            {
                eof = true;
                break;
            }

            getchar();
            PlaceOrder(order);
        }
        //Print();

        //std::cout << GetBid() << " | " << GetAsk() << std::endl;
        WriteMarketDataL1(GetBid(), GetAsk());
        //getchar();
    }

    int64_t ReadOrderFile(int64_t time_boundary)
    {
        ts = time_boundary;
        int64_t moment = time_boundary;

        int64_t total_records = 0;
        //int64_t next_time = 0;
        while (true)
        {
            T order;
            //int64_t res = reader->Read(order, moment);
            //std::cout << "try read" << std::endl;
            int64_t res = read_func(order, moment);
            //std::cout << order << std::endl;
            //std::cout << "stop read" << std::endl;
            if (res == -1)
            {
                eof = true;
                break;
            }
            else if (res == 0)
            {
                break; //end time
            }
            next_time = res;
            total_records++;
            PlaceOrder(order);
        }

        if (total_records > 0)
        {
            WriteMarketDataL1(GetBid(), GetAsk());
        }

        return next_time;
    }

    int64_t GetNextTimeStamp()
    {
        //return reader->GetNextTimeStamp();
        return get_next_time();
    }

    void WriteTimeStamp(std::stringstream &stream)
    {
        int msg_type = TIMESTAMP_MSG;
        stream.write((char *)&msg_type, sizeof(msg_type));
        stream.write((char *)&ts, sizeof(ts));
        //std::cout << "WRITE TO SCRIPT :::::::::::::::::::::::::::::::" << std::endl;
    }

    //handle script !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    BasePipe &operator|(BasePipe &to) override
    {
        //std::cout << "[market-start] mkt.in.tellp = " << in.tellp() << " mkt.in.tellg = " << in.tellg() << std::endl;
        ReadOrderFile();
        //std::cout << "ReadInputStream \n";
        ReadInputStream();
        WriteTimeStamp(to.In());

        //std::cout << "[market-before-copy] script.in.tellp = " << to.In().tellp() << " script.in.tellg = " << to.In().tellg() << std::endl;
        if (out->tellp() > out->tellg())
        {
            to.In() << out->rdbuf(); //copy
        }

        //std::cout << "[market-after-copy] script.in.tellp = " << to.In().tellp() << " script.in.tellg = " << to.In().tellg() << std::endl;
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
        //market.ts = ts;
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

    int eraseOrder(const T &order, int *amount)
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
            *amount = buy_iter->amount;
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
            *amount = sell_iter->amount;
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
        int remaining_amount;
        eraseOrder(*order, &remaining_amount);
        insertSellOrder(copy);
    }

    void updateBuyAmount(typename BuySet::iterator &order, int new_amount)
    {
        T copy = *order;
        copy.amount = new_amount;
        //eraseBuy(order);
        int remaining_amount;
        eraseOrder(*order, &remaining_amount);
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
        int remaining_amount = 0;
        int erase_error = eraseOrder(order, &remaining_amount);
        if (IsSimulatedUser(order))
        {
            if (erase_error == -1)
            {
                //not found
                struct CancelReply reply = {.ext_id = order.ext_id, .orderid = order.orderid, .code = ORDER_NOT_FOUND, .amount = 0};
                reply.ts = ts;
                int msg_type = CANCEL_REPLY_MSG;
                out->write((char *)&msg_type, sizeof(msg_type));
                out->write((char *)&reply, sizeof(reply));
            }
            else
            {
                //found
                struct CancelReply reply = {.ext_id = order.ext_id, .orderid = order.orderid, .code = 0, .amount = remaining_amount};
                reply.ts = ts;
                //std::cout << reply;
                int msg_type = CANCEL_REPLY_MSG;
                out->write((char *)&msg_type, sizeof(msg_type));
                out->write((char *)&reply, sizeof(reply));
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
                reply.ts = ts;

                int msg_type = NEW_REPLY_MSG;
                out->write((char *)&msg_type, sizeof(msg_type));
                out->write((char *)&reply, sizeof(reply));
            }
            else if (status == 1)
            {
                struct NewReply reply = {.ext_id = order.ext_id, .orderid = order.orderid, .code = CROSS_ORDER_ERR};
                reply.ts = ts;
                int msg_type = NEW_REPLY_MSG;
                out->write((char *)&msg_type, sizeof(msg_type));
                out->write((char *)&reply, sizeof(reply));
            }
        }

        //
        for (auto &i : trades)
        {
            if (i.first.user_code > 0 && i.first.user_code < 1000)
            {
                int msg_type = TRADE_MSG;
                i.first.ts = ts;
                i.first.isin_id = isin_id;
                i.first.dir = 1;
                out->write((char *)&msg_type, sizeof(msg_type));
                out->write((char *)&i.first, sizeof(i.first));
            }
            if (i.second.user_code > 0 && i.second.user_code < 1000)
            {
                i.second.ts = ts;
                i.second.isin_id = isin_id;
                i.second.dir = 2;
                int msg_type = TRADE_MSG;
                out->write((char *)&msg_type, sizeof(msg_type));
                out->write((char *)&i.second, sizeof(i.second));
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
                reply.ts = ts;
                int msg_type = NEW_REPLY_MSG;
                out->write((char *)&msg_type, sizeof(msg_type));
                out->write((char *)&reply, sizeof(reply));
            }
            else if (status == 1)
            {
                struct NewReply reply = {.ext_id = order.ext_id, .orderid = order.orderid, .code = CROSS_ORDER_ERR};
                reply.ts = ts;
                int msg_type = NEW_REPLY_MSG;
                out->write((char *)&msg_type, sizeof(msg_type));
                out->write((char *)&reply, sizeof(reply));
            }
        }

        //
        for (auto &i : trades)
        {
            if (i.first.user_code > 0 && i.first.user_code < 1000)
            {
                int msg_type = TRADE_MSG;
                i.first.ts = ts;
                i.first.isin_id = isin_id;
                i.first.dir = 2;
                out->write((char *)&msg_type, sizeof(msg_type));
                out->write((char *)&i.first, sizeof(i.first));
            }
            if (i.second.user_code > 0 && i.second.user_code < 1000)
            {
                i.second.ts = ts;
                i.second.isin_id = isin_id;
                i.second.dir = 1;
                int msg_type = TRADE_MSG;
                out->write((char *)&msg_type, sizeof(msg_type));
                out->write((char *)&i.second, sizeof(i.second));
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
