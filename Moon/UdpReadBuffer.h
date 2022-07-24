#pragma once
#include "UdpWriteBuffer.h"
#include <memory>



class UdpReadBuffer
{
public:
	using Byte = char;
	/* 缓冲区 */
	Byte* buffer;
	/* 总容量 */
	int capacity;
	//int factor = 0.75;
	/* 当前的读取游标处于buffer区的哪个位置 */
	int write_index = 0;
	/* 该数据包已读取的数据量大小 */
	//int content_size = 0;
	int msg_size = -1;
	// 当前包的包头位置
	//int msg_header_loca = -1;
	/* 固定前四个字节作为消息长度 */
	const int header_length = 4;
	//ReadBuffer();
	//ReadBuffer(int buffer_size);

	int reset();
	int expand(int size);
	/* 返回缓冲区剩余大小 */
	int rest_size();
	void init_buffer(int buffer_size);
	UdpReadBuffer();
	UdpReadBuffer(int buffer_size);
	std::pair<int, std::shared_ptr<UdpSendMsg>> Read(int fd);

	~UdpReadBuffer();
};

