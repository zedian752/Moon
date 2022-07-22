#include <stdlib.h>
#include <unistd.h>
#include "ReadBuffer.h"
#include <errno.h>
#include <memory>


static int table_size_for(int size) {
	unsigned int n = size - 1;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	return (n < 0) ? 1 : n + 1;
}

int ReadBuffer::reset()
{
	write_index = 0;
	msg_size = -1;
	return 0;
}

int ReadBuffer::expand(int size)
{
	size = table_size_for(size);
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
	buffer_size = table_size_for(buffer_size);
	buffer = (char*)malloc(buffer_size);
	capacity = buffer_size;
}

ReadBuffer::ReadBuffer()
{
	init_buffer(1024);
}



ReadBuffer::ReadBuffer(int buffer_size)
{
	init_buffer(buffer_size);

}

/*
	返回值：
	0：客户端离线
	-1：读取错误
	1：读取成功, 但没有读取完应用层数据
	2: 将监听读事件转换为监听写事件，此时业务已经处理完成


*/
std::pair<int, std::string*> ReadBuffer::Read(int fd) {

	while (true) {
		int ret = read(fd, buffer + write_index, rest_size());
		/* 客户端离线 */
		if (ret == 0) {
			return { 0, nullptr };
		}
		if (ret == -1) {
			// 当前没有数据可以进行读取，非阻塞io马上返回
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				return { 1 , nullptr };
			}
		}
		write_index += ret;

		/* 读取消息头 */
		if (write_index < header_length) { // ? + msg_size
			/* 读取满足n个字节才开始解包头*/
			return { 1 , nullptr };
		}
		else {
			if (msg_size == -1) { /* 解包头 */
				unsigned int* msg_length = (unsigned int*)buffer;
				msg_size = *msg_length;
				/* 防止消息体过大 */
				if (msg_size > MAX_CONTENT_SIZE) {
					return { -1 , nullptr };
				}
				/* 判断需不需要要进行扩容 */
				if (msg_size > rest_size()) {
					expand(msg_size);
				}
			}
		}
		/* 读取消息体 */
		if (msg_size != -1 && write_index >= msg_size + header_length) {
			char* msg = new char[msg_size];
			memcpy(msg, buffer + header_length, msg_size);
			std::shared_ptr<char> p_msg(msg, [](char* msg) -> bool {
				delete[] msg;
				});
			reset();
			// 处理业务
			//std::shared_ptr<char>
			std::string* res = new std::string("AVA BVB");
			return { 2 , res };
		}
	}
}




ReadBuffer::~ReadBuffer()
{
	free(buffer);
}

