#ifndef _GLOBAL_BASICS_H
#define _GLOBAL_BASICS_H
#include <string>
#include <vector>

//
// Convenience definitions 
//

template<typename T>
inline std::vector<T> &operator<<(std::vector<T> &a, T val) {
  a.push_back(val);  
  return a;
}


template<typename T>
inline std::vector<T> &operator<<(std::vector<T> &a, std::vector<T> const &b) {
  a.insert(a.end(), b.begin(), b.end());
  return a;
}


inline std::vector<std::string> &operator<<(std::vector<std::string> &a, char const *str) {
  a.push_back(str);  
  return a;
}


inline std::string &operator<<(std::string &a, char const *str)        { a += str; return a; }
inline std::string &operator<<(std::string &a, std::string const &str) { a += str; return a; }
inline std::string &operator<<(std::string &a, int val)                { a += std::to_string(val); return a; }
inline std::string &operator<<(std::string &a, long val)               { a += std::to_string(val); return a; }
inline std::string &operator<<(std::string &a, uint32_t val)           { a += std::to_string(val); return a; }
inline std::string &operator<<(std::string &a, uint64_t val)           { a += std::to_string(val); return a; }
inline std::string &operator<<(std::string &a, bool val)               { a += val?"true":"false";  return a; }

std::string &operator<<(std::string &a, float val);
std::string &operator<<(std::string &a, double val);

std::string int_hex(int val);


//
// Helper Functions
//

template<typename T>
inline std::string dump(std::vector<T> const &vec) {
  std::string buf;

  for (int i = 0; i < (int) vec.size(); ++i) {
    if (!buf.empty()) buf << ", ";
    buf << vec[i];
  }

  std::string ret;
  ret << "<" << buf << ">";
  return ret;
}

#endif  // _GLOBAL_BASICS_H
