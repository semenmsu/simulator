#ifndef __MATCHING_TEST__
#define __MATCHING_TEST__

#include <gtest/gtest.h>
#include "../src/market.h"
#include <string>
#include <sstream>
#include <iostream>

#include <stdio.h>
struct Order
{
    int64_t orderid;
    int64_t price;
    int32_t amount;
    int32_t user_code; //16
    char action;
    char dir;
    char __padding[2];

    Order(){};
    Order(std::string str)
    {
        this->operator<<(str);
    }

    friend std::ostream &operator<<(std::ostream &stream, const Order &order)
    {
        stream << order.price << " " << order.amount << " "
               << (int)order.dir << " " << order.orderid << " " << order.user_code << '\n';
        return stream;
    }

    Order &operator<<(std::string order)
    {
        std::istringstream linestream(order);
        int64_t _price;
        int64_t _orderid;
        int32_t _amount;
        int32_t _dir;
        int32_t _user_code = 0;
        linestream >> _price;
        linestream >> _amount;
        linestream >> _dir;
        linestream >> _orderid;
        linestream >> _user_code;
        price = _price;
        orderid = _orderid;
        amount = _amount;
        dir = (char)_dir;
        action = 1;
        user_code = _user_code;

        return *this;
    }

    bool operator!=(const Order &order)
    {
        //printf("call operator != \n");
        if (orderid != order.orderid)
        {
            return true;
        }
        if (price != order.price)
        {
            return true;
        }
        if (amount != order.amount)
        {
            return true;
        }
        if (user_code != order.user_code)
        {
            return true;
        }
        if (action != order.action)
        {
            return true;
        }
        if (dir != order.dir)
        {
            return true;
        }
        return false;
    }
} __attribute__((packed, aligned(4)));

TEST(MatchingTest, AddBuyTakerOrderSingleMatching)
{
    Market<Order> mkt(R"(
        12 1 2 31
        11 1 2 21
        10 1 2 11 
        
        9 1 1 1 
        8 1 1 2 
        7 1 1 3 2
        )");
    mkt << "10 2 1 4 2";
    Market<Order> mkt_expect(R"(
        12 1 2 31
        11 1 2 21
        
        10 1 1 4 2
        9 1 1 1 
        8 1 1 2 
        7 1 1 3 2
        )");
    EXPECT_EQ(1, mkt == mkt_expect);
}

TEST(MatchingTest, AddBuyTakerOrderMultiMatching)
{

    Market<Order> mkt(R"(
        12 1 2 31
        11 1 2 21
        10 1 2 11 
        
        9 1 1 1 
        8 1 1 2 
        7 1 1 3 2
        )");
    mkt << "11 5 1 4 2";
    Market<Order> mkt_expect(R"(
        12 1 2 31
        
        11 3 1 4 2
        9 1 1 1 
        8 1 1 2 
        7 1 1 3 2
        )");
    EXPECT_EQ(1, mkt == mkt_expect);
}

#endif // !__MATCHING_TEST__
