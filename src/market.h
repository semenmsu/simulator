#include <stdio.h>
#include <set>
#include <unordered_map>
#include <iostream>
#include <assert.h>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

#define MAX_DEPTH 10

#define USER_CODE

#define CONTR_ORDER_ERR 31 //

//for matching only need price, orderid, amount, action

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
struct MoreComparator
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

struct Reply
{
    int64_t price;
    int32_t amount;
};

template <typename T>
class Market
{
    typedef typename std::set<T, IdComparator<T>> OrderSet;
    typedef typename std::set<T, MoreComparator<T>> BuySet;
    typedef typename std::set<T, LessComparator<T>> SellSet;

    typedef typename std::unordered_map<std::pair<int32_t, int64_t>, T> Ccid2Order; //cliet client id

    //typedef typename std::unordered_map<int64_t, int64_t> Id2Cid;
    //typedef typename std::unordered_map<int64_t, int64_t> Cid2Id;

    OrderSet orders;
    BuySet buyOrders;
    SellSet sellOrders;

  public:
    Market(){};
    void PlaceOrder(T &order);

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

    void eraseOrder(const T &order)
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
            return;
        }
        if (it->dir == 1)
        {
            T t;
            t.orderid = it->orderid;
            t.price = it->price;
            typename BuySet::iterator buy_iter = buyOrders.find(t);
            //typename BuySet::iterator buy_iter = buyOrders.find({.orderid = it->orderid, .price = it->price});
            eraseBuy(buy_iter);
        }
        else
        {
            T t;
            t.orderid = it->orderid;
            t.price = it->price;
            typename SellSet::iterator sell_iter = sellOrders.find(t);
            //typename SellSet::iterator sell_iter = sellOrders.find({.orderid = it->orderid, .price = it->price});
            eraseSell(sell_iter);
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
};

template <typename T>
void Market<T>::PlaceOrder(T &order)
{
    //printf("placeorder\n");
    //std::cout << order;
    if (order.action == 0)
    {
        //printf("try erase order\n");
        eraseOrder(order);
        //printf("erased order\n");
        return;
    }
    if (order.dir == 1)
    {
        int remainingAmount = order.amount;
        int status = 0;
        typename SellSet::iterator head;
        std::vector<Reply> trades(10);
        //int user_code = order.user_code;

        while ((head = sellOrders.begin()) != sellOrders.end() && order.price >= head->price)
        {

            if (order.user_code > 0 && order.user_code == head->user_code)
            {
                status = 1;
                break;
            }

            if (head->amount > remainingAmount)
            {

                updateSellAmount(head, head->amount - remainingAmount);
                remainingAmount = 0;
                status = 0;
                break;
            }
            else if (head->amount < remainingAmount)
            {
                remainingAmount -= head->amount;
                eraseSell(head);
            }
            else if (head->amount == remainingAmount)
            {
                remainingAmount = 0;
                status = 0;
                eraseSell(head);
                break;
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
                updateBuyAmount(head, head->amount - remainingAmount);
                remainingAmount = 0;
                status = 0;
                break;
            }
            else if (head->amount < remainingAmount)
            {
                remainingAmount -= head->amount;
                eraseBuy(head);
            }
            else if (head->amount == remainingAmount)
            {
                remainingAmount = 0;
                status = 0;
                eraseBuy(head);
                break;
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
