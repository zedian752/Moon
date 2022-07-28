#include "TcpReadBuffer.h"

#include<string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "LogUtils.h"

static constexpr int MAX_CONTENT_SIZE = 10000;
static int table_size_for(int size) {
	unsigned int n = size - 1;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	return (n < 0) ? 1 : n + 1;
}

int TcpReadBuffer::reset()
{
	//write_index = 0;
	msg_size = -1;
	return 0;
}

int TcpReadBuffer::expand(int size)
{
	size = table_size_for(size);
	buffer = (char*)realloc(buffer, size);
	capacity = size;
	return 0;
}

/* 缓冲区剩余容量*/
int TcpReadBuffer::rest_size()
{
	return capacity - write_index;
}

void TcpReadBuffer::init_buffer(int buffer_size)
{
	buffer_size = table_size_for(buffer_size);
	buffer = (char*)malloc(buffer_size);
	capacity = buffer_size;
}

int TcpReadBuffer::recover()
{
	auto alive_byte_size = write_index - read_index; //还有用的字节数
	memcpy(buffer, buffer + read_index, alive_byte_size);
	read_index = 0; // 下标重置
	write_index -= alive_byte_size; // 下标左移

	return 0;
}

TcpReadBuffer::TcpReadBuffer()
{
	init_buffer(init_size);
}



TcpReadBuffer::TcpReadBuffer(int buffer_size)
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
std::pair<int, std::shared_ptr<std::string>> TcpReadBuffer::Read(int fd) {

	while (true) {
		int ret = read(fd, buffer + write_index, rest_size());
		/* 客户端离线 */
		if (ret == 0) {
			return { 0, nullptr };
		}
		if (ret < 0) {
			// 当前没有数据可以进行读取，非阻塞io马上返回
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				return { 1 , nullptr };
			}
			else {
				return { ret, nullptr };
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
				unsigned int * msg_length = (unsigned int*)buffer;
				msg_size = *msg_length;
				/* 防止消息体过大 */
				if (msg_size > MAX_CONTENT_SIZE) {
					return { -1 , nullptr };
				}
		
			}
		}
		/* 读取消息体 */
		if (msg_size != -1 && write_index >= msg_size + header_length) {

			/* 判断需不需要要进行扩容, 能通过回收废弃字节就先回收，不行就回收 + 扩容 */
			auto rest_s = rest_size();
			if (msg_size > rest_s) {
				if (msg_size <= rest_s + read_index) {
					// 回收read_index
					recover();
				}
				else {
					// 回收read_index + 扩容
					recover();
					expand(msg_size);
				}
			}
			/* 拷贝消息体数据 */
			char* msg = new char[msg_size];
			memcpy(msg, buffer + header_length, msg_size);
			read_index += header_length + msg_size;
			reset();
			// 处理业务
			//std::shared_ptr<std::string>
			// 把数据从缓冲区拿走
			return { 2 , std::make_shared<std::string>("AVA BVB")};
		}
	}
}




TcpReadBuffer::~TcpReadBuffer()
{
	free(buffer);
}

