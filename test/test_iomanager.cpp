#include "log.h"
#include "io_manager.h"

#include <sys/types.h>
#include <sys/socket.h>
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

    if(!::connect(sock, (const sockaddr*)&addr, sizeof(sockaddr))) {
    } else if(errno == EINPROGRESS) {
        QFF_LOG_INFO(QFF_LOG_ROOT) << "add event errno=" << errno << " " << strerror(errno);
        IOManager::GetThis()->add_event(sock, IOManager::READ, [=](){
            QFF_LOG_INFO(QFF_LOG_ROOT) << "read callback";
            //char buf[1024];
            //::recv(sock, buf, 1024, 0);
           // close(sock);
            //QFF_LOG_INFO(QFF_LOG_ROOT) << buf;
        });
        IOManager::GetThis()->add_event(sock, IOManager::WRITE, [=](){
            QFF_LOG_INFO(QFF_LOG_ROOT) << "write callback";
            //char buf[] = "GET / HTTP/1.1\r\n"
            //             "Host:www.baidu.com\r\n\r\n";
            //::send(sock, buf, sizeof(buf), 0);
            IOManager::GetThis()->cancel_event(sock, IOManager::READ);
            close(sock);
        });
    } else {
        QFF_LOG_INFO(QFF_LOG_ROOT) << "else " << errno << " " << strerror(errno);
    }
}

int main() {
    LoggerMgr::New();

    IOManager::ptr iom = std::make_shared<IOManager>(2, "main");
    iom->schedule(test);
    iom->schedule(test);
    iom->schedule(test);
    iom->schedule(test);
    iom->schedule(test);
    iom->schedule(test);
    QFF_LOG_DEBUG(QFF_LOG_ROOT) << "789";
    iom->stop();
    LoggerMgr::Delete();
    return 0;
}