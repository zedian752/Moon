#include <iostream>
#include "GlobalService.h"
#include "Worker.h"
#include <unistd.h>
#include <algorithm> 
#include "Ping.h"
#include <time.h>

/*
    架构，线程池，全局消息队列，
    每一个服务一个队列

*/
int main()
{
    GlobalService* global_service = GlobalService::instance();
    Ping * p1 = new Ping("ping1");
    Ping * p2 = new Ping("ping2");
    Ping * p3 = new Ping("ping3");
    Ping * p4 = new Ping("ping4");
    global_service->newservice("ping1", p1);
    global_service->newservice("ping2", p2);
    global_service->newservice("ping3", p3);
    global_service->newservice("ping4", p4);

    p1->send("ping2", "ping");
    // TODO 主线程用于接收网络请求
    while (true)
    {
        usleep(1000); // 1000us = 1ms
        if (!global_service->global_queue.empty()) {
            auto idle_num = global_service->total_worker_amount - global_service->worker_num.load();
            if (idle_num > 0) {
                int trigger_num  = std::min( static_cast<int>(global_service->global_queue.size()), idle_num);
                while (trigger_num-- > 0) {
                    printf("通过while循环触发 %d\n", trigger_num);
                    global_service->cv_ready.notify_one();
                }
            }
        }
    }

    return 0;
}