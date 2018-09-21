#ifndef __ORDER_TEST_H__
#define __ORDER_TEST_H__
#include <string>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <gtest/gtest.h>
#include "../src/script_order.h"
#include "testorder.h"

//u price amount
//n extid orderid code
//c extid orderid code amount
//t orderid deal_price amount user_code?
//d do
int ReadMsgType(std::stringstream &stream)
{
    int msg_type;
    stream.read((char *)&msg_type, sizeof(msg_type));
    return msg_type;
}

void ReadNewOrder(std::stringstream &stream, NewOrder &new_order)
{

    stream.read((char *)&new_order, sizeof(new_order));
}

void ReadCancelOrder(std::stringstream &stream, CancelOrder &cancel_order)
{
    stream.read((char *)&cancel_order, sizeof(cancel_order));
}

TEST(ProcessOrder, UpdatePriceAmount)
{

    ScriptOrder order;
    order << "u 1000 1";
    EXPECT_EQ(order.desired_price, 1000);
    EXPECT_EQ(order.desired_amount, 1);
    EXPECT_EQ(order.state.status, FREE);
}

//
TEST(ProcessOrder, UpdatePriceSendNewOrder)
{
    std::stringstream out;
    ScriptOrder order(out);
    order << "u 1000 1";
    order.Do();

    EXPECT_EQ(out.tellp(), sizeof(int) + sizeof(NewOrder));
    int msg_type = ReadMsgType(out);
    EXPECT_EQ(msg_type, NEW_ORDER);
    NewOrder new_order;
    //out.read((char *)&new_order, sizeof(new_order));
    ReadNewOrder(out, new_order);
    EXPECT_EQ(new_order.price, 1000);
    EXPECT_EQ(new_order.amount, 1);
    EXPECT_EQ(order.state.status, PENDING_NEW);
    EXPECT_EQ(order.state.price, 1000);
    EXPECT_EQ(order.state.amount, 1);
    EXPECT_EQ(order.state.remainingAmount, 1);
}

TEST(ProcessOrder, ReceiveNewReplyChangeStatusToNew)
{
    std::stringstream out;
    ScriptOrder order(out);
    order << "u 1000 1";
    order.Do();
    order << "n 11 101 0";

    EXPECT_EQ(order.state.status, NEW);
}

TEST(ProcessOrder, UpdatePriceSendCancel)
{
    std::stringstream out;
    ScriptOrder order(out);
    order << "u 1000 1";
    order.Do();
    order << "n 11 101 0";
    order.Do();
    order << "u 1010 1";
    order.Do();

    EXPECT_EQ(order.state.status, PENDING_CANCEL);

    int msg_type = ReadMsgType(out);
    EXPECT_EQ(msg_type, NEW_ORDER);
    NewOrder new_order;
    ReadNewOrder(out, new_order);

    msg_type = ReadMsgType(out);
    EXPECT_EQ(msg_type, CANCEL_ORDER);
    CancelOrder cancel_order;
    ReadCancelOrder(out, cancel_order);
    EXPECT_EQ(cancel_order.orderid, 101);
}

TEST(ProcessOrder, ReceiveCancelReplyChangeStatusToFree)
{
    std::stringstream out;
    ScriptOrder order(out);
    order << "u 1000 1";
    order.Do();
    order << "n 11 101 0";
    order.Do();
    order << "u 1010 1";
    order.Do();
    order << "c 11 101 0 1";
    //order.Do();

    EXPECT_EQ(order.state.status, FREE);
}

TEST(ProcessOrder, ReceiveCancelReplyChangeStatusToCanceled)
{
    std::stringstream out;
    ScriptOrder order(out);
    order << "u 1000 2";
    order.Do();
    order << "n 11 101 0";
    order.Do();
    order << "u 1010 2";
    order.Do();
    order << "c 11 101 0 1";
    order.Do();

    EXPECT_EQ(order.state.status, CANCELED);
    EXPECT_EQ(order.state.remainingAmount, 1);
}

TEST(ProcessOrder, ReceiveOneTrade)
{
    std::stringstream out;
    ScriptOrder order(out);
    order << "u 1000 10";
    order.Do();
    order << "n 11 101 0";
    order.Do();
    order << "t 101 1000 1";

    EXPECT_EQ(order.state.status, NEW);
    EXPECT_EQ(order.state.remainingAmount, 9);
}

TEST(ProcessOrder, ReceiveManyTrades)
{
    std::stringstream out;
    ScriptOrder order(out);
    order << "u 1000 10";
    order.Do();
    order << "n 11 101 0";
    order.Do();
    order << "t 101 1000 1";
    order << "t 101 1000 2";
    order << "t 101 1000 3";

    EXPECT_EQ(order.state.status, NEW);
    EXPECT_EQ(order.state.remainingAmount, 4);
}

TEST(ProcessOrder, FilledOrder)
{
    std::stringstream out;
    ScriptOrder order(out);
    order << "u 1000 10";
    order.Do();
    order << "n 11 101 0";
    order.Do();
    order << "t 101 1000 1";
    order << "t 101 1000 2";
    order << "t 101 1000 3";
    order << "t 101 1000 4";

    EXPECT_EQ(order.state.status, FREE);
    EXPECT_EQ(order.state.remainingAmount, 0);
}

TEST(ProcessOrder, CancelOrderComplete)
{
    std::stringstream out;
    ScriptOrder order(out);
    order << "u 1000 10";
    order.Do();
    order << "n 11 101 0";
    order.Do();
    order << "t 101 1000 1";
    order << "c 11 101 0 9";

    EXPECT_EQ(order.state.status, FREE);
    EXPECT_EQ(order.state.remainingAmount, 0);
}

TEST(ProcessOrder, CancelOrderNotAllAmountResolved)
{
    std::stringstream out;
    ScriptOrder order(out);
    order << "u 1000 10";
    order.Do();
    order << "n 11 101 0";
    order.Do();
    order << "t 101 1000 1";
    order << "t 101 1000 1";
    order << "c 11 101 0 7";

    EXPECT_EQ(order.state.status, CANCELED);
    EXPECT_EQ(order.state.remainingAmount, 1);
}

TEST(ProcessOrder, AmountResolvedAfterCancelatiion)
{
    std::stringstream out;
    ScriptOrder order(out);
    order << "u 1000 10";
    order.Do();
    order << "n 11 101 0";
    order.Do();
    order << "t 101 1000 1";
    order << "t 101 1000 1";
    order << "c 11 101 0 7";
    order << "t 101 1000 1";

    EXPECT_EQ(order.state.status, FREE);
    EXPECT_EQ(order.state.remainingAmount, 0);
}
#endif