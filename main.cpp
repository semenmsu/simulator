#include <stdio.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "include/json.hpp"
#include "src/market.h"
#include "tests/testorder.h"

using json = nlohmann::json;
json j3;

/*
class FortsFutSettings
{
    int64_t startPosition;
    int64_t stopPosition;
    int32_t isinId;
    std::string symbol;
    std::string initstr;

  public:
    FortsFutSettings() : startPosition(0), stopPosition(0){};
    FortsFutSettings(json &j)
    {
        InitFromJson(j);
    };

    void InitFromJson(json &j)
    {
        startPosition = j["StartPostion"];
        stopPosition = j["StopPosition"];
        isinId = j["IsinId"];
        symbol = j["Symbol"];

        initstr = j.dump();
    }

    int64_t StartPosition()
    {
        return startPosition;
    }

    int64_t StopPosition()
    {
        return stopPosition;
    }

    std::string Symbol()
    {
        return symbol;
    }

    void Print()
    { //std::cout << "StartPosition: " << startPosition << std::endl;
        std::cout << "Settings: " << symbol << " " << startPosition << " " << stopPosition << std::endl;
    };
};

struct FortsFutOrderBook
{
    int64_t orderid;
    int64_t dealid;
    int64_t price;
    int64_t deal_price;
    int64_t moment; //40
    int32_t amount;
    int32_t amount_rest;
    int32_t status;
    int32_t user_code; //16
    char action;
    char dir;
    char __padding[2];
    friend std::ostream &operator<<(std::ostream &stream, const FortsFutOrderBook &order)
    {
        stream << order.price << " " << order.amount << " |" << (int)order.dir << " " << order.orderid << "  " << order.user_code << '\n';
        return stream;
    }
} __attribute__((packed, aligned(4)));

struct FortsOrder
{
    int64_t orderid;
    int64_t price;
    int64_t moment; //40
    int32_t amount;
    int32_t status;
    int32_t user_code; //16
    char action;
    char dir;
    char __padding[2];
    int cl_ord_id;
    FortsOrder(){};
    FortsOrder(const FortsFutOrderBook &source)
    {
        orderid = source.orderid;
        price = source.price;
        moment = source.moment;
        amount = source.amount;
        status = source.status;
        action = source.action;
        dir = source.dir;
        user_code = source.user_code;
    }
    friend std::ostream &operator<<(std::ostream &stream, const FortsOrder &order)
    {
        stream << order.price << " " << order.amount << " |" << (int)order.dir << " " << order.orderid << "  " << order.user_code << '\n';
        return stream;
    }
} __attribute__((packed, aligned(4)));

void InitConfig()
{
    //std::ifstream ifs("/home/soarix/Data/FortsOrderBookSettings.json");
    //std::string content((std::istreambuf_iterator<char>(ifs)),(std::istreambuf_iterator<char>()));
    std::ifstream i("/home/soarix/Data/FortsOrderBookSettings.json");
    i >> j3;
}

void GetSettings(std::string symbol, FortsFutSettings &settings)
{

    if (j3 != nullptr)
    {
        for (auto &i : j3["Instruments"])
        {

            if (i["Symbol"] == symbol)
            {
                settings.InitFromJson(i);
            }
        }
    }
    else
    {
        throw "get settings invalid json";
    }
}

void ReadFile_(int64_t from, int64_t to)
{
    std::ifstream ifs("/home/soarix/Data/FortsOrderBook.db", std::ios::binary);
    ifs.seekg(from, std::ios::beg);
    int size = to - from;
    std::vector<FortsFutOrderBook> v(size / sizeof(FortsFutOrderBook));
    ifs.read((char *)&v[0], size);
    Market<FortsOrder> si;
    printf("size = %d\n", size);
    for (uint32_t i = 0; i < v.size(); i++)
    {
        FortsOrder order(v[i]);
        si.PlaceOrder(order);
    }

    ifs.close();
}

void scenario1()
{
    InitConfig();
    FortsFutSettings settings;
    GetSettings("Si-12.16", settings);
    settings.Print();
    ReadFile_(settings.StartPosition(), settings.StopPosition());
}
*/

int main()
{

    //read_file(settings.StartPosition(), settings.StopPosition());
    //read_file3(settings.StartPosition(), settings.StopPosition());
    //
    Market<Order> mkt;
    Script script;
    auto cycle = [&](int ts) { ts >> mkt | script | mkt.in; };
    std::vector<int> steps = {1, 2, 3, 4};

    for (auto &i : steps)
    {
        cycle(i);
        std::cout << "cycle \n\n"
                  << std::endl;
        usleep(1000000);
    }
    return 0;
    return 0;
}