#ifndef __IPROTO_H__
#define __IPROTO_H__

#include "stdio.h"
#include <iostream>

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

//send input to script and get answers + pass timestamps
#define FREE 0
#define PENDING_NEW 1
#define NEW 2
#define PENDING_CANCEL 3
#define CANCELED 4

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

#endif