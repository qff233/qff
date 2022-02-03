#include "thread.h"
#include "log.h"

#include <unistd.h>

using namespace qff;

void run() {
    for (size_t i = 0; i < 20; i++) {
        QFF_LOG_INFO(QFF_LOG_ROOT) << "name:" << Thread::GetName();
        ::sleep(2);
    }
}

int main() {
    LoggerMgr::New();

    std::vector<Thread::ptr> threads;
    for (size_t i = 0; i < 2; i++) {
        std::string name = "thread_" + std::to_string(i);
        Thread::ptr thread = std::make_shared<Thread>(run, name);
        threads.push_back(thread);
    }
    
    for(auto i : threads) {
        i->join();
    }

    LoggerMgr::Delete();
    return 0;
}