#pragma once
#include <queue>
#include <string>
class WriteBuffer
{
public:
	/* 在wi之前的都是已经发送了的数据 */
	int write_index = 0;
	/* 待发送队列 */
	std::queue<std::string*> msg_queue;
	int Write(int fd);
	int push_response(std::string*);
	int clear_queue();
};

