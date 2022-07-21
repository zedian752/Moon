
#include "epoll_reactor.h"

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
		self_events[g_lfd] = l_event;
		//self_events[MAX_EVENTS] = self_event(g_lfd, nullptr);
		event_add(EPOLLIN | EPOLLET, self_events[g_lfd]); // 将结构体加入到epoll中
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
	self_events.erase(ev->fd);
	delete self_events[ev->fd];
	int res = epoll_ctl(g_epfd, EPOLL_CTL_DEL, ev->fd, NULL);
}

void epoll_reactor::on_send_data(int cfd, int events, void* arg)
{
	self_event* se = (self_event*)arg;
	int idx = 0;
	int len = se->client.m_write_buffer.Write(cfd);

	if (len < 0) {
		NetworkUtils::print_backtrace();
		close(cfd);
		event_remove(se);
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
	self_events[cfd] = new self_event(cfd, NULL); // 初始化lfd的event结构体
	//self_event_constructor(&self_events[index], cfd, on_recv_data, NULL); // 初始化lfd的event结构体
	event_add(EPOLLIN | EPOLLET, self_events[cfd]); // 将结构体加入到epoll中
}

void epoll_reactor::on_recv_data(int cfd, int events, void* arg)
{
	self_event* se = (self_event*)arg;
	/* 这里的读回调函数不再做业务了，只做read的异常处理 */
	auto ret = se->client.m_read_buffer.Read(cfd);
	if (ret.first <= 0) {
		event_remove(se);
		int res = close(cfd);
		printf("关闭的cfd=[%d] ret=[%d]\n", cfd, res);
		return;
	}

	/* 监听事件转换 */
	if (ret.first == 2) {
		se->client.m_write_buffer.push_response(ret.second);
		//se->callback = on_send_data;
		event_modify(EPOLLOUT | EPOLLET, se);

	}

}

int epoll_reactor::epoll_start()
{
	//bzero(self_events, MAX_EmVENTS * sizeof(self_event));
	//epoll_reactor er;
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

			if (events[i].events & EPOLLIN) { // on_accept_conn或on_recv_data
				log_info("read event");
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
//self_event::self_event() {}

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
