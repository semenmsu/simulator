#ifndef __SIMULATOR_H__
#define __SIMULATOR_H__

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
//include <time.h>
#include <ctime>
#include <sstream>

#include "../include/json.hpp"
#include "market.h"
#include "../tests/testorder.h"
#include "../include/reader.h"

struct SymbolSettings
{
    std::string symbol;
    std::string date; //YYYYMMdd
    std::string exchange;
    std::string data_source;

    std::string start_date;
    std::string stop_date;
};

struct DataPathResolver
{
    static std::string ResolveExchange(std::string symbol)
    {
        if (symbol == "Si-16.12")
        {
            return "FORTS";
        }

        return "FORTS";
    }
    static std::string ResolveDb(std::string symbol, std::string date)
    {
        /*std::stringstream path;
        std::string exchange = ResolveExchange(symbol);
        path << "/home/soarix/Data";
        path << "/" + exchange;
        path << "/" + date;
        std::string str = path.str();
        //std::cout << str << std::endl;
        return str;*/
        std::string db_path = "/home/soarix/Data/FortsOrderBook.db";
        return db_path;
    }

    static std::string ResolveSettings(std::string symbol, std::string date)
    {
        /*std::stringstream path;
        std::string exchange = ResolveExchange(symbol);
        path << "/home/soarix/Data";
        path << "/" + exchange;
        path << "/" + date;
        std::string str = path.str();
        //std::cout << str << std::endl;
        return str;*/
        std::string settings_path = "/home/soarix/Data/FortsOrderBookSettings.json";
        return settings_path;
    }
} DataPathResolver;

//One day simulator
class Simulator : public BasePipe
{

    std::unordered_map<int32_t, Reader *> readers;
    std::unordered_map<int32_t, Market<Order> *> markets;
    std::unordered_map<std::string, Market<Order> *> symbol2market;

  public:
    int64_t ts = 0;
    int64_t start_time = 0;
    int64_t stop_time = 0;
    int64_t current_time = 0;
    std::string date = "";
    std::stringstream in;
    std::stringstream out;
    int32_t eof = 0;

    Simulator() {}

    void AddSymbol(SymbolSettings &settings)
    {
        struct tm tm;
        memset(&tm, 0, sizeof(struct tm));
        const char *time_details = (const char *)settings.date.c_str();
        printf("%s \n", time_details);
        //strptime(time_details, "%Y-%m", &tm);
        strptime(time_details, "%Y%m%d", &tm);

        //std::cout << tm.tm_year + 1900 << " " << tm.tm_mon + 1 << " " << tm.tm_mday << std::endl;

        std::string path = DataPathResolver.ResolveDb(settings.symbol, settings.date);
        std::string settings_path = DataPathResolver.ResolveSettings(settings.symbol, settings.date);

        Reader *reader = new Reader(settings.symbol, path, settings_path);

        readers.insert(std::pair<int32_t, Reader *>(reader->Isin(), reader));

        //Reader reader(settings.symbol, path, settings_path);
        //Market<Order> *mkt = new Market<Order>(reader);
        Market<Order> *mkt = new Market<Order>(reader);
        markets.insert(std::pair<int32_t, Market<Order> *>(reader->Isin(), mkt));

        //std::cout << path << std::endl;

        //time_t t = mktime(&tm);
        //char *dt = ctime(&t);
        //printf("%d \n", t);
        //printf("time = %s \n", dt);
    }

    void RequestAddSymbol(SymbolSettings &settings, InstrumentInfoReply &info_reply)
    {
        if (date == "")
        {
            date = settings.date;
        }
        else
        {
            if (date != settings.date)
            {
                printf("[simulator:RequestAddSymbol]%s date != other date %s \n", date.c_str(), settings.date.c_str());
            }
        }

        struct tm tm;
        memset(&tm, 0, sizeof(struct tm));
        const char *time_details = (const char *)settings.date.c_str();
        printf("%s \n", time_details);
        //strptime(time_details, "%Y-%m", &tm);
        strptime(time_details, "%Y%m%d", &tm);

        tm.tm_hour = 10;
        tm.tm_min = 5;
        tm.tm_sec = 0;

        struct tm tm_stop;
        //tm_stop = tm;
        tm_stop.tm_year = tm.tm_year;
        tm_stop.tm_mday = tm.tm_mday;
        tm_stop.tm_mon = tm.tm_mon;
        tm_stop.tm_hour = 19;
        tm_stop.tm_min = 40;
        tm_stop.tm_sec = 0;

        time_t t = mktime(&tm);
        time_t t_stop = mktime(&tm_stop);

        start_time = t * TICK + DELTA_TIME;
        current_time = start_time;
        stop_time = t_stop * TICK + DELTA_TIME;
        printf("%ld \n", start_time);
        printf("%ld \n", stop_time);
        //getchar();
        char *dt = ctime(&t_stop);
        printf("time = %s \n", dt);
        //getchar();

        //std::cout << tm.tm_year + 1900 << " " << tm.tm_mon + 1 << " " << tm.tm_mday << std::endl;

        std::string path = DataPathResolver.ResolveDb(settings.symbol, settings.date);
        std::string settings_path = DataPathResolver.ResolveSettings(settings.symbol, settings.date);

        if (symbol2market.count(settings.symbol) == 0)
        {
            Reader *reader = new Reader(settings.symbol, path, settings_path);
            readers.insert(std::pair<int32_t, Reader *>(reader->Isin(), reader));
            Market<Order> *mkt = new Market<Order>(reader, &out);
            markets.insert(std::pair<int32_t, Market<Order> *>(reader->Isin(), mkt));
            symbol2market.insert(std::pair<std::string, Market<Order> *>(settings.symbol, mkt));
        }
        //std::cout << "min_step: " << symbol2market[settings.symbol]->reader->MinStep() << std::endl;
        //std::cout << "isin_id: " << symbol2market[settings.symbol]->reader->Isin() << std::endl;
        info_reply.isin_id = symbol2market[settings.symbol]->reader->Isin();
        info_reply.min_step_price = symbol2market[settings.symbol]->reader->MinStep();

        //std::cout << path << std::endl;
    }

    void AddSymbol(std::string symbol)
    {
    }

    void AddSymbol(std::string symbol, std::string settings)
    {
    }

    Simulator &operator<<(std::string symbol_settings)
    {
        std::istringstream linestream(symbol_settings);
        std::string _symbol;
        std::string _date;
        linestream >> _symbol;
        linestream >> _date;
        SymbolSettings settings = {.symbol = _symbol, .date = _date};
        AddSymbol(settings);

        return *this;
    }

    void ReadOrderFile()
    {
        /*
        int count = 0;
        while (count++ < 10)
        {
            T order;
            int res = reader->Read(order);
            if (res == -1)
            {
                eof = true;
                break;
            }
        }*/

        int64_t next_time = 0;
        for (auto i : markets)
        {
            //i.second->ReadOrderFile();
            int64_t _next_time = i.second->ReadOrderFile(current_time); //in C# 1tick = 100nano
            if (_next_time == 0)
            {
                std::cout << "_next_time == 0, this is impossible" << std::endl;
                //current_time += 1000000; //100ms
                //getchar();
                //time_t unix_time = (current_time - DELTA_TIME) / 10000000;

                //char *dt = ctime(&unix_time);
                //std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@TIME " << dt;
            }
            else
            {
                //time_t unix_time = (current_time - DELTA_TIME) / 10000000;

                //char *dt = ctime(&unix_time);
                //std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@TIME " << dt;

                if (current_time > stop_time)
                {
                    time_t unix_time = (stop_time - DELTA_TIME) / 10000000;

                    char *dt = ctime(&unix_time);
                    std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@TIME " << dt;
                    std::cout << "current " << current_time << std::endl;
                    std::cout << "stop" << stop_time << std::endl;
                    eof = 1;
                    return;
                }
                if (next_time == 0)
                {
                    next_time = _next_time;
                }
                else if (next_time > _next_time)
                {
                    next_time = _next_time;
                }
                if (next_time <= current_time)
                {
                    std::cout << "next_time <= current_time, this is wrong" << std::endl;
                    std::cout << "next_time " << next_time << std::endl;
                    std::cout << "current_time" << current_time << std::endl;
                    time_t unix_time = (current_time - DELTA_TIME) / 10000000;

                    char *dt = ctime(&unix_time);
                    std::cout << "TIME " << dt;
                    getchar();
                }

                //std::cout << "next_time " << next_time << std::endl;
                //std::cout << "current_time" << current_time << std::endl;
                //current_time += 10000000; //sec
                current_time += 1000000; //100ms
                //current_time += 100000; //10ms
                //current_time += 1000; //1ms
                current_time = std::max(next_time, current_time);
            }
        }
        //std::cout << current_time << std::endl;

        //getchar();
    }

    int ReadMessageType()
    {
        if (in.tellg() < in.tellp())
        {
            int msg_type;
            in.read((char *)&msg_type, sizeof(msg_type));
            return msg_type;
        }
        else
        {
            return EMPTY;
        }
    }

    void ReadTimeStamp()
    {
        int64_t ists;
        in.read((char *)&ists, sizeof(ists));
        //std::cout << "[simulator] read timestamp " << ists << std::endl;
    }

    void ReadInstrumentInfoRequest()
    {
        InstrumentInfoRequest info_request;
        in.read((char *)&info_request, sizeof(info_request));
        //printf("[simulator] symbol = %s\n", info_request.symbol);

        InstrumentInfoReply info_reply;
        SymbolSettings symbol_settings = {.symbol = info_request.symbol, .date = "20161027"};
        RequestAddSymbol(symbol_settings, info_reply);
        info_reply.ext_id = info_request.ext_id;
        strcpy(info_reply.symbol, info_request.symbol);

        int msg_type = INSTRUMENT_INFO_REPLY;
        out.write((char *)&msg_type, sizeof(msg_type));
        out.write((char *)&info_reply, sizeof(info_reply));
        //std::cout << "put " << out.tellp() << "  get = " << out.tellg() << std::endl;
    }

    void ReadNewOrder()
    {

        NewOrder new_order;
        in.read((char *)&new_order, sizeof(new_order));
        markets[new_order.isin_id]->ReadNewOrder(new_order);
    }

    void ReadCancelOrder()
    {

        CancelOrder cancel_order;
        in.read((char *)&cancel_order, sizeof(cancel_order));
        markets[cancel_order.isin_id]->ReadCancelOrder(cancel_order);
    }

    void ReadInputStream()
    {
        //read input stream
        while (true)
        {
            int msg_type = ReadMessageType();
            switch (msg_type)
            {
            case EMPTY:
                //std::cout << "[simulator] EMPTY\n";
                //ResetIn();
                break;
            case TIMESTAMP_MSG:
                //std::cout << "[simulator] TIMESTAMP_MSG\n";
                ReadTimeStamp();
                //getchar();
                break;
            case NEW_ORDER:
                //std::cout << "[simulator] NEW_ORDER\n";
                ReadNewOrder();
                //getchar();
                break;
            case CANCEL_ORDER:
                // std::cout << "[simulator] CANCEL_ORDER\n";
                ReadCancelOrder();
                break;
            case INSTRUMENT_INFO_REQUEST:
                //std::cout << "[simulator] INSTRUMENT_INFO_REQUEST\n";
                ReadInstrumentInfoRequest();
                //getchar();
                break;

            default:
                throw "unknow state";
            }

            if (msg_type == EMPTY)
            {
                break;
            }
        }
    }

    void WriteTimeStamp(std::stringstream &stream)
    {
        int msg_type = TIMESTAMP_MSG;
        stream.write((char *)&msg_type, sizeof(msg_type));
        stream.write((char *)&ts, sizeof(ts));
    }

    BasePipe &operator|(BasePipe &to) override
    {
        WriteTimeStamp(to.In());
        ReadOrderFile();
        ReadInputStream();
        //ReadOrderFile();
        if (out.tellp() > out.tellg())
        {
            /*std::cout << "COPY  TO SCRIPT BROKER"
                      << "out.tellg() = " << out.tellg()
                      << "out.tellp() = " << out.tellp() << std::endl;*/
            //std::cout << out.rdbuf() << std::endl;
            to.In() << out.rdbuf(); //copy
            /*std::cout << "TO_IN TO SCRIPT BROKER"
                      << "to.in.tellg() = " << to.In().tellg()
                      << "to.in.tellp() = " << to.In().tellp() << std::endl;
            std::cout << "AFTER TO SCRIPT BROKER"
                      << "out.tellg() = " << out.tellg()
                      << "out.tellp() = " << out.tellp() << std::endl;
            //getchar();*/
        }
        return to;
    }

    BasePipe &operator|(std::stringstream &stream)
    {
        return *this;
    }

    //make matching
    friend Simulator &operator>>(int ts, Simulator &market)
    {
        //std::cout << "update ts " << ts << std::endl;
        market.ts = ts;
        return market;
    }

    std::stringstream &In() override
    {
        return in;
    };
};

#endif // !__SIMULATOR_H__