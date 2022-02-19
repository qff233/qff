#include "scheduler.h"
#include "log.h"
#include <unistd.h>
#include <string.h>

using namespace qff;

void test() {
    QFF_LOG_DEBUG(QFF_LOG_ROOT) << "2";
}

int main() {
    LoggerMgr::New();
    
    Fiber::Init(2, 1024*1024);

    std::vector<Fiber::ptr> vec;
    for(size_t i = 0; i < 10; ++i) {
        Fiber::ptr fiber = std::make_shared<Fiber>(test);
        vec.push_back(fiber);
    }
    
    for(auto i : vec) {
        i->swap_in();
    }
    
    for(size_t i = 0; i < 10; ++i) {
        vec[i] = std::make_shared<Fiber>(test);
    }
    for(auto i : vec) {
        i->swap_in();
    }

    // Test test(1024*1024*3);
    // void* ptr = test.get_memory();
    // QFF_LOG_DEBUG(QFF_LOG_ROOT) <<  ptr;
    // free(ptr);

    QFF_LOG_DEBUG(QFF_LOG_ROOT) << "stopstopstopstop";
    //LoggerMgr::Delete();
    return 0;
}