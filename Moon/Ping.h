#pragma once
#include "Node.h"
class Ping :
    public Node
{
public:
	Ping(const char* name):Node(name){}
	void onBaseMsg(std::shared_ptr<BaseMsg> msg);
	~Ping();
};

