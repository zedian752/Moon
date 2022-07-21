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


constexpr int MAX_EVENTS = 1024;
constexpr int BUFLEN = 4096;

/* 线程池 */
static ThreadPool* job_thread_pool = GlobalComponent::thread_pool();
class epoll_reactor;
class self_event;
using reactor_cb_t = void(epoll_reactor* er, int cfd, int events, void* arg);


// 在epoll_ctl的时候赋值，能在遍历epoll回调的时候取出
class self_event { 
public:
	void* arg;
	int fd = -1;
	int events = 0;
	char status;
	Client client;
	int len;
	long last_active; // 上一次加入epoll的时间

	void init() {
		last_active = time(NULL);
	}
	self_event(int fd, void* arg);
	self_event(const  self_event& s);

	self_event& operator=(const self_event& s);
};

class epoll_reactor
{
public:
	/* epoll fd*/
	int g_epfd;
	/* listen fd */
	int g_lfd;
	int port = 6666;
	/* fd -> self_event* */
	std::unordered_map<int, self_event*> self_events; 
	//self_event self_events[MAX_EVENTS + 1];// 具体节点


	void init_socket();
	/* 操作epoll红黑树 */
	void event_add(int events, self_event* se);
	void event_modify(int events, self_event* se);

	void event_remove(self_event* ev);

	void on_send_data(int cfd, int events, void* arg);
	void on_accept_conn(int lfd, int events, void* arg);
	void on_recv_data(int cfd, int events, void* arg);
	int epoll_start();
};

