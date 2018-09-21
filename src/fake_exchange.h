#ifndef __FAKE_EXCHANGE_H__
#define __FAKE_EXCHANGE_H__
#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include "script_broker.h"
#include "iproto.h"

class FakeExchange : BasePipe
{

  public:
    int64_t bid = 1000000;
    int64_t ask = 1000001;
    std::stringstream out;
    std::stringstream in;
    int64_t ts;
    FakeExchange()
    {
        srand(time(NULL));
    }

    void WriteMarketDataL1()
    {
        //not found
        struct MktDataL1 mkt_data_l1;
        mkt_data_l1.isin_id = 1;
        mkt_data_l1.bid = bid;
        mkt_data_l1.ask = ask;
        mkt_data_l1.is_ready = 1;
        int msg_type = MKT_DATA_L1;
        out.write((char *)&msg_type, sizeof(msg_type));
        out.write((char *)&mkt_data_l1, sizeof(mkt_data_l1));
    }

    void Generate()
    {
        bid = bid + rand() % 3 - 1;
        ask = bid + rand() % 3 + 1;
        printf("bid = %ld, ask = %ld\n", bid, ask);
    }

    //handle script !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    BasePipe &operator|(BasePipe &to)
    {
        /*
        std::cout << "[market-start] mkt.in.tellp = " << in.tellp() << " mkt.in.tellg = " << in.tellg() << std::endl;
        ReadOrderFile();
        std::cout << "ReadInputStream \n";
        ReadInputStream();
        WriteTimeStamp(to.In());

        std::cout << "[market-before-copy] script.in.tellp = " << to.In().tellp() << " script.in.tellg = " << to.In().tellg() << std::endl;
        if (out.tellp() > out.tellg())
        {
            to.In() << out.rdbuf(); //copy
        }

        std::cout << "[market-after-copy] script.in.tellp = " << to.In().tellp() << " script.in.tellg = " << to.In().tellg() << std::endl;*/

        Generate();
        WriteMarketDataL1();

        if (out.tellp() > out.tellg())
        {
            to.In() << out.rdbuf(); //copy
        }
        //out.seekg(0, std::ios::beg);
        //out.seekp(0, std::ios::beg);

        return to;
    }

    BasePipe &operator|(std::stringstream &stream)
    {
        return *this;
    }

    friend FakeExchange &operator>>(int ts, FakeExchange &exchange)
    {
        exchange.ts = ts;
        return exchange;
    }

    std::stringstream &In() override
    {
        return in;
    };
};

#endif // !__FAKE_EXCHANGE_H__