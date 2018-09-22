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

class Simulator : public BasePipe
{

    std::unordered_map<int32_t, Reader *> readers;
    std::unordered_map<int32_t, Market<Order> *> markets;
    int32_t ts = 0;
    std::stringstream in;

  public:
    Simulator() {}

    void AddSymbol(SymbolSettings &settings)
    {
        struct tm tm;
        memset(&tm, 0, sizeof(struct tm));
        const char *time_details = (const char *)settings.date.c_str();
        printf("%s \n", time_details);
        //strptime(time_details, "%Y-%m", &tm);
        strptime(time_details, "%Y%m%d", &tm);

        std::cout << tm.tm_year + 1900 << " " << tm.tm_mon + 1 << " " << tm.tm_mday << std::endl;

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
        for (auto i : markets)
        {
            i.second->ReadOrderFile();
        }
    }

    BasePipe &operator|(BasePipe &to) override
    {
        ReadOrderFile();
        return to;
    }

    BasePipe &operator|(std::stringstream &stream)
    {
        return *this;
    }

    //make matching
    friend Simulator &operator>>(int ts, Simulator &market)
    {
        std::cout << "update ts " << ts << std::endl;
        market.ts = ts;
        return market;
    }

    std::stringstream &In() override
    {
        return in;
    };
};

#endif // !__SIMULATOR_H__