#pragma once
#include "WriteBuffer.h"
#include <netinet/in.h>
#include <memory>


struct UdpSendMsg{
	std::string * msg;
	sockaddr_in* client_addr; // 客户端
	UdpSendMsg(){}
	UdpSendMsg(std::string* msg, sockaddr_in* client_addr) : msg(msg), client_addr(client_addr) {

	}
	~UdpSendMsg() {
		delete msg;
		delete client_addr;
	}
};


class UdpWriteBuffer
{
public:
	int write_index = 0;
	std::queue<std::shared_ptr<UdpSendMsg>> msg_queue;
	int Write(int fd);
	int push_response(std::shared_ptr<UdpSendMsg>);
	int clear_queue();

	
};

