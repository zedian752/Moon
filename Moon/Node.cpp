#include "Node.h"
#include "GlobalService.h"


Node::Node(const char* name):name(name)
{
	pthread_spin_init(&spin_lock, NULL);
}

void Node::processMsg(int nums)
{
	for (int i = 0; i < nums; i++) {
		auto msg = this->pop_queue();
		if (msg == nullptr) {
			return;
		}
		onBaseMsg(msg);
	}
}

void Node::push_queue(std::shared_ptr<BaseMsg> msg)
{
	pthread_spin_lock(&spin_lock);
	tasks.push(msg);
	pthread_spin_unlock(&spin_lock);
}

std::shared_ptr<BaseMsg> Node::pop_queue()
{
	std::shared_ptr<BaseMsg> msg = nullptr;
	pthread_spin_lock(&spin_lock);
	if (!tasks.empty()) {
		msg = tasks.front();
		tasks.pop();
	}
	pthread_spin_unlock(&spin_lock);
	return msg;
}

void Node::send(const char* target, const char* cmd)
{
	auto global_service = GlobalService::instance();
	global_service->send(name, target, cmd);
}
Node::~Node()
{
}




/* 真正的业务处理函数 */
void Node::onBaseMsg(std::shared_ptr<BaseMsg> msg)
{
	
	if ((*msg).cmd == "a1") {

	}
}

