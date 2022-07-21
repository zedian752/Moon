#include "WriteBuffer.h"
#include <unistd.h>
#include "LogUtils.h"
/*
	 返回值:
		0 全部发送完成
		2 还有剩余
*/
int WriteBuffer::Write(int fd)
{
	if (msg_queue.empty()) {
		return 0;
	}

	// 尽量把全部信息发送出去
	while (!msg_queue.empty())
	{
		std::string * res =  msg_queue.front();
		int size = res->size() - write_index; //
		int send = write(fd, res->data() + write_index, size);
		if (send != size) {
			// 
			write_index += send;
			return 1;
		}
		// 发送完整后重置
		write_index = 0;
		msg_queue.pop();
		delete res; // 用堆来共享内存
	}
 	return 0;
}

int WriteBuffer::push_response(std::string * res)
{
	msg_queue.push(res);
	return 0;
}
