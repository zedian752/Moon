#include "Client.h"
#include "TcpReadBuffer.h"
#include "TcpWriteBuffer.h"
#include "UdpReadBuffer.h"
#include "UdpWriteBuffer.h"

Client::Client(EPOLL_SOCKET_MODE type): type(type)
{
	if (type == EPOLL_SOCKET_MODE::TCP) {
		m_read_buffer.m_tcp_read_buffer = new TcpReadBuffer;
		m_write_buffer.m_tcp_write_buffer = new TcpWriteBuffer;
	}
	else if (type == EPOLL_SOCKET_MODE::UDP) {
		m_read_buffer.m_udp_read_buffer = new UdpReadBuffer;
		m_write_buffer.m_udp_write_buffer = new UdpWriteBuffer;
	}
}

Client::~Client()
{
	if (type == EPOLL_SOCKET_MODE::TCP) {
		delete m_read_buffer.m_tcp_read_buffer;
		delete m_write_buffer.m_tcp_write_buffer;
	}
	else if (type == EPOLL_SOCKET_MODE::UDP) {
		delete m_read_buffer.m_udp_read_buffer;
		delete m_write_buffer.m_udp_write_buffer;
	}
}
