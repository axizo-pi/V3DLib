#include "dump.h"
#include "Helpers.h"          // bit_diff();
#include <cmath>              // floor()
#include <map>

using namespace V3DLib;

////////////////////////////////////////////////////////
// Class MatrixAdapter
////////////////////////////////////////////////////////

std::string MatrixAdapter::dump_dim() const {
  std::string ret;
  ret << "(" << height() << ", " << width() << ")";
  return ret;
}


/**
 * @brief Check if all values of the matrix are 0.
 *
 * This is inefficient for qpu::matrix; using a pointer with increment is better.  
 * Using ptr 11x faster than using arr[i], still 7x slower than Xf.
 *
 * Here, generic code is selected over efficiency.
 */
bool MatrixAdapter::is_zero() const {
  bool ret = true;

  for (int i = 0; i < rows(); i++) {
    for (int j = 0; j < columns(); j++) {
      if (at(i, j) != 0.0f) {
        ret = false;
        break;
      }
    }
  }

  return ret;
}


////////////////////////////////////////////////////////
// Class FloatArrayAdapter
////////////////////////////////////////////////////////

FloatArrayAdapter::FloatArrayAdapter(V3DLib::Float::Array const &m, int rows, int cols) :
  m_m(m),
  m_rows(rows),
  m_cols(cols)
{}


float FloatArrayAdapter::at(int r, int c) const {
  return m_m[r*m_cols + c];
}


////////////////////////////////////////
// Class CompareStats
////////////////////////////////////////

void CompareStats::reset() {
  first_i  = -1;
  first_j  = -1;
  total    = 0;
  exact    = 0;
  same     = 0;
  zeroes   = 0;
  max_diff = 0.0f;
  max_bit  = -1;
}


bool CompareStats::failed() const {
  assert((first_i == -1 && first_j == -1) || (first_i != -1 && first_j != -1));
  return (first_i != -1);
}


std::string CompareStats::dump(bool show_first_fail) const {
  std::string ret;

  auto as_percent = [] (int denom, int div) -> std::string {
    std::string ret;

    float val = (float) denom/ (float) div*100.0f;

    ret << (int) floor(val) << "." << (int) floor((val - (long) val)*100.0f) << "%";
    return ret;
  };

  auto width = [] (int val) -> int {
    std::string buf;
    buf << val;
    return (int) buf.size();
  };

  auto format = [&width] (int in_width, int val) -> std::string {
    int spaces = in_width - width(val);

    std::string ret;

    for (int i = 0; i < spaces; ++i) {
      ret << " ";
    }

    ret << val;

    return ret;
  };

  ret << "Compare Stats:";

  if (fail_on_first() && failed()) {
     ret << " failed on first, stats incomplete";
    return ret;
  }

  if (exact == total) {
     ret << " exact";
    return ret;
  }

  if ((exact + same) == total) {
     ret << " same";
    return ret;
  }

  ret << "\n";

  if (show_first_fail) {
    ret << "  first fail: (" << first_i << ", " << first_j  << ")\n";
  }

  int x_width = width(exact);

  ret << "  total     : " << total << "\n"
      << "  exact     : " << exact                  << ", " << as_percent(exact, total)        << "\n"
      << "  same      : " << format(x_width, same)  << ", " << as_percent(exact + same, total) << "\n"
      << "  zeroes    : " << zeroes   << "\n"
      << "  max_diff  : " << max_diff << "\n"
      << "  max_bit   : " << max_bit;

  return ret;
}


////////////////////////////////////////////////////////
// Global Functions 
////////////////////////////////////////////////////////

std::string vector_dump(MatrixAdapter const &src, int start_index, bool output_int, bool transpose) {
  std::vector<std::string> ret;
  const int min_count = 5;

  auto out_val = [output_int] (float val) -> std::string {
    std::string buf;

    if (output_int) {
      buf << (int) val;
    } else {
      if (val == (int) val) {
        buf << (int) val;
      } else {
        buf << val;
      }
    }

    return buf;
  };


  auto out_buf = [min_count, &out_val] (float same_val, int same_count) -> std::string {
    std::string buf;

    if (same_count >= min_count) {
      buf << out_val(same_val) << " x " << same_count;
    } else {
      for (int i = 0; i < same_count; ++i) {
        if (i > 0) {
          buf << ", ";
        }

        buf << out_val(same_val);
      }
    }
    return buf;
  };

  auto vectorToString = [] (const std::vector<std::string>& vec, const std::string& delimiter) -> std::string {
    std::string result;
    for (const auto& str : vec) {
        if (!result.empty()) result += delimiter;
        result += str;
    }
    return result;
  };


  int   same_count = 0;
  float same_val   = src.at(start_index, 0);

  auto handle_loop = [&same_count, &same_val, &ret, &out_buf] (float val) {
    if (val == same_val) {
      same_count++;
    } else {
      ret.push_back(out_buf(same_val, same_count));

      same_count = 1;
      same_val   = val;
    }
  };


  if (transpose) {
    assert(start_index == 0);

    for (int h = 0; h < src.height(); ++h) {
      float val = src.at(h, 0);
      handle_loop(val);
    }
  } else {
    for (int w = 0; w < src.width(); ++w) {
      float val = src.at(start_index, w);
      handle_loop(val);
    }
  }

  ret.push_back(out_buf(same_val, same_count));
  return vectorToString(ret, ", ");
}


std::string matrix_dump(MatrixAdapter const &src, bool output_int) {
  std::string ret;

  ret << src.dump_dim() << " ";


  if (src.empty()) {
    ret << "[]";
    return ret;
  }

  if (src.rows() > 1 && src.columns() == 1) {
    ret << "(tr) ";  // Signal transposed
    ret << "[" << vector_dump(src, 0, output_int, true) << "]";
  } else {
    int int_width;
    int prefix_width;

    {
      std::string buf;
      buf << src.rows();
      int_width = (int) buf.size();

      // width of 'xxx-xxx'
      prefix_width = 2*int_width + 1;
    }

    auto pad = [] (int val, int width) -> std::string {
      std::string buf;
      buf << val;

      std::string padding;

      for (int i = 0; i < (width - (int) buf.size()); ++i) {
        padding << " ";
      }

      std::string ret;
      ret << padding << buf;
      return ret;
    };


    int first_h = 0;
    int last_h  = 0;
    std::string same_buf = vector_dump(src, 0, output_int);

    auto dump_row = [&first_h, &last_h, &same_buf, &pad, int_width, prefix_width] () -> std::string {
      std::string ret;

      ret << "  ";

      if (first_h < last_h) {
        ret << pad(first_h, int_width) << "-"  << pad(last_h, int_width);
      } else {
        ret << pad(first_h, prefix_width);
      }

       ret << ": [" << same_buf<< "]\n";

      return ret;
    };

    ret << "[\n";

    for (int h = 0; h < src.rows(); ++h) {
      std::string buf = vector_dump(src, h, output_int);

      if (buf == same_buf) {
        last_h = h;
      } else {
        ret << dump_row();

        first_h    = h;
        last_h     = h;
        same_buf   = buf;
      }
    }

    ret << dump_row();
    ret << "]";
  }

  return ret;
}


/**
 * @brief Simple display of matrix values
 *
 * Common values and rows are not combined.  
 * Probably not used any more, kept for reference.
 */
std::string matrix_dump_simple(MatrixAdapter const &src) {
  std::string buf;
  buf << src.dump_dim() << " ";

  if (src.empty()) {
    buf = "[]";
    return buf;
  }
 
  buf  << "[\n";

  for (int i = 0; i < src.rows(); ++i) {
    buf << "  " << i << ": [";

    for (int j = 0; j < src.columns(); ++j) {
      if (src.at(i,j) == 0.0f) {
        buf << "0";
      } else {
        buf  << src.at(i,j);
      }

      buf << ", ";
    }
    buf << "]\n";
  }
  buf << "]";

  return buf;
}


bool check_precision(float lhs, float rhs, int bit_diff, CompareStats *stats, bool do_show) {
  bool ret = true;
  float diff = abs(lhs - rhs);
  int bit = V3DLib::bit_diff(lhs, rhs, bit_diff);

  bool failed = false;

  if (bit_diff > -1) {
    failed = (bit > -1);
  } else {
    failed = (lhs != rhs);
  }
    
  if (failed) {
    if (do_show) {
      warn << "check_precision fail, diff: " << diff << ", " << "bit: " << bit;
    }
    ret = false;
  }

  if (stats != nullptr) {
    stats->total++;
    if (diff > stats->max_diff) stats->max_diff = diff;
    if (bit  > stats->max_bit)  stats->max_bit = bit;

    if (lhs == rhs) {
      stats->exact++;

      if (lhs == 0.0f) {
        stats->zeroes++;
      }
    } else {
      if (ret) stats->same++;
    }
  }

  return ret;
}


bool same_intern(
  MatrixAdapter const &lhs,
  MatrixAdapter const &rhs,
  int bit_diff,
  CompareStats &stats,
  bool do_show
) {
  //warn << "Called same_intern(Adapter, Adapter)";
  bool ret = true;

  // Special case for 2 input vectors: accept transposed vectors
  if (lhs.columns() == 1 && lhs.columns() == rhs.rows() && lhs.rows() == rhs.columns() ) {
    int size = lhs.rows();
    for (int i = 0; i < size; ++i) {
      if (!check_precision(lhs.at(i, 0), rhs.at(0, i), bit_diff, &stats, do_show)) {
         if (ret) {
          stats.first_i = i;
          stats.first_j = 0;
        }

        ret = false;
        if (stats.fail_on_first()) break;
      }      
    }

    return ret;
  }

  //
  // Do full matrices
  //

  if (lhs.rows() != rhs.rows() || lhs.columns() != rhs.columns() ) {
     warn << "Fail same_intern(Adapter, Adapter) dimensions differ: "
          << "lhs: " << lhs.dump_dim() << ", "
          << "rhs: " << rhs.dump_dim();

     return false;
  }

  for (int i = 0; i < (int) rhs.rows(); ++i) {
    if (stats.fail_on_first() && !ret) break;

    for (int j = 0; j < (int) rhs.columns(); ++j) {
      if (!check_precision(lhs.at(i, j), rhs.at(i, j), bit_diff, &stats, ret && do_show)) {
        if (ret) {  // Register first fail only
          stats.first_i = i;
          stats.first_j = j;
        }

        ret = false;
        if (stats.fail_on_first()) break;
      }      
    }
  }

  return ret;
}


bool same(MatrixAdapter const &lhs, MatrixAdapter const &rhs, int bit_diff, bool show_stats) {
  CompareStats stats(true);
  bool ret = same_intern(lhs, rhs, bit_diff, stats);

  if (stats.failed()) {
    warn << "Fail same() at (i,j): (" << stats.first_i << ", " << stats.first_j  << ")";
  }

  if (show_stats) warn << stats.dump();
  return ret;
}


namespace  bitdiff_stats {
namespace {

/**
 * @brief Keep track of determined bit diff's for a given compare.
 *
 * The first element store is for `bit_diff == -1`, which means that
 * bit diff is within limits.
 */
class StatsStruct {
public:
  StatsStruct() { init(); }

  /**
   * bit_diff is the index of the first bit (RTL) that is different.
   * bit_diff == -1 mean exact match
   */
  void inc(int bit_diff) {
    bit_diff++;
    //warn << "bit_diff: " << bit_diff;

    assert(0 <= bit_diff && bit_diff < SIZE);
    arr[bit_diff]++;
  }

  std::string dump() {
    bool do_percent = true;
    int total = 0;

    if (do_percent) {
      for (int i = 0; i < SIZE; ++ i) {
        total += arr[i];
      }
    }

    std::string ret;
    ret << "Total: " << total << " <";

    for (int i = 0; i < SIZE; ++ i) {
      if (i > 0) ret << ", ";
      ret << arr[i];
    }
    ret << ">";

    if (do_percent) {
      // Do cumulative percentages
			int sum = 0;

      ret << "\n     <";

      for (int i = 0; i < SIZE; ++ i) {
        if (i > 0) ret << ", ";

				sum += arr[i];
        ret << (sum*100/total) << "%";
      }

      ret << ">";
    }

    return ret;
  }

private:
  static const int SIZE = 32 + 1;  // extra element for -1
  int arr[SIZE];

  void init() {
    for (int i = 0; i < SIZE; ++ i) {
      arr[i] = 0;
    }
  }
};


/**
 * @brief Global store for bit diff comparisons
 *
 * Values are stored per id, which is a unique value for a given compare.
 * Keeping the id unique is the responsibility of the programmer.
 */
class Stats : public std::map<int, StatsStruct> {
public:
  void add(int id, int bit_diff) {
    auto & val = (*this)[id];
    val.inc(bit_diff);
  }

  std::string dump() {
    std::string ret;

    if (empty()) {
      ret << "<<bitdiff stats: none>>";
    } else {
      ret << "<<bitdiff stats (first index == -1):\n";

      for (auto val : *this) {
        ret << "  " << val.first << ": " << val.second.dump() << "\n";
      }

      ret << ">>";
    }

    return ret;
  }
};

Stats s_stats;

} // anon namespace


void add(float lhs, float rhs, int id) {
  int bit = V3DLib::bit_diff(lhs, rhs, -1);
  s_stats.add(id, bit);
}


void add(MatrixAdapter const &lhs, MatrixAdapter const &rhs, int id) {
  // Compare both matrices completely and register the largest bit_diff detected.
  CompareStats stats(false);
  same_intern(lhs, rhs, -1, stats, false);

  s_stats.add(id, stats.max_bit);
}

void dump() {
  warn << s_stats.dump();
}

} // namespace  bitdiff_stats
