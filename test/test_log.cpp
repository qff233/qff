#include <time.h>
#include "log.h"

#include <iostream>

int main() {
	qff::Logger logger("ROOT", qff::LogLevel::DEBUG, "[%d] [%T] [%i] [%f] %F:%L   %m");
	qff::LogEvent::ptr log_event 
		= std::make_shared<qff::LogEvent>("UNKNOW", -1, -1, qff::LogLevel::DEBUG
											, __FILE__, __LINE__, time(0), "Hello World 123123123");
	logger.log(log_event);
	return 0;
}
