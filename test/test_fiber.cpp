#include "scheduler.h"
#include "log.h"
#include <unistd.h>

using namespace qff;

void test() {
    QFF_LOG_DEBUG(QFF_LOG_ROOT) << "2";
    Fiber::YieldToReady();
    QFF_LOG_DEBUG(QFF_LOG_ROOT) << "4";
}

int main() {
    LoggerMgr::New();
    
    Scheduler::ptr scheduler 
        = std::make_shared<Scheduler>(1, "main", false);
    scheduler->schedule(test);
    scheduler->schedule(test);
    //scheduler->schedule(test);

    scheduler->start();
    QFF_LOG_DEBUG(QFF_LOG_ROOT) << "started";

    scheduler->stop();

    QFF_LOG_DEBUG(QFF_LOG_ROOT) << "stopstopstopstop";
    LoggerMgr::Delete();
    return 0;
}