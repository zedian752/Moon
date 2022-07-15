#include "Ping.h"
#include "GlobalService.h"
#include <stdio.h>

void Ping::onBaseMsg(std::shared_ptr<BaseMsg> msg)
{
	auto global_service = GlobalService::instance();
	if (msg->cmd == "ping") {
		printf("source: [%s], here: [%s]\n", msg->source_service.data(), name);
		global_service->send(name, (*msg).source_service.c_str(), "ping");
	}
}

Ping::~Ping()
{
}
