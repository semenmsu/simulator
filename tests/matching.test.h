#ifndef __MATCHING_TEST__
#define __MATCHING_TEST__

#include <string>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <gtest/gtest.h>
#include "../src/market.h"
#include "testorder.h"

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
