#include "ThreadPool.h"
#pragma once
class GlobalComponent
{public:
	static ThreadPool* thread_pool() {
		static ThreadPool job_thread_pool(1);
		return &job_thread_pool;
	}
};

