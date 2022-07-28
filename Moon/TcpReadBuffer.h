#pragma once
#include <memory>
/*
读缓存区
使用声明长度法解决粘包问题
*/


class TcpReadBuffer
{
	// 回收无效内存
	int recover();
public:
	using Byte = char;
	/* 缓冲区 */
	Byte* buffer;
	/* 总容量 */
	int capacity;

	int init_size = 1024;
	//int factor = 0.75;
	/* 当前的读取游标处于buffer区的哪个位置 */
	int write_index = 0;

	int read_index = 0; // 在此游标之前的数据都是已经被读取的数据。 /* 解决了，每次处理完一个数据包之后都要memcpy的问题，除此以外还可以利用read固定字节数的办法来解决减少memcpy的次数，但是会增加read的次数*/
	/* 该数据包已读取的数据量大小 */
	//int content_size = 0;
	int msg_size = -1;
	// 当前包的包头位置
	//int msg_header_loca = -1;
	/* 固定前四个字节作为消息长度 */
	const int header_length = 4;
	//ReadBuffer();
	//ReadBuffer(int buffer_size);

	TcpReadBuffer();
	TcpReadBuffer(int buffer_size);
	~TcpReadBuffer();

	int reset();
	int expand(int size);
	int rest_size();
	void init_buffer(int buffer_size);
	std::pair<int, std::shared_ptr<std::string>> Read(int fd);
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