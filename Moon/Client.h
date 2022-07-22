#pragma once
#include "ReadBuffer.h"
#include "WriteBuffer.h"

/* 保存客户端对应状态, 其中包括但不限于读缓冲区和写缓冲区 */
class Client
{
public:
	ReadBuffer m_read_buffer;
	WriteBuffer m_write_buffer;
};

