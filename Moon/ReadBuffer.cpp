#include <stdlib.h>
#include <unistd.h>
#include "ReadBuffer.h"
#include <errno.h>
#include <memory>

int ReadBuffer::reset()
{
	write_index = 0;
	return 0;
}

int ReadBuffer::expand(int size)
{
	int size = tableSizeFor(size);
	buffer = (char*)realloc(buffer, size);
	capacity = size;
	return 0;
}

/* 缓冲区剩余容量*/
int ReadBuffer::rest_size()
{
	return capacity - write_index;
}

void ReadBuffer::init_buffer(int buffer_size)
{
	auto buffer_size = tableSizeFor(buffer_size);
	buffer = (char*)malloc(buffer_size);
	capacity = buffer_size;
}

ReadBuffer::ReadBuffer()
{
	init_buffer(1024);
}

static int tableSizeFor(int size) {
	unsigned int n = size - 1;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	return (n < 0) ? 1 : n + 1;
}

ReadBuffer::ReadBuffer(int buffer_size)
{
	init_buffer(buffer_size);

}

ReadBuffer::~ReadBuffer()
{
	free(buffer);
}

/*
	返回值：
	0：客户端离线
	-1：读取错误
	1：读取成功


*/
int ReadBuffer::Read(int fd) {

	while (true) {
		int ret = read(fd, buffer + write_index, rest_size());
		/* 客户端离线 */
		if (ret == 0) {
			return ret;
		}
		if (ret == -1) {
			// 当前没有数据可以进行读取，非阻塞io马上返回
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				return 1;
			}
		}
		write_index += ret;

		/* 读取消息头 */
		if (write_index < header_length) {
			/* 读取满足n个字节才开始解包*/
			return 1;
		} else {
			unsigned int* msg_length = (unsigned int*)buffer;
			msg_size = *msg_length;
			/* 防止消息体过大 */
			if (msg_size > MAX_CONTENT_SIZE) {
				return -1;
			}
			/* 判断需不需要要进行扩容 */
			if (msg_size > rest_size()) {
				expand(msg_size);
			}
		}
		/* 读取消息体 */
		if (write_index >= msg_size) { 
			char* msg = new char[msg_size];
			memcpy(msg, buffer + header_length, msg_size);

			/* 将内存交给智能指针管理 */
			std::shared_ptr<char*> p_msg(msg);
			reset();
			// 处理业务
			//std::shared_ptr<char>
		}
	}
}

