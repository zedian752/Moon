#pragma once
#include "WriteBuffer.h"
#include <memory>
class TcpWriteBuffer
{
public:
	/* 在wi之前的都是已经发送了的数据 */
	int write_index = 0;
	/* 待发送队列 */
	std::queue<std::shared_ptr<std::string>> msg_queue;
	int Write(int fd);
	int push_response(std::shared_ptr<std::string>);
	int clear_queue();
};

