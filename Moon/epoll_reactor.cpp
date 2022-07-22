
#include "epoll_reactor.h"
#include <sstream>
#include <string>
#include <iostream>

epoll_reactor::epoll_reactor()
{
	init();
}

void epoll_reactor::init()
{
	/* 初始化读写锁 */
	pthread_rwlock_init(&self_evnets_ctl_rw_lock, NULL);
}

self_event*  epoll_reactor::get_self_event(int fd)
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

void epoll_reactor::init_socket() {
	{
		g_lfd = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in servaddr;
		bzero(&servaddr, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servaddr.sin_port = htons(port);

		int opt = 1;
#ifdef _DEBUG
		setsockopt(g_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)); // 端口复用, 在本地调试下才开这个
#endif
		NetworkUtils::my_bind(g_lfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
		NetworkUtils::my_listen(g_lfd, 20);

		/* 初始化监听fd */
		self_event* l_event = new self_event(g_lfd, nullptr);
		add_self_event(g_lfd, l_event);
		//self_events[MAX_EVENTS] = self_event(g_lfd, nullptr);
		event_add(EPOLLIN | EPOLLET, get_self_event(g_lfd)); // 将结构体加入到epoll中
	}

};
/* 操作epoll红黑树 */
void epoll_reactor::event_add(int events, self_event* se) {
	struct epoll_event epv = { 0, {0} };
	epv.data.ptr = se; // 泛型参数
	epv.events = events; // 具体事件
	se->events = events;
	//se->status = 1; // 事件备份 
	if (epoll_ctl(g_epfd, EPOLL_CTL_ADD, se->fd, &epv) == -1) {
		perror(strerror(errno));
		NetworkUtils::print_backtrace();
		exit(-1);
	}
	// printf("加入的fd[%d]\n", se->fd);
}
void epoll_reactor::event_modify(int events, self_event* se) {
	struct epoll_event epv = { 0, {0} };
	se->events = events;  // 事件备份
	epv.events = events; // 具体事件
	epv.data.ptr = se; // 泛型参数
	if (epoll_ctl(g_epfd, EPOLL_CTL_MOD, se->fd, &epv) == -1) { //  Change the event event associated with the target file descriptor fd.
		perror(strerror(errno));
		NetworkUtils::print_backtrace();
		exit(-1);
	}
}

void epoll_reactor::event_remove(self_event* ev) {

	remove_self_event(ev->fd);
	int res = epoll_ctl(g_epfd, EPOLL_CTL_DEL, ev->fd, NULL);
}

void epoll_reactor::on_send_data(int cfd, int events, void* arg)
{
	self_event* se = (self_event*)arg;
	// 已经被关闭了
	if (get_self_event(cfd) == nullptr) {
		se->client.m_write_buffer.clear_queue();
		return;
	}

	int len = se->client.m_write_buffer.Write(cfd);

	if (len < 0) {
		NetworkUtils::print_backtrace();
		event_remove(se);
		close(cfd);
		se->client.m_write_buffer.clear_queue();
		return;
	}
	///
	if (len == 0) {
		//se->callback = on_recv_data;
		event_modify(EPOLLIN | EPOLLET, se);
	}

}

void epoll_reactor::on_accept_conn(int lfd, int events, void* arg)
{
	int cfd = NetworkUtils::my_accept(lfd, NULL, NULL);
	// // 设置为读非阻塞
	int cur_flag = fcntl(cfd, F_GETFL);
	fcntl(cfd, F_SETFL, cur_flag | O_NONBLOCK); // ET模式下要用非阻塞，防止while循环读取全部数据后卡死线程，从而也可以得知一个线程的epoll虽然可以处理很多连接，但是每个连接的读写，连接管理都是一个线程在做，在读的时候可能会卡死其他线程

	if (self_events.size() >= MAX_EVENTS) {
		log_error("连接池已满");
		return;
	}
	self_event* client = new self_event(cfd, NULL);
	add_self_event(cfd, client);// 初始化lfd的event结构体
	//self_events[cfd] = ; 
	//self_event_constructor(&self_events[index], cfd, on_recv_data, NULL); // 初始化lfd的event结构体
	event_add(EPOLLIN | EPOLLET, self_events[cfd]); // 将结构体加入到epoll中
}

void epoll_reactor::on_recv_data(int cfd, int events, void* arg)
{
	self_event* se = (self_event*)arg;
	/* 这里的读回调函数不再做业务了，只做read的异常处理 */
	auto ret = se->client.m_read_buffer.Read(cfd);
	if (ret.first <= 0) {
		/* remove要在close前做，能防止多线程造成的增删冲突*/
		event_remove(se);
		int res = close(cfd);
		printf("关闭的cfd=[%d] ret=[%d]\n", cfd, res);
		return;
	}

	/* 监听事件转换 */
	if (ret.first == 2) {
		se->client.m_write_buffer.push_response(ret.second);
		//se->callback = on_send_data;
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

	init_socket();

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
			//if (se->events != se->events) {
			//	//    printf("警告！");
			//}
			if (events[i].events & EPOLLHUP || events[i].events & EPOLLERR) {
				std::stringstream ss;
				ss << (events[i].events & EPOLLHUP ? "[EPOLLHUP] " : "") << (events[i].events & EPOLLERR ? "[EPOLLERR]" : "");
				log_error(ss.str().data());
			}
			else if (events[i].events & EPOLLIN) { // on_accept_conn或on_recv_data
				std::cout << (se->fd == g_lfd ? "listen event" : "read event") << std::endl;
				job_thread_pool->add_task([&]() -> void {
					if (se->fd == g_lfd) { // 连接事件
						on_accept_conn(se->fd, se->events, se);
					}
					else {
						on_recv_data(se->fd, se->events, se);
					}
					});
			}
			if (events[i].events & EPOLLOUT) { // on_send_data
				std::cout << "write event" << std::endl;
				job_thread_pool->add_task([&]() -> void {
					on_send_data(se->fd, se->events, se);
					});
			}
		}
	}
	return 0;

}


/* self_event:start*/
self_event::self_event(int fd, void* arg) :fd(fd), arg(arg) {
	init();
}
self_event::self_event(const  self_event& s) : fd(s.fd), arg(s.arg) {
	init();
}

self_event& self_event::operator=(const self_event& s)
{
	fd = s.fd;
	arg = s.arg;
	//callback = s.callback;
	init();
}
/* self_event:end*/
