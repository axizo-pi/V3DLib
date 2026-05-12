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
#include "Support/basics.h"
#include "Support/Helpers.h"
#include <cstddef>  // NULL
#include <cstdio>   // printf

using namespace Log;

namespace V3DLib {

Timer::Timer(std::string const &label, bool disp_in_dtor) :
  m_disp_in_dtor(disp_in_dtor),
  m_label(label)
{
  gettimeofday(&tvStart, NULL);

  tvMin = tvStart;
}


Timer::~Timer() {
  if (m_disp_in_dtor) {  // Allows RAII usage
    end();
  }
}


void Timer::start() {
  assert(!started);
  gettimeofday(&tvStart, NULL);  // This ignores timer start in ctor
  count++;
  started = true;
}


void Timer::stop() {
  assert(started);
  assert(count > 0);

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

  started = false;
}


timeval Timer::diff_time() {
  timeval tvEnd, tvDiff;
  gettimeofday(&tvEnd, NULL);
  timersub(&tvEnd, &tvStart, &tvDiff);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
  m_diff= 1.0f*tvDiff.tv_sec + (1.0f*tvDiff.tv_usec/1000000l);
#pragma GCC diagnostic pop

  if (count == 0) {
    return tvDiff;
  } else {
    return tvTotal;
  }
}


std::string Timer::time_to_str(timeval const &val, int count) {

#define format "%2ld.%06lds"

  assert(count > 0);
  char buf[128]; 

  auto tmp = (val.tv_sec*1000000l + val.tv_usec)/count;  // type long int
  auto avg_sec = tmp/1000000l;
  auto avg_usec = tmp % 1000000l;

  sprintf(buf, format, avg_sec, avg_usec);

  return buf;

#undef format  
}


std::string Timer::dump(int width, bool show_extended) {
  assert(!m_label.empty());

  std::string buf2;

  if (count == 0) {
    timeval time = diff_time();
    buf2 << time_to_str(time);
  } else {
    if (started) stop();

    char buf[64]; 
    sprintf(buf, " in %5d steps", count);

    buf2 << time_to_str(tvTotal) << buf << ", average: " << time_to_str(tvTotal, count);

    if (show_extended) {
      buf2 << " - Min: " << time_to_str(tvMin) << ", Max:: " << time_to_str(tvMax);

      if (HISTORY_SIZE > 0) {
        buf2 << "\n" << indentBy(width) << "    History: [";

        for (int i = 0; i < (int) m_history.size(); ++i) {
          if (i != 0) buf2 << ",";

          buf2 << time_to_str(m_history[i]);
        }

        buf2 << "]";
      }
    }
  }

  std::string ret = m_label;

  ret << indentBy(width - (int) m_label.size());
  ret << ": " << buf2;

  return ret;
}


std::string Timer::end(bool show_output) {
  if (show_output) {
    warn << dump();
  }

  return time_to_str(diff_time());
}


/////////////////////////////////////////////////
// Timers
/////////////////////////////////////////////////

Timers timers;


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
  int index = find(label);

  if (index == -1) {
    //warn << "Adding timer '" << label << "'";
    m_list << Timer(label);
    index = (int) m_list.size() - 1;
  }

  m_list[index].start();

  return m_list[index];
}


void Timers::stop(std::string const &label) {
  int index = find(label);
  assert(index != -1);
  m_list[index].stop();
}


void Timers::end(bool show_extended) {
  assert(!m_list.empty());

  // Determine label width
  int width = -1;
  for (int i = 0; i < (int) m_list.size(); ++i) {
    int tmp = (int) m_list[i].label().length();
    if (width < tmp) {
      width = tmp;
    }
  }

  std::string buf;
  for (int i = 0; i < (int) m_list.size(); ++i) {
    buf << "  " << m_list[i].dump(width, show_extended) << "\n";
  }

  warn << "Timers end:\n" << buf;
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
      //warn << "Found timer";
      index = i;
      break;
    }
  }

  return index;
}

}  // namespace
