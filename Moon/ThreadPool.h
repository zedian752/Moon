#pragma once
#include <mutex>
#include <condition_variable>
#include <list> 
#include <array>
#include <thread>
#include <cstddef>
#include <functional>
#include <atomic>
#include <vector>
#include <exception>
#include "LogUtils.h"

/*
使用有界队列，假如线程数5，最大线程数为10，队列数量为20，当超出队列数量时，线程数开始增长
*/


//void consume_start_work(ThreadPool::Worker* worker, ThreadPool* thread_pool);

class ThreadPool
{
	struct Worker {
		std::thread* m_t;
		~Worker() {
			delete m_t;
		}
	};

	size_t m_thread_amount; // 线程数量
	size_t m_max_thread_amount; // 最大线程数量 TODO-动态线程数量
	std::list<std::function<void(void)>> m_task_queue; // 外部参数请用闭包闭进来
	std::condition_variable_any m_task_list_is_empty; // 等待任务到来
	std::mutex m_task_list_is_empty_mutex;
	std::mutex m_queue_mtx;
	std::vector<Worker*> m_thread_group;

private:
	void init_thread_pool(void);
	friend void consume_start_work(ThreadPool::Worker* worker, ThreadPool* thread_pool);
public:
	ThreadPool(size_t m_thread_amount);
	//size_t get_queue_list();
	void add_task(std::function<void(void)> task);
};

