#include "TcpWriteBuffer.h"
#include <unistd.h>
#include "LogUtils.h"


/*
	 返回值:
		0 全部发送完成
		2 数据发送不完全, epoll读写模式保持为读加写
*/
int TcpWriteBuffer::Write(int fd)
{
	if (msg_queue.empty()) {
		return 0;
	}

	// 尽量把全部信息发送出去
	while (!msg_queue.empty())
	{
		auto res = msg_queue.front();
		int size = res->size() - write_index; //
		int ret = write(fd, res->data() + write_index, size);

		if (ret < 0 && (ret == EAGAIN || ret == EWOULDBLOCK)) {
			return 2;
		}
		if (ret != size) {
			write_index += ret;
			return 2;
		}

		// 发送完整后重置
		write_index = 0;
		msg_queue.pop();
	}
	return 0;
}

int TcpWriteBuffer::push_response(std::shared_ptr<std::string> res)
{
	msg_queue.push(res);
	return 0;
}

int TcpWriteBuffer::clear_queue()
{
	while (!msg_queue.empty()) {
		msg_queue.pop();
	}
	return 0;
}
