///////////////////////////////////////////////////////////////////////////////
//
// NOTE: use following to add two timerval structs:
//
//     void timeradd(struct timeval *a, struct timeval *b, struct timeval *res);
//
// From man timercmp:
//
//    struct timeval {
//        time_t      tv_sec;     /* seconds */
//        suseconds_t tv_usec;    /* microseconds */
//    };
//
// - tv_usec has a value in the range 0 to 999,999.
//
///////////////////////////////////////////////////////////////////////////////
#include "Timer.h"
#include "Log.h"
#include "global.h"
#include <algorithm>  // sort()
#include <cstddef>    // NULL
#include <cstdio>     // printf()
#include <iostream>

using namespace Log;

namespace {

std::vector<std::string> s_headers = {
  "Label", 
  "Total",
  "Steps",
  "Average",
  "Min",
  "Max"
};


/**
 * @brief Return time passed from start, in seconds
 */
float diff_time(timeval const &tvStart, timeval &tvDiff) {
  float ret;

  timeval tvEnd;
  gettimeofday(&tvEnd, NULL);
  timersub(&tvEnd, &tvStart, &tvDiff);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
  ret= 1.0f*tvDiff.tv_sec + (1.0f*tvDiff.tv_usec/1000000l);
#pragma GCC diagnostic pop

  return ret;
}

} // anon namespace

////////////////////////////////////////////////////////////
// Class TimeOut
////////////////////////////////////////////////////////////

TimeOut::TimeOut(int seconds) {
  set(seconds);
}

void TimeOut::set(int seconds) {
  interval = seconds;
  reset();
}


/**
 *
 * ===========================================================================
 * Notes
 * -----
 *
 * 1. time_t, `clock()` **NOT WORKING PROPERLY**. Fires way too fast.
 *    timeval, `diff_time()` works better.
 */
bool TimeOut::timed_out(bool do_reset) {
  timeval tvDiff;
  float diff = ::diff_time(start, tvDiff);

  bool ret = (float) interval < diff;
  if (ret && do_reset) reset();
/*
  if (ret) {
    Log::warn << "Timed out" << ", interval: " << (interval - diff);
  } else {
    Log::warn << "interval: " << (interval - diff);
  }
*/
  return ret;
}


void TimeOut::reset() {
  assert(interval > 0);
  //start = clock();
  gettimeofday(&start, NULL);
}


////////////////////////////////////////////////////////////
// Class MaxWidths
////////////////////////////////////////////////////////////

/**
 * @brief Contains width of specific fields in `dump()`.
 *
 * Used internally only
 */
struct MaxWidths {
  int operator[] (int index) const { return m_widths[index]; }
  int label() const { return m_widths[0]; }
  int total() const { return m_widths[1]; }
  int steps() const { return m_widths[2]; }
  int avg()   const { return m_widths[3]; }
  int min()   const { return m_widths[4]; }
  int max()   const { return m_widths[5]; }

  void update(Timer const &timer);
  void update(Timers const &timers);
  void update(int index, std::string const &str) {
    adjust_width(index, str);
  }

private:
  std::vector<int> m_widths = {
    -1, //label
    -1, //total
    -1, //steps
    -1, //avg
    -1, //min
    -1  //max
  };


  void adjust_width(int index, int val) {
    if (m_widths[index] < val) m_widths[index] = val;
  }


  void adjust_width(int index, std::string const &str) {
    int tmp = (int) str.length();
    adjust_width(index, tmp);
  }
};


void MaxWidths::update(Timer const &timer) {
  adjust_width(0, timer.label());
  adjust_width(1, timer.total_str());

  std::string tmp_count;
  tmp_count << timer.count();
  adjust_width(2, tmp_count);

  adjust_width(3, timer.avg_str());
  adjust_width(4, timer.min_str());
  adjust_width(5, timer.max_str());
}


void MaxWidths::update(Timers const &timers) {
  for (int i = 0; i < (int) timers.list().size(); ++i) {
    auto const &n = timers.list()[i];
    update(n);
  }
}


////////////////////////////////////////////////////////////
// Class Timer
////////////////////////////////////////////////////////////

Timer::Timer(std::string const &label, bool disp_in_dtor) :
  m_disp_in_dtor(disp_in_dtor),
  m_label(label)
{
  gettimeofday(&tvStart, NULL);
  tvMin = tvStart;
}


Timer::~Timer() {
  if (m_disp_in_dtor) {  // Allows RAII usage
    info << "Timer " << m_label << ": " << dump();
  }
}


void Timer::start() {
  assert(!m_started);
  gettimeofday(&tvStart, NULL);  // This ignores timer start in ctor
  m_count++;
  m_started = true;
}


void Timer::stop() {
  assert(m_started);
  assert(m_count > 0);

  // Update total
  timeval tvEnd, tvDiff;

  gettimeofday(&tvEnd, NULL);
  timersub(&tvEnd, &tvStart, &tvDiff);
  timeradd(&tvDiff, &tvTotal, &tvTotal);

  if (timercmp(&tvDiff, &tvMin, <)) {
    tvMin = tvDiff;
  }

  if (timercmp(&tvDiff, &tvMax, >)) {
    tvMax = tvDiff;
  }

  if ((int) m_history.size() < HISTORY_SIZE) {
    m_history.push_back(tvDiff);
  }

  m_started = false;
}


timeval Timer::diff_time() {
  timeval tvDiff;
  m_diff = ::diff_time(tvStart, tvDiff);

  if (m_count == 0) {
    return tvDiff;
  } else {
    return tvTotal;
  }
}


long Timer::time_long(timeval const &val, int count) const {
  auto tmp = (val.tv_sec*1000000l + val.tv_usec)/count;  // type long int
  return tmp;
}


std::string Timer::time_to_str(timeval const &val, int count) const {

#define format "%2ld.%06lds"

  assert(count > 0);
  char buf[128]; 

  auto tmp = time_long(val, count);
  auto avg_sec = tmp/1000000l;
  auto avg_usec = tmp % 1000000l;

  sprintf(buf, format, avg_sec, avg_usec);

  return buf;

#undef format  
}


std::string Timer::total_str() const { return time_to_str(tvTotal); }
std::string Timer::avg_str()   const { return time_to_str(tvTotal, m_count); }
std::string Timer::min_str()   const { return time_to_str(tvMin); }
std::string Timer::max_str()   const { return time_to_str(tvMax); }


std::string Timer::dump_g(MaxWidths const &widths, bool show_extended) {
  using namespace string;
  assert(!m_label.empty());

  std::string buf2;

  if (m_started) stop();

  buf2 << pad(total_str(), widths.total(), false) << " "
       << pad(m_count    , widths.steps())        << " "
       << pad(avg_str()  , widths.avg()  , false) << " ";

  if (show_extended) {
    buf2 << pad(min_str()  , widths.min(), false) << " "
         << pad(max_str()  , widths.max(), false);
  }

  std::string ret;

  ret << pad(m_label, widths.label()) << " ";
  ret << buf2;
  return ret;
}


std::string Timer::dump(bool show_extended) {
  assert(!m_label.empty());

  std::string buf2;

  if (m_count == 0) {
    timeval time = diff_time();
    buf2 << time_to_str(time);
  } else {
    if (m_started) stop();

    buf2 << total_str() 
         << " in "      << m_count << " steps, "
         << "average: " << avg_str();

    if (show_extended) {
      buf2 << " - Min: " << min_str()
           << ", Max: "  << max_str();

      if (HISTORY_SIZE > 0) {
        buf2 << "\n    History: [";

        for (int i = 0; i < (int) m_history.size(); ++i) {
          if (i != 0) buf2 << ",";
          buf2 << time_to_str(m_history[i]);
        }

        buf2 << "]";
      }
    }
  }

  std::string ret;

  ret << m_label << ": "
      << buf2;
  return ret;
}


std::string Timer::dump() {
  return time_to_str(diff_time());
}


/////////////////////////////////////////////////
// Timers
/////////////////////////////////////////////////

namespace {

bool timers_did_end = true;

}  // anon namespace

Timers timers;

Timers::~Timers() {
  if (!timers_did_end) {
    warn << "Global timers set but not outputted.";
  }
}


/**
 * @brief Start a global timer
 *
 * Labels are unique over timers.
 * If the timer does not exist, it is created.
 *
 * **NOTE:** Returned timer does not always work (ie. stop())
 *           as expected. It is safer to use `Timers::stop()`.
 * 
 * @return The started timer
 */
Timer &Timers::start(std::string const &label) {
  if (!m_total.started()) {
    m_total.start();
  }

  int index = find(label);

  if (index == -1) {
    //warn << "Adding timer '" << label << "'";
    m_list << Timer(label);
    index = (int) m_list.size() - 1;
  } else {
    if (m_list[index].started()) {
      cerr << "Profile timer '" << label << "' already started."; // << thrw;
      breakpoint;
    }
  }

  m_list[index].start();
  timers_did_end = false;

  return m_list[index];
}


void Timers::stop(std::string const &label) {
  int index = find(label);
  assert(index != -1);
  m_list[index].stop();
}


std::vector<int> Timers::sort_indexes() {
  assert(!m_list.empty());

  auto to_lower = [] (std::string data) -> std::string {
    std::transform(data.begin(), data.end(), data.begin(),
      [](unsigned char c){ return std::tolower(c); });

    return data;
  };

  auto &list      = m_list;
  auto &sort_data = m_sort_data;

  auto comp = [&list, &sort_data, &to_lower](int lhs_index, int rhs_index) -> bool {
    switch (sort_data.sort_column) {
      case None: assert(false); break;  // Should not be reached here
      case Label: {
        auto lhs = list[lhs_index].label();
        auto rhs = list[rhs_index].label();

        if (sort_data.ignore_case) {
          lhs = to_lower(lhs);
          rhs = to_lower(rhs);
        }

        return (sort_data.desc)? lhs > rhs: lhs < rhs;
      } 
      case Total: {
        auto lhs = list[lhs_index].total();
        auto rhs = list[rhs_index].total();

        return (sort_data.desc)? lhs > rhs: lhs < rhs;
      }
      case Average: {
        auto lhs = list[lhs_index].average();
        auto rhs = list[rhs_index].average();

        return (sort_data.desc)? lhs > rhs: lhs < rhs;
      }
    };

    assert(false);  // Should never be reached
    return false;
  };


  std::vector<int> indexes(m_list.size());
  for (int i = 0; i < (int) indexes.size(); ++i) {
    indexes[i] = i;
  }

  if (m_sort_data.sort_column != None) {
    std::sort(indexes.begin(), indexes.end(), comp);
  }

  return indexes;
}


/**
 * @brief emit headers for global timers.
 *
 * Adjusts the widths if necessary, if the headers are larger than the values.
 */
std::string Timers::emit_headers(MaxWidths &widths, bool show_minmax) {
  std::string buf = "  ";


  int num = (int) s_headers.size();
  if (!show_minmax) num -= 2;

  for (int i = 0; i < num; ++i) {
    auto const &label = s_headers[i];
    widths.update(i, label);
  }

  for (int i = 0; i < num; ++i) {
    auto const &label = s_headers[i];
    bool do_left = (i > 0);
    buf << string::pad(label, widths[i], do_left) << " ";
  }
  buf << "\n  ";

  for (int i = 0; i < num; ++i) {
    buf << string::rep("=", widths[i]) << " ";
  }
  buf << "\n";

  return buf;
}


std::string Timers::dump(bool show_minmax) {
  auto indexes = sort_indexes();
  MaxWidths widths;
  widths.update(*this);
  widths.update(m_total);

  std::string buf;
  buf << "Global Timers:\n"
      << emit_headers(widths, show_minmax)
      << "  " << m_total.dump_g(widths, show_minmax)   // Total is since first call to timers
      << "\n";

  for (int i = 0; i < (int) indexes.size(); ++i) {
    buf << "  " << m_list[indexes[i]].dump_g(widths, show_minmax) << "\n";
  }

  return buf;
}


void Timers::end(bool log_info, bool show_minmax) {
  if (m_total.started()) {
    m_total.stop();
  }

  if (m_list.empty()) {
    info << "Timers end: No global timers, nothing to show";
    return;
  }

  auto buf2 = dump(show_minmax);

  if (log_info) {
    info << buf2;
  } else {
    warn << buf2;
  }

  timers_did_end = true;
}


/**
 * @brief Search timer by label
 *
 * @return index of found timer, -1 if not found
 */
int Timers::find(std::string const &label) {
  int index = -1;

  for (int i = 0; i < (int) m_list.size(); ++i) {
    if (label == m_list[i].label()) {
      index = i;
      break;
    }
  }

  return index;
}


Timers &Timers::sort(SortColumn sort_column, bool desc) {
  m_sort_data.sort_column = sort_column;
  m_sort_data.desc        = desc;

  return *this;
}
