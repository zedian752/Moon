#include "GlobalService.h"
#include <thread>
#include <iostream>
#include <memory>
#include "Utils.h"

void GlobalService::pushNode(Node* node)
{ 
	pthread_spin_lock(&spin_lock);
	/* 插入 */
	global_queue.push(node);
	pthread_spin_unlock(&spin_lock);
}

Node* GlobalService::popNode()
{
	Node* node = nullptr;
	pthread_spin_lock(&spin_lock);
	/* 弹出队首元素 */
	if (!global_queue.empty()) {
		node = global_queue.front();
		global_queue.pop();
	}
	pthread_spin_unlock(&spin_lock);
	return node;
}

GlobalService::GlobalService()
{
	init();
}

// Core function
void GlobalService::send(std::string src, std::string target, const char* cmd)
{
	std::shared_ptr<BaseMsg> basemsg  = std::make_shared<BaseMsg>();
	basemsg->cmd = cmd;
	basemsg->source_service = src;
	basemsg->trace_id = Utils::getIncrement();
	basemsg->time = time(NULL);
	/* 投递信息 */
	//auto node = services.at(target);
	auto node = services[target];
	if (node == nullptr) return;
	node->push_queue(basemsg);

	/* 唤醒工作线程 */
	if (worker_num == total_worker_amount) { // 如果工作线程已经全部唤醒了
		return;
	}
	/* 已经在遍历执行了*/
	else if (node->isInGlobalQueue) {
		/* 防止任务没有全部完成的情况 */
		//pushNode(node);
		std::cout << node->name <<"已经在全局队列内" << std::endl;
		return;
	}
	else {
		/* 推进全进消息队列 */
		pushNode(node);
		cv_ready.notify_one();
		std::cout << "current working num" << worker_num.fetch_add(1) << std::endl;
	}

	
}

void GlobalService::newservice(std::string name, Node* node)
{
	services[name] = node;
}

void GlobalService::removeService(std::string name)
{
	auto it = services.find(name);
	auto node = it->second;

	if (node != nullptr) {
		services.erase(name);
		delete node;
	}
}

/* 目前限制为一个进程只有一个service */
GlobalService* GlobalService::instance()
{
	static GlobalService global_sercice;
	return &global_sercice;
}

void GlobalService::init()
{
	worker_num.store(0);
	pthread_spin_init(&spin_lock, NULL);
	workers.reserve(total_worker_amount);
	mtx_ready = std::unique_lock<std::mutex>(mtx);
	for (int i = 0; i < total_worker_amount; i++) {

		Worker worker;
		std::thread t(worker);
		/* 分离线程，不用考虑线程终止回收的问题 */
		t.detach();
		workers.push_back(worker);
	}
}
