//#include "WriteBuffer.h"
//#include <unistd.h>
//#include "LogUtils.h"
///*
//	 返回值:
//		0 全部发送完成
//		2 数据发送不完全, epoll读写模式保持为读加写
//*/
//int WriteBuffer::Write(int fd)
//{
//	if (msg_queue.empty()) {
//		return 0;
//	}
//
//	// 尽量把全部信息发送出去
//	while (!msg_queue.empty())
//	{
//		std::string * res =  msg_queue.front();
//		int size = res->size() - write_index; //
//		int ret = write(fd, res->data() + write_index, size);
//
//		if (ret < 0 && (ret == EAGAIN || ret == EWOULDBLOCK)) {
//			return 2;
//		}
//		if (ret != size) {
//			write_index += ret;
//			return 2;
//		}
//
//		// 发送完整后重置
//		write_index = 0;
//		msg_queue.pop();
//		delete res; // 用堆来共享内存
//	}
// 	return 0;
//}
//
//int WriteBuffer::push_response(std::string * res)
//{
//	msg_queue.push(res);
//	return 0;
//}
//
//int WriteBuffer::clear_queue()
//{
//	while (!msg_queue.empty()) {
//		delete msg_queue.front();
//		msg_queue.pop();
//	}
//	return 0;
//}
