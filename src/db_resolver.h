#ifndef __DB_RESOLVER_H__
#define __DB_RESOLVER_H__
#include <string>

struct DataPathResolver
{
    static std::string ResolveExchange(std::string symbol)
    {
        if (symbol == "Si-16.12")
        {
            return "FORTS";
        }
        else if (symbol == "USD000UTSTOM" || symbol == "USD000000TOD")
        {
            return "MOEX_CURRENCY";
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

        if (symbol == "USD000UTSTOM" || symbol == "USD000000TOD")
        {
            return "/home/soarix/Data/MicexOrderBook.db";
        }

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
        if (symbol == "USD000UTSTOM" || symbol == "USD000000TOD")
        {
            return "/home/soarix/Data/MicexOrderBookSettings.json";
        }

        std::string settings_path = "/home/soarix/Data/FortsOrderBookSettings.json";
        return settings_path;
    }
} DataPathResolver;

#endif // !__DB_RESOLVER_H__
