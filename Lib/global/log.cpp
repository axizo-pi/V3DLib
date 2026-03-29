/******************************************************************
 * Version: 6 - restored log_to_cout()
 * Version: 5 - Added logging to file
 * Version: 4 - Added thrw flag, option to suppres console output
 * Version: 3 - Added hex flag
 ******************************************************************/
#include "log.h"
#include <stdexcept>
#include <iostream>
#include <cstdlib>            // abort()
#include <time.h>             // strftime()
#include <sstream>            // std::stringstream
#include <fstream>
#include <filesystem>

namespace Log {
namespace {

namespace fs = std::filesystem; // Alias for brevity

bool s_cout_show_timestamp = true;
bool s_log_to_cout = true;

std::string timestamp() {
  char time_buf[256];
  auto now = time(NULL);
  strftime(time_buf, 256, "[%F %T] ", gmtime(&now));  // Don't need return value

  return time_buf;
}


/**
 * Source: https://www.tutorialpedia.org/blog/create-a-directory-if-it-doesn-t-exist/#google_vignette
 */
bool create_directory_if_missing(const fs::path& dir_path) {
  try {
    // Check if the directory exists
    if (fs::exists(dir_path) && fs::is_directory(dir_path)) {
      //std::cout << "Directory already exists: " << dir_path << '\n';
      return false; // No action needed
    }

    // Create directory (and parents)
    if (fs::create_directories(dir_path)) {
      //std::cout << "Created directory: " << dir_path << '\n';
      return true;
    } else {
      std::cerr << "Failed to create directory: " << dir_path << '\n';
      return false;
    }
  } catch (const fs::filesystem_error& e) {
    std::cerr << "Filesystem error: " << e.what() << '\n';
    return false;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << '\n';
    return false;
  }
}

/**
 * Internal Logger
 */
class IntLogger {
public:
  IntLogger(Logger::Level level) : m_level(level) {}

  virtual bool will_output(Logger::Level level) const {
    return  (m_level <= level);
  }

  void out(Logger::Level level, std::string const &msg) {
    if (m_level > level) return; 
    _out(level, msg);
  }

protected:
  Logger::Level m_level = Logger::DDEBUG;

  virtual void _out(Logger::Level level, std::string const &msg) = 0;
};


class CoutLogger: public IntLogger {
public:
  CoutLogger(Logger::Level level = Logger::Level::DDEBUG) : IntLogger(level) {}

protected:

  void _out(Logger::Level level, std::string const &msg) override {
    if (!s_log_to_cout) return;

    if (level > Logger::ERROR) {
      std::cerr << msg;
    } else {
      std::cout << msg;
    }

    if (level == Logger::FATAL) {
      std::flush(std::cout);
      std::flush(std::cerr);
    }
  }
};


class FileLogger: public IntLogger {
public:
  FileLogger(Logger::Level level = Logger::Level::DDEBUG) : IntLogger(level) {}

  bool will_output(Logger::Level level) const override {
    if (m_log_dir.empty()) return false;
    if (m_file.empty())    return false;

    return  (m_level <= level);
  }

  void log_file(std::string const &file) {
    m_file = file;
  }

  static void log_dir(std::string path) {
    m_log_dir = path;
  }

protected:
  void _out(Logger::Level level, std::string const &msg) override {
    using namespace std;

    if (m_log_dir.empty()) return;
    if (m_file.empty())    return;

    stringstream path;
    path << m_log_dir << "/" << m_file;

    ofstream of;
    of.open(path.str(), ios::app);
    assert(!of.fail(), "Can not open logfile for appending");

    of << msg;
    of.close();
  }

private:
         std::string m_file;
  static std::string m_log_dir;
};

std::string FileLogger::m_log_dir;


CoutLogger cout_logger(Logger::Level::WARNING);
FileLogger file_logger;

}  // anon namespace


LogItem::~LogItem() noexcept(false) {
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


LogItem &LogItem::operator<<(unsigned n) {
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


LogItem &LogItem::operator<<(unsigned long n) {
  if (m_next_is_hex) {
    char buf[64];
    sprintf(buf, "0x%lx", n);
    m_log.buf() << buf;

    m_next_is_hex = false;
  } else {
    m_log.buf() << n;
  }

  return *this;
}



LogItem &LogItem::operator<<(float n) {
  m_log.buf().setf(std::ios_base::scientific, std::ios_base::floatfield);
  m_log.buf()  << n;
  return *this;
}


LogItem &LogItem::operator<<(double n) {
  m_log.buf() << n;
  return *this;
}


LogItem &LogItem::operator<<(LogFlag f) {
  if (f == hex)  m_next_is_hex = true;
  if (f == thrw) m_throw       = true;
  return *this;
}


std::string Logger::msg() {
  std::string prefix = "";

  switch (m_level) {
    case DDEBUG:  prefix = "DEBUG: ";   break;
    case INFO:    prefix = "INFO: ";    break;
    case WARNING: prefix = "WARNING: "; break;
    case ERROR:   prefix = "ERROR: ";   break;
    case FATAL:   prefix = "FATAL: ";   break;
  }

  assert(!m_buf.str().empty(), "m_buf is empty!");
  std::stringstream msg;
  msg << prefix << m_buf.str() << "\n";

  return msg.str();
}


void Logger::flush(bool do_throw) {
  if (m_level == FATAL) {
    do_throw = true;
  }

  if (cout_logger.will_output(m_level)) {
    std::string buf;
    if (s_cout_show_timestamp) {
      buf = timestamp();
    }
    cout_logger.out(m_level, buf + msg());
  }

  if (file_logger.will_output(m_level)) {
    file_logger.out(m_level, timestamp() + msg());
  }
  

  if (do_throw) {
     // Throwing is better, since it allows to unroll the stack
     throw std::runtime_error(m_buf.str());
  }

  m_buf.str("");
}


void set_log_dir(std::string const &path) {
  create_directory_if_missing(path);
  FileLogger::log_dir(path);
}


void set_log_file(std::string const &file) {
  file_logger.log_file(file);
}


void log_to_cout(bool val) {
  s_log_to_cout = val;
}


void assertq(bool condition, const std::string &msg) {
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


cout_timestamp::cout_timestamp(bool val) {
  m_prev = s_cout_show_timestamp;
  s_cout_show_timestamp = val;
}


cout_timestamp::~cout_timestamp() {
  s_cout_show_timestamp = m_prev;
}
} // namespace Log
