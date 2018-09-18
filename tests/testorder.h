#ifndef __TEST_ORDER__
#define __TEST_ORDER__

#include <string>
#include <sstream>
#include <iostream>

#include <stdio.h>

struct Order
{
    int64_t orderid;
    int64_t price;
    int64_t ext_id;
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
        int64_t _ext_id;
        linestream >> _price;
        linestream >> _amount;
        linestream >> _dir;
        linestream >> _orderid;
        linestream >> _user_code;
        linestream >> _ext_id;
        price = _price;
        orderid = _orderid;
        amount = _amount;
        dir = (char)_dir;
        action = 1;
        user_code = _user_code;
        ext_id = _ext_id;

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

#endif // !__TEST_ORDER__
