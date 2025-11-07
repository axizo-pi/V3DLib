#ifndef BASIC_LOG_H
#define BASIC_LOG_H
#include <sstream>

namespace Log {

class Logger;

enum LogFlag {
  none,
  hex
};

class LogItem {
private:
	Logger &m_log;
  bool    m_next_is_hex = false;

public:	
	LogItem(Logger &log) : m_log(log) {}
	~LogItem();

	LogItem &operator<<(const std::string &str);
	LogItem &operator<<(const char *str);
	LogItem &operator<<(int n);
	LogItem &operator<<(LogFlag f);
};


class Logger {
public:
	enum Level {
		DDEBUG,  // prefix D to avoid conflict with define
		INFO,
		WARNING,
		ERROR,
		FATAL
	};

	Logger(Level level = INFO) : m_level(level) {}

	LogItem operator<<(const std::string &str) {
		m_buf << str;
		return LogItem(*this);
	};


	std::stringstream &buf() { return m_buf; }
	void flush();

private:
	Level m_level;
	std::stringstream m_buf;
};

void set_level(Logger::Level level);

#pragma push_macro("assert")
#undef assert

void assert(bool condition, const std::string &msg);

#pragma pop_macro("assert")


extern Logger debug;
extern Logger cout;  // same as info
extern Logger info;
extern Logger warn;
extern Logger cerr;  // same as error
extern Logger error;
extern Logger fatal;

} // namespace Log

#endif // BASIC_LOG_H
