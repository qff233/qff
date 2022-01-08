#ifndef __QFF_LOG_H__
#define __QFF_LOG_H__

#include <string>
#include <memory>
#include <vector>

namespace qff {

class LogLevel {
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

class LogEvent {
public:
	using ptr = std::shared_ptr<LogEvent>;
	LogEvent(const std::string& thread_name, int thread_id, int fiber_id
			, LogLevel::Level level, const std::string& file_name
			, int line, time_t time, const std::string& content);

	const std::string& get_thread_name() { return m_thread_name; }
	int get_thread_id() { return m_thread_id; }
	int get_fiber_id() { return m_fiber_id; }
	LogLevel::Level get_level() { return m_level; }
	const std::string& get_file_name() { return m_file_name; }
	int get_line() { return m_line; }
	time_t get_time() { return m_time; }
	const std::string& get_content() { return m_content; }
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
	using ptr = std::shared_ptr<LogAppender>;

	LogAppender(const std::string& name, LogLevel::Level level);
	virtual ~LogAppender() { };

	virtual void output(const std::string& str) = 0;
	
	LogLevel::Level get_level() { return m_level; }
private:
	std::string m_name;
	LogLevel::Level m_level;
};

class FileLogAppender : public LogAppender {
public:
	FileLogAppender(const std::string& name, LogLevel::Level level);

	void output(const std::string& str) override;
private:
};

class StandLogAppender : public LogAppender {
public:
	StandLogAppender(const std::string& name, LogLevel::Level level);

	void output(const std::string& str) override;
};

class Logger {
public:
	Logger(const std::string& name, LogLevel::Level level, const std::string& format);

	void set_format(const std::string& format);
	void log(LogEvent::ptr event);
private:
	std::string m_name;
	LogLevel::Level m_level;
	std::vector<uint8_t> m_format;
	std::vector<LogAppender::ptr> m_appenders;
};


}

#endif
