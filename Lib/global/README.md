# Components which can be used in any project

**Version:** 14

## Version history

  - 14: 
    * Added Timer class.
    * Renamed files of class `log` to `Log`.
    * `global` is now officially an internal library. It will also be used in project `Granlund`.
    * Added this README
  - 13 - Isolated log and constituent files. There are no more dependencies whatsoever on V3DLib.
  - 12 - removed `void assertq(const std::string &msg)`, didn't fire
  - 11 - Added call enable_log_file() to init default file logging
  - 10 - Added parameter `do_break` to assertq()
  -  9 - Prevent segfault on global shutdown, see Note 1 of Logger:msg()
  -  8 - Added globals 'global', 'fixed' and associated
         Modifier flags moved to anon namespace; they are now global over all log calls
         Pretty extensive refactoring to make this work
         Added unit tests for logging, especially for new modifiers
  -  7 - Added rollover, improved file permissions
  -  6 - restored log_to_cout()
  -  5 - Added logging to file
  -  4 - Added thrw flag, option to suppres console output
  -  3 - Added hex flag

