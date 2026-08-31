#ifndef _EXAMPLE_SUPPORT_TIMER_H
#define _EXAMPLE_SUPPORT_TIMER_H
#include <sys/time.h>
#include <vector>
#include <string>

namespace V3DLib {

class MaxWidths;  // Forward declaration

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
  bool started() const { return m_started; }

  std::string const &label() const { return m_label; }
  long total()   const { return time_long(tvTotal); }
  std::string total_str() const;
  long average() const { return time_long(tvTotal, m_count); }
  std::string avg_str() const;
  int count() const { return m_count; }
  std::string min_str() const;
  std::string max_str() const;

  std::string dump(MaxWidths const &widths, bool show_extended = false);
  std::string end(bool show_output = true);

private:
  const int HISTORY_SIZE = 0;

  bool m_disp_in_dtor = false;
  std::string m_label;

  timeval tvStart;
  timeval tvTotal   = {0,0};
  int     m_count   = 0;
  bool    m_started = false;
  float   m_diff;

  timeval tvMin;
  timeval tvMax = {0,0};

  timeval diff_time();
  long time_long(timeval const &val, int count = 1) const;
  std::string time_to_str(timeval const &val, int count = 1) const;

  std::vector<timeval> m_history;
};


/**
 * @brief Global timers for internal profiling
 *
 * Usage:
 *
 *    timers.start("label");
 *    ...
 *    timers.stop("label");
 *    ...
 *    timers.end();
 *
 * Timers can be started and stopped as often as needed.
 */
class Timers {
public:
  enum SortColumn {
    None,
    Label,
    Total,
    Average
  };

  Timer &start(std::string const &label);
  void stop(std::string const &label);
  void end(bool show_minmax = false);
  Timers &sort(SortColumn sort_column, bool desc = false);
  std::vector<Timer> const &list() const { return m_list; }

private:
  struct SortData {
    SortColumn sort_column = None;
    bool       desc        = false;
    bool       ignore_case = true;
  } m_sort_data;

  std::vector<Timer> m_list;

  int find(std::string const &label);
  std::vector<int> sort_indexes();
};

extern V3DLib::Timers timers;

}  // V3DLib namespace

#endif  // _EXAMPLE_SUPPORT_TIMER_H
