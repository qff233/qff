#ifndef __QFF_LOG_H__
#define __QFF_LOG_H__

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <sstream>
#include <fstream>

#include "utils.h"
#include "macro.h"
#include "singleton.h"
#include "thread.h"


#define QFF_LOG_NAME(NAME)															\
	qff::Loggermgr::Get()->															\
		get_logger(NAME)

#define QFF_LOG_ROOT																\
	qff::LoggerMgr::Get()->get_root()

#define QFF_LOG_SYSTEM 																\
	qff::LoggerMgr::Get()->get_system()

#define QFF_LOG_EVENT(LEVEL)	\
	std::make_shared<qff::LogEvent>(qff::GetThreadName(), 			\
			 ::qff::GetThreadId(), ::qff::GetFiberId(), qff::LogLevel::LEVEL 		\
					,__FILE__, __LINE__, time(0))

#define QFF_LOG_DEBUG(LOGGER)														\
	qff::LogEventManager(QFF_LOG_EVENT(DEBUG), LOGGER).get_SS()

#define QFF_LOG_INFO(LOGGER)														\
	qff::LogEventManager(QFF_LOG_EVENT(INFO), LOGGER).get_SS()

#define QFF_LOG_WARN(LOGGER)														\
	qff::LogEventManager(QFF_LOG_EVENT(WARN), LOGGER).get_SS()

#define QFF_LOG_ERROR(LOGGER)														\
	qff::LogEventManager(QFF_LOG_EVENT(ERROR), LOGGER).get_SS()

#define QFF_LOG_FATAL(LOGGER)														\
	qff::LogEventManager(QFF_LOG_EVENT(FATAL), LOGGER).get_SS()

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
	typedef std::shared_ptr<LogEvent> ptr;
	LogEvent(const std::string& thread_name, int thread_id, int fiber_id
			, LogLevel::Level level, const std::string& file_name
			, int line, time_t time);

	const std::string& get_thread_name() const { return m_thread_name; }
	int get_thread_id() const { return m_thread_id; }
	int get_fiber_id() const { return m_fiber_id; }
	LogLevel::Level get_level() const { return m_level; }
	const std::string& get_file_name() const { return m_file_name; }
	int get_line() const { return m_line; }
	time_t get_time() const { return m_time; }
	std::stringstream& get_SS() { return m_content; }

private:
	std::string m_thread_name;
	int m_thread_id = -1;
	int m_fiber_id = -1;
	LogLevel::Level m_level;
	std::string m_file_name;
	int m_line = -1;
	time_t m_time = -1;
	std::stringstream m_content;
};


class LogAppender {
public:
	NONECOPYABLE(LogAppender);
	typedef std::shared_ptr<LogAppender> ptr;
	typedef Mutex MutexType;

	LogAppender(const std::string& name, LogLevel::Level level);
	virtual ~LogAppender() { };

	virtual void output(const std::string& str) = 0;
	
	const std::string& get_name() const { return m_name; }
	LogLevel::Level get_level() { return m_level; }
protected:
	MutexType m_mutex;
private:
	std::string m_name;
	LogLevel::Level m_level;
};

class FileLogAppender final : public LogAppender {
public:
	FileLogAppender(const std::string& name, LogLevel::Level level
					, const std::string& file_name);
	~FileLogAppender();

	void output(const std::string& str) override;
private:
	std::ofstream m_ofs;
};

class StandLogAppender final : public LogAppender {
public:
	StandLogAppender(const std::string& name, LogLevel::Level level);

	void output(const std::string& str) override;
};

class LogFormat final {
public:
	NONECOPYABLE(LogFormat);
	typedef std::shared_ptr<LogFormat> ptr;
	typedef Mutex MutexType;
	typedef std::function<void(std::string&, LogEvent::ptr
									, const std::string&)> ItemFunc;

	LogFormat(const std::string& format = "");

	void reset(const std::string& format);

    std::string format(LogEvent::ptr p_event);
private:
	std::vector<ItemFunc> m_item;
	MutexType m_mutex;
};

class Logger final {
public:
	NONECOPYABLE(Logger);
	typedef std::shared_ptr<Logger> ptr;
	typedef Mutex MutexType;

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
	MutexType m_mutex;
};

class LogEventManager final {
public:
	NONECOPYABLE(LogEventManager);

	LogEventManager(LogEvent::ptr p_event, Logger::ptr p_logger);
	~LogEventManager();

	std::stringstream& get_SS();
private:
	LogEvent::ptr m_event;
	Logger::ptr m_logger;
};

class LoggerManager final {
public:
	NONECOPYABLE(LoggerManager);

	typedef Mutex MutexType;

	LoggerManager();

	void log(const std::string& name, LogEvent::ptr p_event);
	void log_system(LogEvent::ptr p_event);
	void log_root(LogEvent::ptr p_event);

	void add_logger(Logger::ptr p_logger);
	void del_logger(Logger::ptr p_logger);

	Logger::ptr get_logger(const std::string& name);
	Logger::ptr get_root();
	Logger::ptr get_system();
private:
	Logger::ptr m_system;
	Logger::ptr m_root;
	std::vector<Logger::ptr> m_loggers;
	MutexType m_mutex;
};


using LoggerMgr = Singleton<LoggerManager>;

}

#endif
