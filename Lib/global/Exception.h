#ifndef _GLOBAL_EXCEPTION_H
#define _GLOBAL_EXCEPTION_H
#include <string>
#include <exception>

struct Exception : public std::exception {
  Exception(std::string ss) : s(ss) {}
  ~Exception() throw () {}

  const char *what() const throw() override { return s.c_str(); }
  std::string const &msg() const throw() { return s; }

private:
  std::string s;
};

#endif  // _GLOBAL_EXCEPTION_H
