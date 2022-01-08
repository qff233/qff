#include "log.h"

#include <algorithm>

namespace qff {

std::string LogLevel::ToString(Level level) {
	switch(level) {
	case DEBUG:
		return "DEBUG";
	case INFO:
		return "INFO";
	case WARN:
		return "WARN";
	case ERROR:
		return "ERROR";
	case FATAL:
		return "FATAL";
	}
	return "UNKNOW";
}

LogLevel::Level LogLevel::FromString(const std::string& str) {
	std::string str_up = str;
	std::transform(str_up.begin(), str_up.end(), str_up.begin(), ::toupper);
	if(str == "DEBUG") return DEBUG;
	if(str == "INFO") return INFO;
	if(str == "WARN") return WARN;
	if(str == "ERROR") return ERROR;
	if(str == "FATAL") return FATAL;
	return FATAL;
}

LogEvent::LogEvent(const std::string& thread_name, int thread_id
				, int fiber_id, LogLevel::Level level
				, const std::string& file_name, int line
				, time_t time, const std::string& content)
	:m_thread_name(thread_name)
	,m_thread_id(thread_id)
	,m_fiber_id(fiber_id)
	,m_level(level)
	,m_file_name(file_name)
	,m_line(line)
	,m_time(time) 
	,m_content(content){
}

LogAppender::LogAppender(const std::string& name, LogLevel::Level level) 
	:m_name(name)
	,m_level(level) {
}

FileLogAppender::FileLogAppender(const std::string& name
								, LogLevel::Level level) 
	:LogAppender(name, level) {
}

void FileLogAppender::output(const std::string& str) {
}

StandLogAppender::StandLogAppender(const std::string& name
								, LogLevel::Level level)
	:LogAppender(name, level) {
}

void StandLogAppender::output(const std::string& str) {
	printf("%s", str.c_str());
}


//%n 换行
//%t 四个空格
//%m 内容
//%p 级别
//%T 线程名
//%% 百分号
//%d 日期和时间
//%F 源文件名称
//%L 代码所在行数
Logger::Logger(const std::string& name, LogLevel::Level level, const std::string& format) 
	:m_name(name)
	,m_level(level) {
	this->set_format(format);
}

void Logger::set_format(const std::string& format) {
	enum {
		eTheradName = 0, 
		eThreadId   = 1, 
		eFiberId	= 2, 
		eLevel      = 3, 
		eFileName   = 4, 
		eLine       = 5, 
		eTime       = 6, 
		e2Block     = 7, 
		eNext       = 8,
		ePercent    = 9,
		eContent    = 10
	};

	for(auto ptr = format.begin(); ptr != format.end(); ++ptr) {
		auto next_ptr = ptr + 1;
		if(next_ptr == format.end()) break;

		if(*ptr == '%') {
			switch(*next_ptr) {
			case 'n':
				m_format.push_back(eNext);
			case 't':
				m_format.push_back(e2Block);
				m_format.push_back(e2Block);
			case 'm':
				m_format.push_back(eContent);
			case 'p':
				m_format.push_back(eLevel);
			case 'T':
				m_format.push_back(eTheradName);
			case '%':
				m_format.push_back(ePercent);
			case 'd':
				m_format.push_back(eTime);
			case 'F':
				m_format.push_back(eFileName);
			case 'L':
				m_format.push_back(eLine);
			default:
				continue;
			}
		}

		int i = 1;
		for(auto j = next_ptr; j != format.end() && *j != '%'; ++j)
			++i;
		for(int j = 0; j < i/2 + i%2; ++j) {
			m_format.push_back(e2Block);
		}
		ptr = ptr -1 + i;
	}
}

void Logger::log(LogEvent::ptr event) {
	//eTheradName = 0, 
	//eThreadId   = 1, 
	//eFiberId	  = 2, 
	//eLevel      = 3, 
	//eFileName   = 4, 
	//eLine       = 5, 
	//eTime       = 6, 
	//e2Block     = 7, 
	//eNext       = 8,
	//ePercent    = 9,
	//eContent    = 10
	std::string content;
	for(auto i : m_format) {
		switch(i) {
		case 0:
			content += event->get_thread_name();
			continue;
		case 1:
			content += event->get_thread_id();
			continue;
		case 2:
			content += event->get_fiber_id();
			continue;
		case 3:
			content += LogLevel::ToString(event->get_level());
			continue;
		case 4:
			content += event->get_file_name();
			continue;
		case 5:
			content += event->get_line();
			continue;
		case 6: {
			time_t t = event->get_time();
			tm* local;
			char buf[64] = {0};
		
			local = localtime(&t);
			strftime(buf, 64, "%Y-%m-%d %H:%M:%S", local);
			content += buf;
		}
			continue;
		case 7:
			content += "  ";
			continue;
		case 8:
			content += '\n';
			continue;
		case 9:
			content += '%';
			continue;
		case 10:
			content += event->get_content();
			continue;
		default:
			content += "  ";
		};
	}
	for(auto ptr : m_appenders) {
		if(event->get_level() < m_level)
			return;
		if(event->get_level() < ptr->get_level())
			return;
		ptr->output(content);
	}
}







}
