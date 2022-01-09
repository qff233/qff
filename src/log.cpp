#include "log.h"

#include <algorithm>
#include <iostream>

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

#define XX(ItemName, Func)										\
	class ItemName##Item : public LogFormat::Item {				\
	public:														\
		void append(std::string& str, LogEvent::ptr p_event,	\
			const std::string& time_format) override {			\
			str += p_event->get_##Func();						\
		}														\
	}

XX(ThreadName, thread_name);
XX(File, file_name);
XX(Line, line);
XX(Content, content);

#undef XX

//%n 换行
//%t 四个空格
//%m 内容
//%p 级别
//%% 百分号
//%d 日期和时间
//%F 源文件名称
//%L 代码所在行数

class ThreadIdItem : public LogFormat::Item {
public:
	void append(std::string& str, LogEvent::ptr p_event
			, const std::string& time_format) override {
		str += std::to_string(p_event->get_thread_id());
	}
};

class FiberIdItem : public LogFormat::Item {
public:
	void append(std::string& str, LogEvent::ptr p_event
			, const std::string& time_format) override {
		str += std::to_string(p_event->get_fiber_id());
	}
};

class StrItem : public LogFormat::Item {
public:
	StrItem(const std::string& str) 
		:m_str(str) {
	}
	void append(std::string& str, LogEvent::ptr p_event
			, const std::string& time_format) override {
		str += m_str;
	}
private:
	std::string m_str;
};

class CharItem : public LogFormat::Item {
public:
	CharItem(char c)
		:m_c(c) {
	}
	void append(std::string& str, LogEvent::ptr p_event
			, const std::string& time_format) override {
		str += m_c;
	}
private:
	char m_c;
};

class LevelItem : public LogFormat::Item {
public:
	void append(std::string& str, LogEvent::ptr p_event
			, const std::string& time_format) override {
		str += LogLevel::ToString(p_event->get_level());
	}
};

class TimeItem : public LogFormat::Item {
public:
	void append(std::string& str, LogEvent::ptr p_event
			, const std::string& time_format) override {
		time_t t = p_event->get_time();
		tm* local;
		char buf[64] = {0};

		local = localtime(&t);
		strftime(buf, 64, time_format.c_str(), local);

		str += buf;
	}
};

LogFormat::LogFormat(const std::string& format) {
	this->reset(format);
}

void LogFormat::reset(const std::string& format) {
	m_item.clear();

	for(auto it = format.cbegin()
			; it != format.cend(); ++it) {

		if(it+1 == format.cend()) {
			Item::ptr item = std::make_shared<CharItem>(*it);
			m_item.push_back(item);
			break;
		}
		
#define XX(Name, Args)										\
		{ Item::ptr item = std::make_shared<Name##Item>(Args);	\
		m_item.push_back(item);								\
		it = next_it;										\
		continue;}

		if(*it == '%') {
			auto next_it = it + 1;
			switch(*next_it) {
			case 'n':
				XX(Char, '\n')
			case 't':
				XX(Char, '\t')
			case 'm':
				XX(Content, )
			case 'p':
				XX(Level, )
			case 'T':
				XX(ThreadName, )
			case 'i':
				XX(ThreadId, )
			case 'f':
				XX(FiberId, )
			case '%':
				XX(Char, '%')
			case 'd':
				XX(Time, )
			case 'F':
				XX(File, )
			case 'L':
				XX(Line, )
			default:
				std::string str(1, *it);
				str += *next_it;
				XX(Str, std::move(str));
			}
		}
#undef XX
		std::string str;
		for(auto i = it; i != format.end() && *i != '%'; ++i) {
			str += *i;
		}
		Item::ptr item = std::make_shared<StrItem>(std::move(str));
		m_item.push_back(item);
		it += str.size() - 1;
	}
}

std::string LogFormat::format(LogEvent::ptr p_event) {
	std::string str;
	for(auto i : m_item) {
		i->append(str, p_event, "%Y-%m-%d %H:%M:%S");
	}
	return str;
}

//%n 换行
//%t 四个空格
//%m 内容
//%p 级别
//%T 线程名
//%i 线程ID
//%f 协程ID
//%% 百分号
//%d 日期和时间
//%F 源文件名称
//%L 代码所在行数
Logger::Logger(const std::string& name, LogLevel::Level level
				, const std::string& format) 
	:m_name(name)
	,m_level(level) {
	m_format = std::make_shared<LogFormat>(format);
	auto p_appender = std::make_shared<StandLogAppender>("stand", m_level);
	this->add_appender(p_appender);
}

void Logger::set_format(const std::string& format) {
	m_format->reset(format);
}

void Logger::add_appender(LogAppender::ptr ptr) {
	for(auto it = m_appenders.cbegin()
			; it != m_appenders.cend(); ++it) {
		if(*it == ptr) return;
	}
	m_appenders.push_back(ptr);
}

void Logger::del_appender(LogAppender::ptr ptr) {
	for(auto it = m_appenders.begin()
			; it != m_appenders.end(); ++it) {
		if(*it == ptr) {
			m_appenders.erase(it);
			return;
		}
	}
}

void Logger::log(LogEvent::ptr p_event) {
	if(p_event->get_level() < m_level)
		return;
	for(auto ptr : m_appenders) {
		if(p_event->get_level() < ptr->get_level())
			return;
		std::string content = m_format->format(p_event);
		ptr->output(content);
	}
}



}
