#include "log.h"
#include "io_manager.h"
#include "hook.h"

#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <sys/epoll.h>

using namespace qff;

void test() {
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    ::fcntl(sock, F_SETFL, O_NONBLOCK);

    ::sockaddr_in addr;
    ::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    ::inet_pton(AF_INET, "14.215.177.39", &addr.sin_addr.s_addr);

    ::connect(sock, (const sockaddr*)&addr, sizeof(sockaddr));
    char buf[4096] = "GET / HTTP/1.1\r\n"
                "Host:www.baidu.com\r\n\r\n";
    int rt = ::send(sock, buf, sizeof(buf), 0);
    if(rt < 0)
         QFF_LOG_DEBUG(QFF_LOG_ROOT) << "?";
    rt = ::recv(sock, buf, 4096, 0);
         
    if(rt < 0)
        QFF_LOG_DEBUG(QFF_LOG_ROOT) << "???   " << errno << strerror(errno);

    ::close(sock);

    QFF_LOG_INFO(QFF_LOG_ROOT) << buf;
}

void test2() {
    QFF_LOG_DEBUG(QFF_LOG_ROOT) << "123";
    ::sleep(1);
    QFF_LOG_DEBUG(QFF_LOG_ROOT) << "456";
    ::sleep(3);
    QFF_LOG_DEBUG(QFF_LOG_ROOT) << "789";
}

void test3() {
    IOManager::ptr iom = std::make_shared<IOManager>(1, "main");
    set_hook_enable(true);
    //iom->schedule(test2);
    iom->schedule(test);
}

int main() {
    LoggerMgr::New();
    test3();
    return 0;
}