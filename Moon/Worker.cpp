#include "Worker.h"
#include <assert.h>
#include "GlobalService.h"

void Worker::operator()()
{
	while (!isStop) {
		auto global_service = GlobalService::instance();
		global_service->cv_ready.wait(global_service->mtx_ready);
		Node* node = global_service->popNode();
		//global_service->mtx_ready.unlock();

		/* 校验 */	
		assert(node != nullptr);
		node->processMsg(handle_num);
		/* 总工作线程*/
		global_service->worker_num.fetch_sub(1);
	}
	

}

