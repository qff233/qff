#ifndef __QFF_LOG_H__
#define __QFF_LOG_H__

#include <string>
#include <memory>
#include <vector>
#include <functional>

#include "utils.h"
#include "macro.h"
#include "singleton.h"


#define QFF_LOG_NAME(NAME, STR) \
	Loggermgr::Get()->log(NAME,  \
			std::make_shared<qff::LogEvent>("UNKNOW", ::qff::GetThreadId(), ::qff::GetFiberId(), qff::LogLevel::DEBUG \
											, __FILE__, __LINE__, time(0), STR));

#define QFF_LOG_ROOT(STR) \
	qff::LoggerMgr::Get()->	\
		log_root(std::make_shared<qff::LogEvent>("UNKNOW", ::qff::GetThreadId(), ::qff::GetFiberId(), qff::LogLevel::DEBUG \
											, __FILE__, __LINE__, time(0), STR));

#define QFF_LOG_SYSTEM(STR) \
	qff::LoggerMgr::Get()-> \
	log_system(std::make_shared<qff::LogEvent>("UNKNOW", ::qff::GetThreadId(), ::qff::GetFiberId(), qff::LogLevel::DEBUG \
											, __FILE__, __LINE__, time(0), STR));

namespace qff {

class LogLevel final {
public:
	enum Level {
		DEBUG = 0,
		INFO  = 1,
		WARN  = 2,
		ERROR = 3,
		FATAL = 4
	};
	static std::string ToString(Level level);
	static LogLevel::Level FromString(const std::string& str);
};

class LogEvent final {
public:
	using ptr = std::shared_ptr<LogEvent>;
	LogEvent(const std::string& thread_name, int thread_id, int fiber_id
			, LogLevel::Level level, const std::string& file_name
			, int line, time_t time, const std::string& content);

	const std::string& get_thread_name() const { return m_thread_name; }
	int get_thread_id() const { return m_thread_id; }
	int get_fiber_id() const { return m_fiber_id; }
	LogLevel::Level get_level() const { return m_level; }
	const std::string& get_file_name() const { return m_file_name; }
	int get_line() const { return m_line; }
	time_t get_time() const { return m_time; }
	const std::string& get_content() const { return m_content; }
private:
	std::string m_thread_name;
	int m_thread_id = -1;
	int m_fiber_id = -1;
	LogLevel::Level m_level;
	std::string m_file_name;
	int m_line = -1;
	time_t m_time = -1;
	std::string m_content;
};

class LogAppender {
public:
	NONECOPYABLE(LogAppender);
	using ptr = std::shared_ptr<LogAppender>;

	LogAppender(const std::string& name, LogLevel::Level level);
	virtual ~LogAppender() { };

	virtual void output(const std::string& str) = 0;
	
	const std::string& get_name() const { return m_name; }
	LogLevel::Level get_level() { return m_level; }
private:
	std::string m_name;
	LogLevel::Level m_level;
};

class FileLogAppender final : public LogAppender {
public:
	FileLogAppender(const std::string& name, LogLevel::Level level);

	void output(const std::string& str) override;
private:
};

class StandLogAppender final : public LogAppender {
public:
	StandLogAppender(const std::string& name, LogLevel::Level level);

	void output(const std::string& str) override;
};

class LogFormat final {
public:
	NONECOPYABLE(LogFormat);
	using ptr = std::shared_ptr<LogFormat>;
	using ItemFunc = std::function<void(std::string&
									,LogEvent::ptr, const std::string&)>;

	LogFormat(const std::string& format = "");

	void reset(const std::string& format);

    std::string format(LogEvent::ptr p_event);
private:
	std::vector<ItemFunc> m_item;
};

class Logger final {
public:
	NONECOPYABLE(Logger);
	using ptr = std::shared_ptr<Logger>;

	Logger(const std::string& name, LogLevel::Level level, const std::string& format);
	
	const std::string& get_name() const { return m_name; }

	void set_format(const std::string& format);
	void add_appender(LogAppender::ptr ptr);
	void del_appender(LogAppender::ptr ptr);

	void log(LogEvent::ptr event);
private:
	std::string m_name;
	LogLevel::Level m_level;
	LogFormat::ptr m_format;
	std::vector<LogAppender::ptr> m_appenders;
};

class LoggerManager final {
public:
	LoggerManager();

	void log(const std::string& name, LogEvent::ptr p_event);
	void log_system(LogEvent::ptr p_event);
	void log_root(LogEvent::ptr p_event);

	void add_logger(Logger::ptr logger);
	void del_logger(Logger::ptr logger);

	Logger::ptr get_logger(const std::string& name);
	Logger::ptr get_root();
private:
	Logger::ptr m_system;
	Logger::ptr m_root;
	std::vector<Logger::ptr> m_loggers;
};

using LoggerMgr = Singleton<LoggerManager>;

}

#endif
