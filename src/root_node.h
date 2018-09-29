#ifndef __ROOT_NODE_H__
#define __ROOT_NODE_H__

#include "../strategies/arbitrage.h"
#include "../strategies/spreader.h"

struct RootNode : public VNode
{
    DataStorage *storage;
    std::stringstream *out;
    std::unordered_map<int32_t, VNode *> nodes;
    int64_t ts;

    //Spreader *spreader;
    //Spreader *spreader2;

    RootNode(DataStorage &storage, std::stringstream &out) : storage(&storage), out(&out)
    {
        id = 1;
        nodes.insert(std::pair<int32_t, VNode *>(id, this));
        Initialization();
        Build();
    }

    void Initialization()
    {
        //Spreader *spreader = new Spreader("USD000UTSTOM", *storage, *out);
        //Spreader *spreader2 = new Spreader("RTS-12.16", *storage, *out);
        //SpreaderSber *spreader2 = new SpreaderSber(*storage, *out);
        //Spreader *spreader2 = new Spreader("SBRF-12.16", *storage, *out);
        //Spreader *spreader3 = new Spreader("RTS-12.16", *storage, *out);

        //spreader->SetProperty("spread", "4");
        //spreader2->SetProperty("spread", "10");
        //spreader2->SetProperty("spread", "15");
        //spreader3->SetProperty("spread", "100");

        //Mount(*spreader);
        //Mount(*spreader2);
        // Mount(*spreader2);
        // Mount(*spreader3);

        Arbitrage *arbitrage = new Arbitrage(*storage, *out);
        Mount(*arbitrage);
    }

    void Mount(VNode &node)
    {
        nodes.insert(std::pair<int32_t, VNode *>(++id, &node));
        childs.push_back(&node);
    }

    void Build()
    {
    }

    void Do()
    {
        for (auto &child : childs)
        {
            child->Do();
        }
    }

    void SetProperty(std::string name, std::string value) override
    {

        if (name == "status")
        {
            SetPropertyForAll(name, value);
        }
    }

    void SetPropertyForAll(std::string name, std::string value)
    {

        for (auto &kvp : nodes)
        {
            if (kvp.second->id > 1)
            {
                kvp.second->SetProperty(name, value);
            }
        }
    }

    void SetStrategyProperty(int id, std::string name, std::string value)
    {
        if (nodes.count(id))
        {
            nodes[id]->SetProperty(name, value);
        }
        else
        {
            //response fail
        }
    }

    void GetSchema()
    {
        std::string schema = R"(
        
        si-spreader:
            type: spreader
            symbol:
                Si-12.16
                    - OrderBook
                    - 1minut
            isin:
                - 148131
            params:
                MaxAmount:
                    initial: 20
                    type: i
                Limit: 10000
                User: semen
                Risk: Non

        )";
    }

    void RequestSettings()
    {
        for (auto &child : childs)
        {
            child->RequestSettings();
        }
    }

    void Print()
    {
        for (auto &child : childs)
        {
            child->Print();
        }
    }

    void UpdateTime(int64_t ts)
    {
        this->ts = ts;
        for (auto &child : childs)
        {
            child->UpdateTime(ts);
        }
    }
};
#endif // !__ROOT_NODE_H__
