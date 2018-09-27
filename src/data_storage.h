#ifndef __DATA_STORAGE_H__
#define __DATA_STORAGE_H__

#include <unordered_map>
#include "iproto.h"
struct DataStorage
{
    std::unordered_map<std::string, MktDataL1> symbol2BestBidAsk;
    std::unordered_map<int32_t, MktDataL1 *> l1;

    void UpdateL1(MktDataL1 &data)
    {
        /*
        std::cout << "try update" << std::endl;
        MktDataL1 *quote = &l1[data.isin_id];
        quote->bid = data.bid;
        quote->ask = data.ask;
        quote->is_ready = 1;*/
        //std::cout << "Start Update L1" << std::endl;
        MktDataL1 *quote = l1[data.isin_id];
        quote->bid = data.bid;
        quote->ask = data.ask;
        quote->is_ready = 1;
        //std::cout << "Update L1 " << data.isin_id << " " << data.bid << " " << data.ask << std::endl;
        //getchar();
    }

    MktDataL1 &RegisterL1(MktDataL1 &data)
    {
        /*MktDataL1 &quote = l1[data.isin_id];
        quote.isin_id = data.isin_id;
        quote.bid = 0;
        quote.ask = 0;
        quote.is_ready = 0;
        return quote;*/
        //MktDataL1 l1;
        //return l1;
        return data;
    }

    MktDataL1 &RegisterL1(std::string symbol)
    {
        if (symbol2BestBidAsk.count(symbol) == 0)
        {
            MktDataL1 l1;
            symbol2BestBidAsk[symbol] = l1;
            return symbol2BestBidAsk[symbol];
            //symbol2BestBidAsk.insert(std::pair<std::string, MktDataL1>(symbol))
        }
        else
        {
            return symbol2BestBidAsk[symbol];
        }
    }

    //write to out all request for marketdata
    //ext_id = 0, this is for data storage
    void RequestInstrumentInfo(std::stringstream &out)
    {
        for (auto &kvp : symbol2BestBidAsk)
        {
            InstrumentInfoRequest info_request;
            std::string symbol = kvp.first;
            symbol.copy(info_request.symbol, symbol.length());
            info_request.symbol[symbol.length()] = '\0';
            info_request.ext_id = 0;
            int msg_type = INSTRUMENT_INFO_REQUEST;
            out.write((char *)&msg_type, sizeof(msg_type));
            out.write((char *)&info_request, sizeof(info_request));
        }
    }

    void ReplyInstrumentInfo(InstrumentInfoReply info_reply)
    {
        std::string symbol(info_reply.symbol);
        //std::cout << "[data-storeage] ReplyInstrumentInfo " << symbol << std::endl;
        if (symbol2BestBidAsk.count(symbol) == 0)
        {
            MktDataL1 bidask;
            bidask.isin_id = info_reply.isin_id;

            symbol2BestBidAsk[symbol] = bidask;
            bidask.min_step_price = info_reply.min_step_price;

            if (l1.count(info_reply.isin_id) == 0)
            {
                l1[info_reply.isin_id] = &symbol2BestBidAsk[symbol];
            }
        }
        else
        {
            symbol2BestBidAsk[symbol].isin_id = info_reply.isin_id;
            symbol2BestBidAsk[symbol].min_step_price = info_reply.min_step_price;
            if (l1.count(info_reply.isin_id) == 0)
            {
                l1[info_reply.isin_id] = &symbol2BestBidAsk[symbol];
            }
        }
        //getchar();
    }
};
#endif