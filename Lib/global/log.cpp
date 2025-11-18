/******************************************************************
 * Version: 4  - Added thrw flag, option to suppres console output
 * Version: 3  - Added hex flag
 ******************************************************************/
#include "log.h"
#include <iostream>
#include <cstdlib>  // abort()
#include <time.h>   // strftime()


namespace Log {

namespace {

Logger::Level log_level = Logger::DDEBUG;

//
// Ensure that no logging information is written to console.
//
// If logging to file is enabled, logfiles should still be written written
//
bool s_log_to_cout = true;

}  // anon namespace


void set_level(Logger::Level level) {
	log_level = level;
}


void log_to_cout(bool val) {
  s_log_to_cout = val;
}


LogItem::~LogItem() {
	m_log.flush(m_throw);
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
  if (m_next_is_hex) {
    char buf[32];
    sprintf(buf, "0x%x", n);
    m_log.buf() << buf;

    m_next_is_hex = false;
  } else { 
	  m_log.buf() << n;
  }
	return *this;
}


LogItem &LogItem::operator<<(LogFlag f) {
  if (f == hex)  m_next_is_hex = true;
  if (f == thrw) m_throw       = true;
	return *this;
}


void Logger::flush(bool do_throw) {
  if (!s_log_to_cout) return;  // TODO: adjustment needed when logging to file enabled


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

		if (m_level == FATAL || do_throw) {
			std::cerr << time_buf << prefix << m_buf.str() << "\n";
			std::flush(std::cout);
			std::flush(std::cerr);

      //
      // CATCH NOT WORKING
      //

      // Throwing is better, since it allows to unroll the stack
			//abort();
      throw std::runtime_error(m_buf.str());
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
Logger cdebug(Logger::DDEBUG);
Logger cout;
Logger info;
Logger warn(Logger::WARNING);
Logger cerr(Logger::ERROR);
Logger error(Logger::ERROR);
Logger fatal(Logger::FATAL);

} // namespace Log
