#pragma once
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <functional>
#include <unordered_map>
#include "NetworkUtils.h"
#include "Client.h"
#include "GlobalComponent.h"
#include "GlobalEnum.h"

constexpr int MAX_EVENTS = 1024;


//constexpr int BUFLEN = 4096;

/* 线程池 */
static ThreadPool* job_thread_pool = GlobalComponent::thread_pool();

// 在epoll_ctl的时候赋值，能在遍历epoll回调的时候取出
class self_event { 
public:

	Client * client; // 负责读写
	void* arg;
	int fd = -1;
	int events = 0;
	int len;
	EPOLL_SOCKET_MODE type = EPOLL_SOCKET_MODE::TCP; // 默认为TCP
	//char status;
	//long last_active; // 上一次加入epoll的时间

	void init();
	self_event(int fd, void* arg);
	self_event(int fd, void* arg, EPOLL_SOCKET_MODE type);
	self_event(const self_event& s);

	self_event& operator=(const self_event& s);
	~self_event();
};



class epoll_reactor
{

public:
	/* epoll fd*/
	int g_epfd;
	/* listen fd */
	int g_tcp_lfd;
	int g_udp_lfd;
	int mode = EPOLL_SOCKET_MODE::TCP;
	int tcp_port = 6666;
	int udp_port = 6667;
	epoll_reactor();
	epoll_reactor(EPOLL_SOCKET_MODE mode);
	void init();
	/* fd -> self_event* */
	std::unordered_map<int, self_event*> self_events; 
	pthread_rwlock_t self_evnets_ctl_rw_lock;
	self_event* get_self_event(int fd);
	int remove_self_event(int fd);
	int add_self_event(int fd, self_event*& e);
	//
	void init_tcp_socket();
	int init_udp_socket();
	/* 操作epoll红黑树 */
	void event_add(int events, self_event* se);
	void event_modify(int events, self_event* se);
	void event_remove(self_event* ev);
	// tcp
	void on_send_data(int cfd, void* arg);
	void on_accept_conn(int lfd, void* arg);
	void on_recv_data(int cfd, void* arg);
	//
	void on_udp_send_data(int cfd, void* arg);
	void on_udp_recv_data(int cfd, void* arg);
	int epoll_start();
};

