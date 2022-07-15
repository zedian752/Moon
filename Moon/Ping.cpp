#include "Ping.h"
#include "GlobalService.h"
#include <stdio.h>
#include <random>
#include <string>
#include <iostream>
#include <time.h>
#include <cstdlib>// Header file needed to use rand


void Ping::onBaseMsg(std::shared_ptr<BaseMsg> msg)
{
	auto ti = static_cast<long>(time(NULL));
	
	std::uniform_int_distribution<int> u(1, 4);
	std:: default_random_engine e(ti);

	auto global_service = GlobalService::instance();
	if (msg->cmd == "ping") {
		std::string target = "ping" + std::to_string(rand() % 4 + 1);

		printf("source: [%s], here: [%s], diff time:[%ld]\n", msg->source_service.data(), name, time(NULL) - msg->time);
		global_service->send(name, target.data(), "ping");
	}
}

Ping::~Ping()
{
}
