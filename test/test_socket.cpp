#include "address.h"
#include "log.h"
#include "hook.h"
#include "io_manager.h"
#include "socket.h"

#include <vector>
#include <arpa/inet.h>

using namespace qff;

void test123() {
    std::vector<qff::Address::ptr> addrs;

    QFF_LOG_INFO(QFF_LOG_ROOT) << "begin";
    addrs = qff::Address::Lookup("www.baidu.com");
    //bool v = sylar::Address::Lookup(addrs, "www.baidu.com", AF_INET);
    QFF_LOG_INFO(QFF_LOG_ROOT) << "end";
    if(addrs.empty()) {
        QFF_LOG_INFO(QFF_LOG_ROOT) << "lookup fail";
        return;
    }

    for(size_t i = 0; i < addrs.size(); ++i) {
        QFF_LOG_INFO(QFF_LOG_ROOT) << i << " - " << *addrs[i];
    }

    auto addr = qff::Address::LookupAny("www.baidu.com");
    if(addr) {
        QFF_LOG_INFO(QFF_LOG_ROOT) << *addr;
    } else {
        QFF_LOG_INFO(QFF_LOG_ROOT) << "error";
    }
}

void test_iface() {
    auto results = qff::Address::GetInterfaceAddresses();

    if(results.empty()) {
        QFF_LOG_INFO(QFF_LOG_ROOT) << "GetInterfaceAddresses fail";
        return;
    }

    for(auto& i: results) {
        auto[one,two,three] = i;
        QFF_LOG_INFO(QFF_LOG_ROOT) << one << " - " << *two << " - " << three;
    }
}

void test_ipv4() {
    //auto addr = qff::IPAddress::Create("127.0.0.1");
    auto addr = qff::IPAddress::Create("127.0.0.2");
    if(addr) {
        QFF_LOG_INFO(QFF_LOG_ROOT) << *addr;
    }
}

void test1() {
    // IPAddress::ptr addr1 = IPAddress::LookupAny("www.baidu.com");
    // if(addr1) {
    //     QFF_LOG_INFO(QFF_LOG_ROOT) << "get address: " << *addr1;
    // } else {
    //     QFF_LOG_INFO(QFF_LOG_ROOT) << "get address fail";
    //     return;
    // }
    IPAddress::ptr addr = IPAddress::Create("220.181.38.251", 80);
    qff::Socket::ptr sock = std::make_shared<Socket>(Socket::IPv4, Socket::TCP);
    addr->set_port(80);
    QFF_LOG_INFO(QFF_LOG_ROOT) << "addr=" << *addr;
    if(sock->connect(addr)) {
        QFF_LOG_INFO(QFF_LOG_ROOT) << "connect " << *addr << " fail";
        return;
    } else {
        QFF_LOG_INFO(QFF_LOG_ROOT) << "connect " << *addr << " connected";
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if(rt <= 0) {
        QFF_LOG_INFO(QFF_LOG_ROOT) << "send fail rt=" << rt;
        return;
    }

    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());

    if(rt <= 0) {
        QFF_LOG_INFO(QFF_LOG_ROOT) << "recv fail rt=" << rt;
        return;
    }

    buffs.resize(rt);
    QFF_LOG_INFO(QFF_LOG_ROOT) << buffs;
}

void test2() {
    IPAddress::ptr addr = IPAddress::LookupAny("qff233.com");
    if(addr) {
        QFF_LOG_INFO(QFF_LOG_ROOT) << "get address: " << *addr;
    } else {
        QFF_LOG_INFO(QFF_LOG_ROOT) << "get address fail";
        return;
    }
    addr->set_port(80);

    Socket::ptr sock = std::make_shared<Socket>(Socket::IPv4, Socket::TCP);
    if(sock->connect(addr)) {
        QFF_LOG_INFO(QFF_LOG_ROOT) << "connect " << *addr << " fail";
        return;
    } else {
        QFF_LOG_INFO(QFF_LOG_ROOT) << "connect " << *addr << " connected";
    }

    uint64_t ts = GetCurrentUS();
    for(size_t i = 0; i < 10000000000ul; ++i) {
        if(int err = sock->get_error()) {
            QFF_LOG_INFO(QFF_LOG_ROOT) << "err=" << err << " errstr=" << strerror(err);
            break;
        }
        static int batch = 10000000;
        if(i && (i % batch) == 0) {
            uint64_t ts2 = GetCurrentUS();
            QFF_LOG_INFO(QFF_LOG_ROOT) << "i=" << i << " used: " << ((ts2 - ts) * 1.0 / batch) << " us";
            ts = ts2;
        }
    }
}

void test123123123() {
    QFF_LOG_INFO(QFF_LOG_ROOT) << 123;
}


int main(int argc, char** argv) {
    qff::LoggerMgr::New();
    //test_ipv4();
    //test_iface();
    set_hook_enable(true);
    IOManager iom(1, "main");
    iom.schedule(test1);
    //iom.schedule(test123123123);
    return 0;
}