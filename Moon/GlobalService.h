#pragma once
#include <queue>
#include <thread>
#include <pthread.h>
#include <condition_variable>
#include <atomic>
#include <unordered_map>
#include <vector>
#include <string>
//
#include "Node.h"
#include "Worker.h"

class GlobalService {
private:
	pthread_spinlock_t spin_lock;
	GlobalService();


	void init();
public:
	/* 全局节点队列 */  // 获取到节点之后，再从节点中获取信息
	std::queue<Node*> global_queue;
	void pushNode(Node* node);
	Node* popNode();

	static GlobalService* instance();
	
	/* */
	std::vector<Worker> workers;
	/* 总线程数量 */
	int total_worker_amount = 4;

	/* 正在运作的线程数量 */
	std::atomic_int worker_num;
	/* 等待任务通知 */
	std::condition_variable cv_ready;
	std::mutex mtx;
	std::unique_lock<std::mutex> mtx_ready;
	/* 服务名 -> 服务 */
	std::unordered_map<std::string, Node*> services;

	void send(std::string src, std::string target, const char* cmd);
	

};