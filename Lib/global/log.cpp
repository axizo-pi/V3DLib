/***********************************
 * Version: 2
 ***********************************/
#include "log.h"
#include <iostream>
#include <cstdlib>  // abort()
#include <time.h>   // strftime()


namespace log {

Logger::Level log_level = Logger::DDEBUG;


void set_level(Logger::Level level) {
	log_level = level;
}


LogItem::~LogItem() {
	m_log.flush();
}


LogItem &LogItem::operator<<(const std::string &str) {
	m_log.buf() << str;
	return *this;
}


LogItem &LogItem::operator<<(const char *str) {
	m_log.buf() << str;
	return *this;
}


LogItem &LogItem::operator<<(int n) {
	m_log.buf() << n;
	return *this;
}


void Logger::flush() {
	if (m_level >= log_level) {

		char time_buf[256];
		auto now = time(NULL);
		strftime(time_buf, 256, "[%F %T] ", gmtime(&now));  // iDon't need return value

		std::string prefix = "";

		switch (m_level) {
			case DDEBUG:  prefix = "DEBUG: ";   break;
			case INFO:    prefix = "INFO: ";    break;
			case WARNING: prefix = "WARNING: "; break;
			case ERROR:   prefix = "ERROR: ";   break;
			case FATAL:   prefix = "FATAL: ";   break;
		}

		if (m_level == FATAL) {
			std::cerr << time_buf << prefix << m_buf.str() << "\n";
			std::flush(std::cout);
			std::flush(std::cerr);
			abort();
		}

		if (m_level == ERROR) {
			std::cerr << time_buf << prefix << m_buf.str() << "\n";
		} else {
			std::cout << time_buf << prefix << m_buf.str() << "\n";
		}

	}

	m_buf.str("");
}


void assert(bool condition, const std::string &msg) {
	if (condition) return;
	fatal << msg << "\n";
}


Logger debug(Logger::DDEBUG);
Logger cout;                   // Same as info
Logger info;
Logger warn(Logger::WARNING);
Logger cerr(Logger::ERROR);    // same as error
Logger error(Logger::ERROR);
Logger fatal(Logger::FATAL);

} // namespace log
