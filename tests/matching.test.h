#ifndef __MATCHING_TEST__
#define __MATCHING_TEST__

#include <string>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <gtest/gtest.h>
#include "../src/market.h"
#include "testorder.h"

TEST(MatchingTest, AddBuyMakerOrder)
{

    Market<Order> mkt(R"(
        12 1 2 31
        11 1 2 21
        10 1 2 11 
        
        9 1 1 1 
        8 1 1 2 
        7 1 1 3 2
        )");
    mkt << "6 1 1 41";
    Market<Order> mkt_expect(R"(
        12 1 2 31
        11 1 2 21
        10 1 2 11 

        9 1 1 1 
        8 1 1 2 
        7 1 1 3 2
        6 1 1 41
        )");
    EXPECT_EQ(1, mkt == mkt_expect);
}

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

TEST(User, AddOrderReceiveNewReply)
{
    Market<Order> mkt(R"(
        12 1 2 31
        11 1 2 21
        10 1 2 11 
        
        9 1 1 1 
        8 1 1 2 
        7 1 1 3 2 101
        )");

    int msg_type;
    mkt.out.read((char *)&msg_type, sizeof(msg_type));
    EXPECT_EQ(msg_type, NEW_REPLY_MSG);

    NewReply newReply;
    mkt.out.read((char *)&newReply, sizeof(newReply));
    EXPECT_EQ(newReply.ext_id, 101);
    EXPECT_EQ(mkt.out.tellp(), sizeof(newReply) + sizeof(msg_type));
    EXPECT_EQ(mkt.out.tellg(), mkt.out.tellp());
}

TEST(User, AddBuyTakerReceiveNewReplyAndOneTrade)
{
    Market<Order> mkt(R"(
        16 1 2 31
        15 1 2 21
        10 1 2 11 
        
        9 1 1 1 
        8 1 1 2 
        7 1 1 3
        )");
    mkt << "11 2 1 555 77 101";

    //new order
    int msg_type;
    mkt.out.read((char *)&msg_type, sizeof(msg_type));
    EXPECT_EQ(msg_type, NEW_REPLY_MSG);

    NewReply newReply;
    mkt.out.read((char *)&newReply, sizeof(newReply));
    EXPECT_EQ(newReply.ext_id, 101);
    EXPECT_EQ(newReply.code, 0);

    //trade
    mkt.out.read((char *)&msg_type, sizeof(msg_type));
    EXPECT_EQ(msg_type, TRADE_MSG);

    Trade newTrade;
    mkt.out.read((char *)&newTrade, sizeof(newTrade));
    EXPECT_EQ(newTrade.amount, 1);
    EXPECT_EQ(newTrade.user_code, 77);
    EXPECT_EQ(newTrade.deal_price, 10);

    //end
    EXPECT_EQ(mkt.out.tellg(), mkt.out.tellp());
}

TEST(User, AddBuyTakerReceiveNewReplyAndTwoTrades)
{
    Market<Order> mkt(R"(
        16 1 2 31
        15 1 2 21
        11 1 2 15
        10 1 2 11 
        
        9 1 1 1 
        8 1 1 2 
        7 1 1 3
        )");
    mkt << "12 10 1 555 77 101";

    //new order
    int msg_type;
    mkt.out.read((char *)&msg_type, sizeof(msg_type));
    EXPECT_EQ(msg_type, NEW_REPLY_MSG);

    NewReply newReply;
    mkt.out.read((char *)&newReply, sizeof(newReply));
    EXPECT_EQ(newReply.ext_id, 101);
    EXPECT_EQ(newReply.code, 0);

    //trade
    mkt.out.read((char *)&msg_type, sizeof(msg_type));
    EXPECT_EQ(msg_type, TRADE_MSG);

    Trade newTrade;
    mkt.out.read((char *)&newTrade, sizeof(newTrade));
    EXPECT_EQ(newTrade.amount, 1);
    EXPECT_EQ(newTrade.user_code, 77);
    EXPECT_EQ(newTrade.deal_price, 10);

    //trade
    mkt.out.read((char *)&msg_type, sizeof(msg_type));
    EXPECT_EQ(msg_type, TRADE_MSG);

    Trade newTrade2;
    mkt.out.read((char *)&newTrade2, sizeof(newTrade2));
    EXPECT_EQ(newTrade2.amount, 1);
    EXPECT_EQ(newTrade2.user_code, 77);
    EXPECT_EQ(newTrade2.deal_price, 11);

    //end
    EXPECT_EQ(mkt.out.tellg(), mkt.out.tellp());
}

#endif // !__MATCHING_TEST__
