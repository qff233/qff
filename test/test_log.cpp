#include <time.h>
#include "log.h"
#include "thread.h"
#include <iostream>
#include <unistd.h>

int count = 0;

void run() {
	for (size_t i = 0; i < 20; i++) {
		++count;
	}
	
}

int main() {
	// qff::LoggerMgr::New();

	// qff::FileLogAppender::ptr appender 
	// 	= std::make_shared<qff::FileLogAppender>("file", qff::LogLevel::DEBUG, "test.log");
	// qff::LoggerMgr::Get()->get_root()->add_appender(appender);

	qff::Thread thread(run, "test");

	// for(size_t i = 0; i < 10; ++i) {
	// 	std::cerr << "aaa" << "bbb" << "ccc" << std::endl;
	// 	sleep(2);
	// }
	for (size_t i = 0; i < 20; i++) {
		++count;
	}
	
	thread.join();

	// qff::LoggerMgr::Delete();
	std::cout << count << std::endl;
	return 0;
}
