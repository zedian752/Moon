#pragma once
#include <string>
using std::string;
class BaseMsg
{
public:
	string cmd;
	// 源服务
	string source_service;

	/* debug_field */
	int trace_id;
	/* debug_field */
	long time;
};

