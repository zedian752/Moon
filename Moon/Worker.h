#pragma once
#include <thread>
#include <condition_variable>



/* 存储工作线程的具体运作函数，存储工作线程工作中所需资源，包括控制线程和函数计算所需资源*/
class Worker
{
public:
	/* 一次处理的数量，过大的数量会造成业务饥饿性等待 */
	int handle_num = 100;
	bool isStop = false;

	void operator()();
	//std::thread * t;
};

