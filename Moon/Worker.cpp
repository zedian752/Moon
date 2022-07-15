#include "Worker.h"
#include <assert.h>
#include "GlobalService.h"

void Worker::operator()()
{
	while (!isStop) {
		auto global_service = GlobalService::instance();
		global_service->cv_ready.wait(global_service->mtx_ready);
		Node* node = global_service->popNode();
		
		/* 校验 */	
		assert(node != nullptr);
		node->isInGlobalQueue = true;
		node->processMsg(handle_num);

		/* 如果还有剩余的任务未完成, 则设置回全局消息队列 */
		if (!node->tasks.empty()) {
			global_service->pushNode(node);
			continue;
		}

		/* 释放资源 */
		node->isInGlobalQueue = false;
		global_service->worker_num.fetch_sub(1);
	}
}

