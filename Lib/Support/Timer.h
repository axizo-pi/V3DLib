#ifndef _EXAMPLE_SUPPORT_TIMER_H
#define _EXAMPLE_SUPPORT_TIMER_H
#include <sys/time.h>
#include <vector>
#include <string>

namespace V3DLib {

/**
 * Simple wrapper class for outputting run time for examples
 */
class Timer {
public:
  Timer(std::string const &label = "Run time", bool disp_in_dtor = false);
  ~Timer();

  float diff() const { return m_diff; }
  void start();
  void stop();

	std::string const &label() const { return m_label; }
  std::string dump(int width = -1);
  std::string end(bool show_output = true);

private:
  bool m_disp_in_dtor = false;
  std::string m_label;

  timeval tvStart;
  timeval tvTotal = {0,0};
  int count = 0;
  bool started = false;
  float m_diff;

	timeval diff_time();
};


/**
 * @brief Global timers for internal profiling
 */
class Timers {
public:
	Timer &start(std::string const &label);
	void stop(std::string const &label);
	void end();

private:
	std::vector<Timer> m_list;

	int find(std::string const &label);
};

extern V3DLib::Timers timers;

}  // namespace

#endif  // _EXAMPLE_SUPPORT_TIMER_H
