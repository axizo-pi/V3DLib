#ifndef BASIC_LOG_H
#define BASIC_LOG_H
#include <sstream>

namespace Log {

class Logger;

enum LogFlag {
  none,
  hex,
  thrw  // Can't use 'throw', reserved keyword
};

class LogItem {
private:
  Logger &m_log;
  bool    m_next_is_hex = false;

  // Can be used with any logging form;
  // logging is outputted before throw occurs
  bool    m_throw       = false;

public:  
  LogItem(Logger &log) : m_log(log) {}
  ~LogItem() noexcept(false);

  LogItem &operator<<(const std::string &str);
  LogItem &operator<<(const char *str);
  LogItem &operator<<(int n);
  LogItem &operator<<(unsigned n);
  LogItem &operator<<(unsigned long n);
  LogItem &operator<<(float n);
  LogItem &operator<<(double n);
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

  LogItem operator<<(int rhs) {
    m_buf << rhs;
    return LogItem(*this);
  };


  std::stringstream &buf() { return m_buf; }
  void flush(bool do_throw);

private:
  Level m_level;
  std::stringstream m_buf;

  std::string msg();
};

void set_log_dir(std::string const &path);
void set_log_file(std::string const &file);
void log_to_cout(bool val);

void assertq(bool condition, const std::string &msg = "");

inline void assertq(const std::string &msg) {
  assertq(false, msg);
}


// Duplicates of several instances to avoid error "reference to ‘error’ is ambiguous"
// Eventually, these should be leading, but then we'll have to get rid of debug.h
extern Logger debug;
extern Logger cdebug; // same as debug
extern Logger cout;   // same as info
extern Logger info;
extern Logger warn;
extern Logger cerr;   // same as error
extern Logger error;
extern Logger fatal;


class cout_timestamp {
public:
  cout_timestamp(bool val);
  ~cout_timestamp();

private:
  bool m_prev;
};

} // namespace Log

#ifndef assert
#define assert(cond) Log::assertq(cond)
#endif // assert

#endif // BASIC_LOG_H
