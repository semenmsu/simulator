#ifndef __VNODE_H__
#define __VNODE_H__
#include <vector>
#include <string>

class VNode
{
  public:
    VNode *parent;
    std::vector<VNode *> childs;
    int id;
    VNode *ref;

    VNode()
    {
        ref = this;
    }

    VNode *GetRoot()
    {

        if (parent == nullptr)
        {
            return parent;
        }
        VNode *root = parent;
        while (root->parent != nullptr)
        {
            root = root->parent;
        }
        return root;
    }

    virtual void Do(){};
    virtual void UpdateTime(int64_t ts){};
    virtual void Print(){};
    virtual void SetProperty(std::string name, std::string value) {}
    virtual void RequestSettings(){};

    void GetMetaInfo()
    {
    }
};
#endif // !__VNODE_H__
