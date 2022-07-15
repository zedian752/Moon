#pragma once
#include <queue>
#include <pthread.h>
#include <memory>
#include "BaseMsg.h"


class Node
{ 

private:
	virtual void onBaseMsg(std::shared_ptr<BaseMsg>);
public:
	const char* name;
	/* 任务队列是否已经在全局消息队列中 */
	bool isInGlobalQueue = false;
	/* 作为任务的插入和弹出锁 */
	pthread_spinlock_t spin_lock;
	// 
	std::queue<std::shared_ptr<BaseMsg>> tasks;

	Node(const char * name);

	void processMsg(int nums);
	void push_queue(std::shared_ptr<BaseMsg>);
	std::shared_ptr<BaseMsg> pop_queue();

	void send(const char* target, const char* cmd);
	virtual ~Node();
};

