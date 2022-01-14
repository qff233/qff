#include <time.h>
#include "log.h"


int main() {
	qff::LoggerMgr::New();

	QFF_LOG_SYSTEM("log_event1")
	QFF_LOG_ROOT("log_event2")
	return 0;
}
