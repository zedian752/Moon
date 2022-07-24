#pragma once
#include "ReadBuffer.h"
#include "WriteBuffer.h"
#include "GlobalEnum.h"
#include "TcpReadBuffer.h"
#include "TcpWriteBuffer.h"
#include "UdpReadBuffer.h"
#include "UdpWriteBuffer.h"

/* 保存客户端对应状态, 其中包括但不限于读缓冲区和写缓冲区 */
class Client
{
public:
	Client(EPOLL_SOCKET_MODE type);
	EPOLL_SOCKET_MODE type;
	//ReadBuffer * m_read_buffer;
	//WriteBuffer * m_write_buffer;
	union read_buffer {
		TcpReadBuffer * m_tcp_read_buffer;
		UdpReadBuffer* m_udp_read_buffer;
	} m_read_buffer;

	union write_buffer {
		TcpWriteBuffer* m_tcp_write_buffer;
		UdpWriteBuffer* m_udp_write_buffer;
	} m_write_buffer;
	~Client();
};

