#pragma once
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <execinfo.h>
#include <sys/socket.h>

class NetworkUtils
{
public:
    static int my_bind(int __fd, const struct sockaddr* _addr, socklen_t __len) {
        int n = bind(__fd, _addr, __len);
        checkError(n);
        return n;
    };
    static int my_listen(int fd, int backlog) {
        int n = listen(fd, backlog);
        checkError(n);
        return n;

    };

    static int my_accept(int fd, struct sockaddr* sa, socklen_t* salenptr) {
        int n;
    again:
        if ((n = accept(fd, sa, salenptr)) < 0) {
            if (errno == ECONNABORTED || errno == EINTR) { // 由于server发送了一个rst给已经建立了三次握手的服务端引起的abort，或信号中断
                goto again;
            }
            else {
                checkError(n);
            }
        }
        return n;
    };

    int my_connect(int fd, const struct sockaddr* sa, socklen_t salen) {
        int n = connect(fd, sa, salen);
        checkError(n);
        return n;
    };



    void static checkError(int return_no) {
        if (return_no != 0) {
            printf("退出编码：[%d] %s\n", return_no, strerror(errno));
            print_backtrace();
            exit(-1);
        }
    }

    /* 
        函数print_backtrace在调试时需要加上以下选项
        - g - rdynamic 
    */
    void static print_backtrace() {
        const unsigned int SIZE = 100;
        void* buffer[SIZE];
        char** strings;
        int nptrs = backtrace(buffer, SIZE);
        printf("backtrace() returned %d addresses\n", nptrs);
        strings = backtrace_symbols(buffer, nptrs);
        if (strings == NULL) {
            perror("backtrace_symbols");
            exit(EXIT_FAILURE);
        }
        for (int j = 0; j < nptrs; j++)
            printf("%s\n", strings[j]);
        free(strings);
    }
};

