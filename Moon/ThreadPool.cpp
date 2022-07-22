#include "ThreadPool.h"

void consume_start_work(ThreadPool::Worker* worker, ThreadPool* thread_pool) {
	while (true) {

		thread_pool->m_task_list_is_empty_mutex.lock();
		thread_pool->m_task_list_is_empty.wait(thread_pool->m_task_list_is_empty_mutex, [&]() -> bool {return thread_pool->m_task_queue.size() > 0; });
		thread_pool->m_task_list_is_empty_mutex.unlock();

		while (!thread_pool->m_task_queue.empty()) {
			thread_pool->m_queue_mtx.lock();
			if (!thread_pool->m_task_queue.empty()) { // 双重检测
				thread_pool->m_queue_mtx.unlock();
				auto cb = thread_pool->m_task_queue.front();
				thread_pool->m_task_queue.pop_front();
				thread_pool->m_task_list_is_empty_mutex.unlock();
				try {
					log_info("线程开始工作");
					cb();
				}
				catch (std::exception e) { // TODO 暂时不返回异常给用户处理
					log_error(e.what());
				}
			}
			else {
				thread_pool->m_queue_mtx.unlock();
				break;
			}
		}

	}

}

void ThreadPool::init_thread_pool() {
	// 预留vector位置
	this->m_thread_group.resize(this->m_thread_amount);

	// 线程初始化
	for (size_t i = 0; i < m_thread_amount; ++i) {
		Worker* worker = new Worker;
		std::thread* t = new std::thread(consume_start_work, worker, this);
		worker->m_t = t;
		t->detach();
		this->m_thread_group.push_back(worker);
	}
}

ThreadPool::ThreadPool(size_t m_thread_amount) : m_thread_amount(m_thread_amount)
{
	this->init_thread_pool();
}


void ThreadPool::add_task(std::function<void(void)> task)
{
	this->m_task_list_is_empty_mutex.lock();
	this->m_task_queue.push_front(task);
	this->m_task_list_is_empty.notify_one();
	this->m_task_list_is_empty_mutex.unlock();
}
