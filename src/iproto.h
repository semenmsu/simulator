#ifndef __IPROTO_H__
#define __IPROTO_H__

#include "stdio.h"
#include <iostream>
#include <string>
#include <sstream>

//from C#          621355968000000000
#define DELTA_TIME 621355968000000000
#define MKS 1000000
#define TICK 10000000

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
#define INSTRUMENT_INFO_REQUEST 201
#define INSTRUMENT_INFO_REPLY 202
#define SET_PROPERTY_MSG 203
#define GET_PROPERTY_MSG 204
#define PROPERTY_VALUE_MSG 205

//system
//#define SETTINGS_REQUEST 100001 //from simulator to strategy
//#define STRING_MSG 100002
//#define SET_PROPERTY_MSG 100003 //to strategy

#define MKT_DATA_L1 10001

#define ORDERID_MULT 1000

//send input to script and get answers + pass timestamps
#define FREE 0
#define PENDING_NEW 1
#define NEW 2
#define PENDING_CANCEL 3
#define CANCELED 4

#define BUY 1
#define SELL 2

//strategy regime
#define STOP 0
#define START 1
#define CLOSE 2
#define HARD_CLOSE 3 //use market order?

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

/*
#define FREE 0
#define PENDING_NEW 1
#define NEW 2
#define PENDING_CANCEL 3
#define CANCELED 4

*/
std::string GetStringOrderStatus(int status)
{
    switch (status)
    {
    case FREE:
        return "FREE";
    case PENDING_NEW:
        return "PENDING_NEW";
    case NEW:
        return "NEW";
    case PENDING_CANCEL:
        return "PENDING_CANCEL";
    case CANCELED:
        return "CANCELED";
    default:
        return "UNKNOWN";
    }
}

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

    NewReply &operator<<(std::string new_reply)
    {
        std::istringstream linestream(new_reply);
        int64_t _ext_id = 0;
        int64_t _orderid;
        int32_t _code;
        linestream >> _ext_id;
        linestream >> _orderid;
        linestream >> _code;
        ext_id = _ext_id;
        orderid = _orderid;
        code = _code;

        return *this;
    }

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

    CancelReply &operator<<(std::string cancel_reply)
    {
        std::istringstream linestream(cancel_reply);

        int64_t _ext_id;
        int64_t _orderid;
        int32_t _code;
        int32_t _amount;
        linestream >> _ext_id;
        linestream >> _orderid;
        linestream >> _code;
        linestream >> _amount;
        ext_id = _ext_id;
        orderid = _orderid;
        code = _code;
        amount = _amount;

        return *this;
    }

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

    Trade &operator<<(std::string trade)
    {
        std::istringstream linestream(trade);
        int64_t _orderid;
        int64_t _deal_price;
        int32_t _amount;
        int32_t _user_code;
        linestream >> _orderid;
        linestream >> _deal_price;
        linestream >> _amount;
        linestream >> _user_code;
        orderid = _orderid;
        deal_price = _deal_price;
        amount = _amount;
        user_code = _user_code;

        return *this;
    }

} __attribute__((packed, aligned(4)));

struct MktDataL1
{
    int32_t isin_id;
    int64_t bid;
    int64_t ask;
    int32_t is_ready = 0;
    MktDataL1() : bid(0), ask(0), is_ready(0)
    {
    }
} __attribute__((packed, aligned(4)));

struct InstrumentInfoRequest
{
    int64_t ext_id;
    char symbol[32];
} __attribute__((packed, aligned(4)));

struct InstrumentInfoReply
{
    int64_t ext_id;
    int32_t isin_id;
    char symbol[32];
    int64_t min_step_price;
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