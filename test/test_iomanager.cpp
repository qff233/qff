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

    sockaddr_in addr;
    ::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "14.215.177.39", &addr.sin_addr.s_addr);

    if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
    } else if(errno == EINPROGRESS) {
        QFF_LOG_INFO(QFF_LOG_ROOT) << "add event errno=" << errno << " " << strerror(errno);
        IOManager::GetThis()->add_event(sock, E_T_READ, [](){
            QFF_LOG_INFO(QFF_LOG_ROOT) << "read callback";
        });
        IOManager::GetThis()->add_event(sock, E_T_WRITE, [=](){
            QFF_LOG_INFO(QFF_LOG_ROOT) << "write callback";
            IOManager::GetThis()->cancel_event(sock, E_T_READ);
            close(sock);
        });
    } else {
        QFF_LOG_INFO(QFF_LOG_ROOT) << "else " << errno << " " << strerror(errno);
    }
}

int main() {
    LoggerMgr::New();

    IOManager::ptr iom = std::make_shared<IOManager>(1, "main");
    iom->schedule(test);
    iom->start();
    QFF_LOG_DEBUG(QFF_LOG_ROOT) << "789";
    iom->stop();
    LoggerMgr::Delete();
    return 0;
}