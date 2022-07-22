#pragma once
#include <string.h>
#include <string>

/*
读缓存区
使用声明长度法解决粘包问题
*/
constexpr int MAX_CONTENT_SIZE = 10000;

class ReadBuffer
{
	int reset();
	int expand(int size);
	/* 返回缓冲区剩余大小 */
	int rest_size();
	void init_buffer(int buffer_size);
public:

	using Byte = char;
	/* 缓冲区 */
	Byte * buffer;
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
	ReadBuffer();
	ReadBuffer(int buffer_size);

	std::pair<int, std::string*> Read(int fd);
	

	~ReadBuffer();

};

/*
	接收缓冲区解决方案：
		设置读缓冲区，使用长度信息法获取读写长度，将2字节作为长度单位(65536)
		优先读2字节，获取到字节单位后读取固定长度
		得到一整个包的长度后，转交msghandler。
		
*/


/*
	发送缓冲区解决方案：
	设置发送消息队列，队列元素为{buffer = char[], sendIndex(sendIndex之前的字节都为已发送字节), restlength(剩余未发送字节长度)}
	每次触发write事件时，出列队头元素，写入发送缓冲区，更新sendIndex和restLength，restLength若为0，即出队并free掉这个队列元素
*/
