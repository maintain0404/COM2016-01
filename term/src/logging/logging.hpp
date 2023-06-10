#ifndef __LOGGING_LOGGING_H__
#define __LOGGING_LOGGING_H__
#include <iostream>
#include <ostream>
#include <string>

/// LOGGING
#define TRACE 0
#define DEBUG 20
#define INFO 40
#define WARN 60
#define ERROR 80
#define CRITICAL 100

#define EXIT ;

int _LOG_LEVEL = TRACE;

inline const char *log_header_msg(int level) {
  switch (level) {
  // TRACE, white
  case TRACE: {
    return "TRACE: ";
  };
  // DEBUG, cyan
  case DEBUG: {
    return "\033[0;36mDEBUG: \033[0m";
  };
  // INFO, green
  case INFO: {
    return "\033[0;32mINFO: \033[0m";
  };
  // WARN, yellow
  case WARN: {
    return "\033[0;31mWARN: \033[0m";
  };
  // ERROR, red
  case ERROR: {
    return "\033[0;31mERROR: \033[0m";
  }
  // CRITICAL, red, bold
  case CRITICAL: {
    return "\033[1;31mCRITICAL: \033[0m";
  }
  default: {
    return "";
  };
  };
}

inline void log(int level, std::string msg) {
  if (level >= _LOG_LEVEL) {
    std::string logmsg = log_header_msg(level) + msg;
    std::cout << logmsg << std::endl;
  }
}

inline void setLevel(int level) { _LOG_LEVEL = level; }

#define LOG_TRACE(msg) log(TRACE, msg)
#define LOG_DEBUG(msg) log(DEBUG, msg)
#define LOG_INFO(msg) log(INFO, msg)
#define LOG_WARN(msg) log(WARN, msg)
#define LOG_ERROR(msg) log(ERROR, msg)
#define LOG_CRITICAL(msg) log(CRITICAL, msg)
#define EXIT_WITH_LOG_CRITICAL(msg) log(CRITICAL, msg)
#endif