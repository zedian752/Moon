
#include "epoll_reactor.h"
#include <sstream>
#include <string>
#include <iostream>
#include "UdpWriteBuffer.h"

epoll_reactor::epoll_reactor()
{
	init();
}

epoll_reactor::epoll_reactor(EPOLL_SOCKET_MODE mode)
{
	init();
	this->mode |= mode;
}

void epoll_reactor::init()
{
	/* 初始化读写锁 */
	pthread_rwlock_init(&self_evnets_ctl_rw_lock, NULL);
}

self_event* epoll_reactor::get_self_event(int fd)
{
	self_event* se = nullptr;
	pthread_rwlock_rdlock(&self_evnets_ctl_rw_lock);
	se = self_events[fd];
	pthread_rwlock_unlock(&self_evnets_ctl_rw_lock);
	return se;
}

int epoll_reactor::remove_self_event(int fd)
{
	pthread_rwlock_wrlock(&self_evnets_ctl_rw_lock);
	{
		self_events.erase(fd);
		delete self_events[fd];
	}
	pthread_rwlock_unlock(&self_evnets_ctl_rw_lock);

	return 0;
}

int epoll_reactor::add_self_event(int fd, self_event*& e)
{
	if (e == nullptr) {
		return -1;
	}
	pthread_rwlock_wrlock(&self_evnets_ctl_rw_lock);
	{
		self_events[fd] = e;
	}
	pthread_rwlock_unlock(&self_evnets_ctl_rw_lock);

	return 0;
}

int epoll_reactor::init_udp_socket()
{
	int lfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (lfd < 0)
	{
		log_error("udp socket error");
		return -1;
	}

	g_udp_lfd = lfd;

	//绑定
	struct sockaddr_in serv;
	struct sockaddr_in client;
	bzero(&serv, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_port = htons(udp_port);
	serv.sin_addr.s_addr = htonl(INADDR_ANY);
	NetworkUtils::my_bind(lfd, (struct sockaddr*)&serv, sizeof(serv));

	/* 初始化监听fd */
	self_event* l_event = new self_event(g_udp_lfd, nullptr);
	l_event->type = EPOLL_SOCKET_MODE::UDP;
	event_add(EPOLLIN | EPOLLET, l_event); // 将结构体加入到epoll中

}

void epoll_reactor::init_tcp_socket() {
	{
		g_tcp_lfd = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in servaddr;
		bzero(&servaddr, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servaddr.sin_port = htons(tcp_port);

		int opt = 1;
#ifdef _DEBUG
		setsockopt(g_tcp_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)); // 端口复用, 在本地调试下才开这个
#endif
		NetworkUtils::my_bind(g_tcp_lfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
		NetworkUtils::my_listen(g_tcp_lfd, 20);

		/* 初始化监听fd */
		self_event* l_event = new self_event(g_tcp_lfd, nullptr);
		add_self_event(g_tcp_lfd, l_event);
		event_add(EPOLLIN | EPOLLET, l_event); // 将结构体加入到epoll中
	}

};
/* 操作epoll红黑树 */
void epoll_reactor::event_add(int events, self_event* se) {
	struct epoll_event epv = { 0, {0} };
	epv.data.ptr = se; // 泛型参数
	epv.events = events; // 具体事件
	se->events = events;

	if (epoll_ctl(g_epfd, EPOLL_CTL_ADD, se->fd, &epv) == -1) {
		perror(strerror(errno));
		NetworkUtils::print_backtrace();
		exit(-1);
	}
}
void epoll_reactor::event_modify(int events, self_event* se) {
	struct epoll_event epv = { 0, {0} };
	se->events = events;  // 事件备份
	epv.events = events; // 具体事件
	epv.data.ptr = se; // 泛型参数
	if (epoll_ctl(g_epfd, EPOLL_CTL_MOD, se->fd, &epv) == -1) {
		perror(strerror(errno));
		NetworkUtils::print_backtrace();
		exit(-1);
	}
}

void epoll_reactor::event_remove(self_event* ev) {

	remove_self_event(ev->fd);
	int res = epoll_ctl(g_epfd, EPOLL_CTL_DEL, ev->fd, NULL);
}

void epoll_reactor::on_send_data(int cfd, void* arg)
{
	self_event* se = (self_event*)arg;
	// 已经被关闭了
	if (get_self_event(cfd) == nullptr) {
		se->client->m_write_buffer.m_tcp_write_buffer->clear_queue();
		return;
	}

	int len = se->client->m_write_buffer.m_tcp_write_buffer->Write(cfd);

	if (len < 0) {
		NetworkUtils::print_backtrace();
		event_remove(se);
		close(cfd);
		se->client->m_write_buffer.m_tcp_write_buffer->clear_queue();
		return;
	}
	//
	if (len == 0) {
		event_modify(EPOLLIN | EPOLLET, se);
	}

}

void epoll_reactor::on_accept_conn(int lfd, void* arg)
{
	int cfd = NetworkUtils::my_accept(lfd, NULL, NULL);
	// // 设置为读非阻塞
	int cur_flag = fcntl(cfd, F_GETFL);
	fcntl(cfd, F_SETFL, cur_flag | O_NONBLOCK); // ET模式下要用非阻塞，防止while循环读取全部数据后卡死线程，从而也可以得知一个线程的epoll虽然可以处理很多连接，但是每个连接的读写，连接管理都是一个线程在做，在读的时候可能会卡死其他线程

	if (self_events.size() >= MAX_EVENTS) {
		log_error("epoll事件池已满");
		return;
	}
	self_event* client = new self_event(cfd, NULL);
	add_self_event(cfd, client);// 初始化lfd的event结构体
	event_add(EPOLLIN | EPOLLET, self_events[cfd]); // 将结构体加入到epoll中
}

void epoll_reactor::on_recv_data(int cfd, void* arg)
{
	self_event* se = (self_event*)arg;
	// 已经被关闭了
	if (get_self_event(cfd) == nullptr) {
		se->client->m_write_buffer.m_tcp_write_buffer->clear_queue();
		return;
	}
	/* 这里的读回调函数不再做业务了，只做read的异常处理 */
	auto ret = se->client->m_read_buffer.m_tcp_read_buffer->Read(cfd);
	if (ret.first <= 0) {
		/* remove要在close前做，能防止多线程造成的增删冲突*/
		event_remove(se);
		int res = close(cfd);
		printf("关闭的cfd=[%d] ret=[%d]\n", cfd, res);
		return;
	}

	/* 监听事件转换 */
	if (ret.first == 2) {
		se->client->m_write_buffer.m_tcp_write_buffer->push_response(ret.second);
		event_modify(EPOLLOUT | EPOLLET | EPOLLIN, se);
	}

}

void epoll_reactor::on_udp_send_data(int cfd, void* arg)
{

	self_event* se = (self_event*)arg;

	int len = se->client->m_write_buffer.m_udp_write_buffer->Write(cfd);

	if (len < 0) {
		NetworkUtils::print_backtrace();
		event_remove(se);
		close(cfd);
		se->client->m_write_buffer.m_udp_write_buffer->clear_queue();
		return;
	}
	///
	if (len == 0) {
		event_modify(EPOLLIN | EPOLLET, se);
	}

}

void epoll_reactor::on_udp_recv_data(int cfd, void* arg)
{
	self_event* se = (self_event*)arg;
	/* 这里的读回调函数不再做业务了，只做read的异常处理 */
	auto ret = se->client->m_read_buffer.m_udp_read_buffer->Read(cfd);
	if (ret.first <= 0) {

		return;
	}

	/* 监听事件转换 */
	if (ret.first == 2) {
		se->client->m_write_buffer.m_udp_write_buffer->push_response(ret.second);
		event_modify(EPOLLOUT | EPOLLET | EPOLLIN, se);
	}


}

int epoll_reactor::epoll_start()
{
	g_epfd = epoll_create(MAX_EVENTS + 1);
	if (g_epfd == -1) {
		perror("");
		return -1;
	}
	if (mode & EPOLL_SOCKET_MODE::TCP) {
		//init_tcp_socket();
	}

	if (mode & EPOLL_SOCKET_MODE::UDP) {
		init_udp_socket();
	}

	struct epoll_event events[MAX_EVENTS + 1];

	while (1) {
		int nfd = epoll_wait(g_epfd, events, MAX_EVENTS, -1); // 阻塞等待
		if (nfd < 0) {
			if (errno == EINTR) // 处理信号打断
				continue;
			break;
		}
		for (int i = 0; i < nfd; i++) {
			self_event* se = (self_event*)events[i].data.ptr;

			if (events[i].events & EPOLLHUP || events[i].events & EPOLLERR) {
				std::stringstream ss;
				ss << (events[i].events & EPOLLHUP ? "[EPOLLHUP] " : "") << (events[i].events & EPOLLERR ? "[EPOLLERR]" : "");
				log_error(ss.str().data());
			}
			else if (events[i].events & EPOLLIN) { // on_accept_conn或on_recv_data
				std::cout << (se->fd == g_tcp_lfd ? "listen event" : "read event") << std::endl;
				job_thread_pool->add_task([&]() -> void {
					if (se->fd == g_tcp_lfd) { // 连接事件
						on_accept_conn(se->fd, se);
					}
					else {

						if (se->type == EPOLL_SOCKET_MODE::TCP) {
							on_recv_data(se->fd, se);

						}
						else {
							on_udp_recv_data(se->fd, se);
						}

					}
					});
			}
			if (events[i].events & EPOLLOUT) {
				std::cout << "write event" << std::endl;
				job_thread_pool->add_task([&]() -> void {

					if (se->type == EPOLL_SOCKET_MODE::TCP) {
						on_send_data(se->fd, se);
					}
					else if (se->type == EPOLL_SOCKET_MODE::UDP) {
						on_udp_send_data(se->fd, se);
					}
					});
			}
		}
	}
	return 0;

}


void self_event::init()
{
	//last_active = time(NULL);
	client = new Client(type);

}

/* self_event:start*/
self_event::self_event(int fd, void* arg) :fd(fd), arg(arg) {
	init();
}
self_event::self_event(int fd, void* arg, EPOLL_SOCKET_MODE type) : type(type)
{
	init();
}
self_event::self_event(const  self_event& s) : fd(s.fd), arg(s.arg) {
	init();
}

self_event& self_event::operator=(const self_event& s)
{
	fd = s.fd;
	arg = s.arg;
	init();
}
self_event::~self_event()
{
	delete client;
}
/* self_event:end*/
