#pragma once
#include "Node.h"
class Ping1 :
    public Node
{
    void onBaseMsg(std::shared_ptr<BaseMsg> msg);
};

