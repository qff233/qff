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
				, time_t time)
	:m_thread_name(thread_name)
	,m_thread_id(thread_id)
	,m_fiber_id(fiber_id)
	,m_level(level)
	,m_file_name(file_name)
	,m_line(line)
	,m_time(time) {
}


LogAppender::LogAppender(const std::string& name, LogLevel::Level level) 
	:m_name(name)
	,m_level(level) {
}

FileLogAppender::FileLogAppender(const std::string& name
								, LogLevel::Level level
								, const std::string& file_name) 
	:LogAppender(name, level) {
	FSUtils::OpenForWrite(m_ofs, file_name, std::ios_base::app);
}

FileLogAppender::~FileLogAppender() {
	m_ofs.close();
}

void FileLogAppender::output(const std::string& str) {
	MutexType::Lock mutex(m_mutex);
	m_ofs << str << std::endl;
}

StandLogAppender::StandLogAppender(const std::string& name
								, LogLevel::Level level)
	:LogAppender(name, level) {
}

void StandLogAppender::output(const std::string& str) {
	MutexType::Lock mutex(m_mutex);
	printf("%s\n", str.c_str());
}

#define XX(Name, class_fun) \
static void Name(std::string& str, LogEvent::ptr p_event, \
			const std::string& time_format) { \
	str += p_event->get_##class_fun(); \
}

XX(F_ThreadName, thread_name);
XX(F_File, file_name);

#undef XX

static void F_Content(std::string& str, LogEvent::ptr p_event, 
			const std::string& time_format) {
	str += p_event->get_SS().str();
}

//%n 换行
//%t 四个空格
//%m 内容
//%p 级别
//%% 百分号
//%d 日期和时间
//%F 源文件名称
//%L 代码所在行数
static void F_Line(std::string& str, LogEvent::ptr p_event
			, const std::string& time_format) {
	str += std::to_string(p_event->get_line());
}

static void F_ThreadId(std::string& str, LogEvent::ptr p_event
			, const std::string& time_format) {
	str += std::to_string(p_event->get_thread_id());
}

static void F_FiberId(std::string& str, LogEvent::ptr p_event
			, const std::string& time_format) {
	str += std::to_string(p_event->get_fiber_id());
}

static void F_Level (std::string& str, LogEvent::ptr p_event
			, const std::string& time_format) {
	str += LogLevel::ToString(p_event->get_level());
}

static void F_Time(std::string& str, LogEvent::ptr p_event
			, const std::string& time_format) {
	time_t t = p_event->get_time();
	tm* local;
	char buf[64] = {0};

	local = localtime(&t);
	strftime(buf, 64, time_format.c_str(), local);

	str += buf;
}

LogFormat::LogFormat(const std::string& format) {
	this->reset(format);
}

void LogFormat::reset(const std::string& format) {
	MutexType::Lock lock(m_mutex);
	m_item.clear();

	for(auto it = format.cbegin()
			; it != format.cend(); ++it) {

		if(it+1 == format.cend()) {
			auto func = [=](std::string& str, LogEvent::ptr p_event
							, const std::string& time_format){
					str += *it;
				};
			m_item.push_back(func);
			break;
		}

		if(*it == '%') {
			auto next_it = it + 1;
			switch(*next_it) {
			case 'n':
				m_item.push_back([](std::string& str, LogEvent::ptr p_event
							, const std::string& time_format){
					str += '\n';
				});
				++it;
				continue;
			case 't': 
				m_item.push_back([](std::string& str, LogEvent::ptr p_event
							, const std::string& time_format){
					str += '\t';
				});
				++it;
				continue;
			case 'm':
				m_item.push_back(F_Content);
				++it;
				continue;
			case 'p':
				m_item.push_back(F_Level);
				++it;
				continue;
			case 'T':
				m_item.push_back(F_ThreadName);
				++it;
				continue;
			case 'i':
				m_item.push_back(F_ThreadId);
				++it;
				continue;
			case 'f':
				m_item.push_back(F_FiberId);
				++it;
				continue;
			case '%':
				m_item.push_back([](std::string& str, LogEvent::ptr p_event
							, const std::string& time_format){
					str += '%';
				});
				++it;
				continue;
			case 'd':
				m_item.push_back(F_Time);
				++it;
				continue;
			case 'F':
				m_item.push_back(F_File);
				++it;
				continue;
			case 'L':
				m_item.push_back(F_Line);
				++it;
				continue;
			default:
				std::string cow_str(1, *it);
				cow_str += *next_it;
				m_item.push_back([=](std::string& str, LogEvent::ptr p_event
							, const std::string& time_format){
					str += cow_str;
				});
				++it;
				continue;
			}
		}
		std::string cow_str;
		for(auto i = it; i != format.end() && *i != '%'; ++i) {
			cow_str += *i;
		}
		auto item_func = [=](std::string& str, LogEvent::ptr p_event
							,const std::string& time_format) {
				str += cow_str;
			};
		m_item.push_back(item_func);
		it += cow_str.size() - 1;
	}
} //LogFormat::reset(const std::string& format)

std::string LogFormat::format(LogEvent::ptr p_event) {
	std::string str;
	for(auto fun : m_item) {
		fun(str, p_event, "%Y-%m-%d %H:%M:%S");
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
	MutexType::Lock lock(m_mutex);
	for(auto it = m_appenders.cbegin()
			; it != m_appenders.cend(); ++it) {
		if(*it == ptr) return;
	}
	m_appenders.push_back(ptr);
}

void Logger::del_appender(LogAppender::ptr ptr) {
	MutexType::Lock lock(m_mutex);
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

LoggerManager::LoggerManager() {
	m_root = std::make_shared<Logger>("ROOT", qff::LogLevel::DEBUG
								, "[%d] [%T] [%i] [%f]  [%p] %F:%L   %n%m");
	m_system = std::make_shared<Logger>("SYSTEM", qff::LogLevel::DEBUG
								, "[%d] [%T] [%i] [%f]  [%p] %F:%L   %n%m");	
	m_loggers.push_back(m_root);
	m_loggers.push_back(m_system);
}


void LoggerManager::log(const std::string& name, LogEvent::ptr p_event) {
	Logger::ptr p_logger = this->get_logger(name);
	p_logger->log(p_event);
}

void LoggerManager::log_system(LogEvent::ptr p_event) {
	m_system->log(p_event);
}

void LoggerManager::log_root(LogEvent::ptr p_event) {
	m_root->log(p_event);
}

void LoggerManager::add_logger(Logger::ptr p_logger) {
	MutexType::Lock lock(m_mutex);
	std::string logger_name = p_logger->get_name();
	for(const Logger::ptr i: m_loggers) {
		if(i->get_name() == logger_name) return;
	}
	m_loggers.push_back(p_logger);
}

void LoggerManager::del_logger(Logger::ptr p_logger) {
	MutexType::Lock lock(m_mutex);
	std::string logger_name = p_logger->get_name();
	for(auto it = m_loggers.begin(); it != m_loggers.end(); ++it) {
		if((*it)->get_name() == logger_name) m_loggers.erase(it);
	}
}

Logger::ptr LoggerManager::get_logger(const std::string& name) {
	for(Logger::ptr i : m_loggers) {
		if(i->get_name() == name) return i;
	}
	return m_system;
}

Logger::ptr LoggerManager::get_root() {
	return this->get_logger("ROOT");
}

Logger::ptr LoggerManager::get_system() {
	return this->get_logger("SYSTEM");
}

LogEventManager::LogEventManager(LogEvent::ptr p_event, Logger::ptr p_logger) 
	:m_event(p_event)
	,m_logger(p_logger) {
}

LogEventManager::~LogEventManager() {
	m_logger->log(m_event);
}

std::stringstream& LogEventManager::get_SS() {
	return m_event->get_SS();
}

}
