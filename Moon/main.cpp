#include <iostream>
#include "GlobalService.h"
#include "Worker.h"
#include <unistd.h>

#include "Ping.h"


/*
    架构，线程池，全局消息队列，
    每一个服务一个队列

*/
int main()
{
    GlobalService* global_service = GlobalService::instance();
    Ping p1("ping1");
    Ping p2("ping2");
    global_service->services["ping1"] = &p1;
    global_service->services["ping2"] = &p2;
    p1.send("ping2", "ping");
    // TODO 主线程用于接收网络请求
    while (true)
    {
        // TODO 定时检查
        sleep(1000);

    }

    return 0;
}