/******************************************************************
 * Versions
 * ========
 * 8 - Added globals 'global', 'fixed' and associated
 *   - Modifier flags moved to anon namespace; they are now global over all log calls
 *   - Pretty extensive refactoring to make this work
 *   - Added unit tests for logging, especially for new modifiers
 * 7 - Added rollover, improved file permissions
 * 6 - restored log_to_cout()
 * 5 - Added logging to file
 * 4 - Added thrw flag, option to suppres console output
 * 3 - Added hex flag
 ******************************************************************/
#include "log.h"
#include "Support/Helpers.h"
#include "Support/basics.h"
#include <iostream>
#include <filesystem>
#include <fstream>

using namespace V3DLib;

namespace Log {

namespace fs = std::filesystem; // Alias for brevity

namespace {

class Modifiers {
public:
  // Can be used with any logging form;
  // logging is outputted before throw occurs
  bool    m_throw       = false;

  bool    m_next_is_global = false;
  bool    m_next_is_hex    = false;  // If false, next int is dec
  bool    m_next_is_fixed  = false;  // If false, next float is scientific

	bool s_hex   = false;
	bool s_fixed = false;

	bool do_throw() { bool ret = m_throw; m_throw = false; return ret; }
	void handle(LogFlag f);
	bool do_hex();
	bool do_fixed();
};

void Modifiers::handle(LogFlag f) {
	if (m_next_is_global) {
		//std::cout << "Handling global: " << f << "\n";

		switch (f) {
			case none:       assert(false);    break; // Not expecting 'none' as input
			case dec:        s_hex         = false;
			                 m_next_is_hex = false;
			break;
			case hex:        s_hex         = true;
			                 m_next_is_hex = true;
			break;
			case fixed:      s_fixed         = true;
			                 m_next_is_fixed = true;
			break;
			case scientific: s_fixed         = false;
			                 m_next_is_fixed = false;
			break;
			case global:     assert(false);    break; // Disallow multiple globals in a row
			default: break; // Rest ignored 
		}

		m_next_is_global = false;
	} else {
		//std::cout << "Handling next\n";

		switch (f) {
			case none:       assert(false);            break; // Not expecting 'none' as input
			case dec:        m_next_is_hex    = false; break;
			case hex:        m_next_is_hex    = true;  break;
	  	case thrw:       m_throw          = true;  break;
			case fixed:      m_next_is_fixed  = true;  break;
			case scientific: m_next_is_fixed  = false; break;
			case global:     m_next_is_global = true;  break;
			default: assert(false); break;
		}
	}
}


bool Modifiers::do_hex() {
	bool ret = s_hex;

  if (m_next_is_hex != s_hex) {
		//std::cout << "do_hex() global != next\n";
		ret = m_next_is_hex;
  	m_next_is_hex = !m_next_is_hex;
	}

	return ret;
}


bool Modifiers::do_fixed() {
	bool ret = s_fixed;

  if (m_next_is_fixed != s_fixed) {
		//std::cout << "do_fixed() global != next\n";
		ret = m_next_is_fixed;
  	m_next_is_fixed = !m_next_is_fixed;
	}

	return ret;
}

Modifiers s_modifiers;

const unsigned long ROLLOVER = 10*1024*1024;

bool s_cout_show_timestamp = true;
bool s_log_to_cout = true;

std::string timestamp() {
  char time_buf[256];
  auto now = time(NULL);
  strftime(time_buf, 256, "[%F %T] ", gmtime(&now));  // Don't need return value

  return time_buf;
}


/**
 * TODO: consolidate with `ensure_path_exists()` in Helpers
 *
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
 * @brief Tryout for rollover
 */
void rollover(std::string const &dir, std::string const &file) {
  std::string outfile;
  outfile << dir << "/" << file;
  if (!fs::exists(outfile)) return;

  fs::path p{outfile};
  auto size = fs::file_size(p);

  std::string base = split(file, ".")[0];
  //std::cout << "File base: " << base << "\n";
  //std::cout << "size log '" << outfile << ": " << size << "\n";

  if (size <= ROLLOVER) return;

  std::cout << "Performing rollover.\n";

  int index = -1;
  for (const auto & entry : fs::directory_iterator(dir)) {
    std::string file = split(entry.path(), "/").back();

    if (contains(file, base)) {
      //std::cout << "  " << entry.path() << std::endl;

      // Isolate the index
      auto tmp = split(file, ".");
      assert(tmp.size() == 2);
      tmp = split(tmp[0], "-");
      if (tmp.size() > 1) {
        int tmp_index = atoi(tmp[1].c_str());
        if (tmp_index > index) {
          index = tmp_index;
          //std::cout << "New highest index: " << index << "\n";
        }
      }
    }
  }

  index++;
  //std::cout << "Next index: " << index << "\n";

  // Create new filename
  auto path = split(outfile, ".");
  std::string new_filename;
  new_filename << path[0] << "-" << index << ".log";
  std::cout << "New filename: " << new_filename << "\n";
  fs::rename(outfile, new_filename);
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

  void log_file(std::string const &file) { m_file = file;    }
  std::string log_file()                 { return m_file;    }

  static void log_dir(std::string path)  { m_log_dir = path; }
  static std::string log_dir()           { return m_log_dir; }

protected:
  void _out(Logger::Level level, std::string const &msg) override {
    using namespace std;

    if (m_log_dir.empty()) return;
    if (m_file.empty())    return;

    std::string outfile;
    outfile << m_log_dir << "/" << m_file;

    rollover(m_log_dir, m_file);

    ensure_path_exists(m_log_dir);

    bool new_file = !fs::exists(outfile);
    ofstream of;
    of.open(outfile, ios::app);
    assertq(!of.fail(), "Can not open logfile for appending");

    of << msg;
    of.close();

    if (new_file) {
      // Change the file permissions to read/write
      fs::permissions(
        outfile,
        fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all,
        fs::perm_options::replace
      );
    }
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
  m_log.flush(s_modifiers.do_throw());
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
  if (s_modifiers.do_hex()) {
    char buf[32];
    sprintf(buf, "0x%x", n);
    m_log.buf() << buf;
  } else { 
    m_log.buf() << n;
  }
  return *this;
}


LogItem &LogItem::operator<<(unsigned n) {
  if (s_modifiers.do_hex()) {
    char buf[32];
    sprintf(buf, "0x%x", n);
    m_log.buf() << buf;
  } else { 
    m_log.buf() << n;
  }
  return *this;
}


LogItem &LogItem::operator<<(unsigned long n) {
  if (s_modifiers.do_hex()) {
    char buf[64];
    sprintf(buf, "0x%lx", n);
    m_log.buf() << buf;
  } else {
    m_log.buf() << n;
  }

  return *this;
}



LogItem &LogItem::operator<<(float n) {
  if (s_modifiers.do_fixed()) {
  	m_log.buf().setf(std::ios_base::fixed, std::ios_base::floatfield);
	} else {
  	m_log.buf().setf(std::ios_base::scientific, std::ios_base::floatfield);
	}
  m_log.buf()  << n;
  return *this;
}


LogItem &LogItem::operator<<(double n) {
	// NOTE: No modifiers handled here
  m_log.buf() << n;
  return *this;
}


LogItem &LogItem::operator<<(LogFlag f) {
	s_modifiers.handle(f);
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

  assertq(!m_buf.str().empty(), "m_buf is empty!");
  std::string msg;
  msg << prefix << m_buf.str() << "\n";

  return msg;
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

std::string log_dir() {
  return FileLogger::log_dir();
}


void set_log_file(std::string const &file) {
  file_logger.log_file(file);
}

std::string log_file() {
  return file_logger.log_file();
}


void log_to_cout(bool val) {
  s_log_to_cout = val;
}


void assertq(bool condition, const std::string &msg) {
  if (condition) return;

  if (msg.empty()) {
    fatal << "assertq failed.\n";
  } else {
    fatal << msg << "\n";
  }
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
