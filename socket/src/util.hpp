#include <iostream>
#include <stdio.h>
#include <stdarg.h>

#define COMPILE_MAIN
#define COMPILE_DEBUG

/// LOGGING
#define TRACE 0
#define DEBUG 20
#define INFO 40
#define WARN 60
#define ERROR 80
#define CRITICAL 100

int LOG_LEVEL = TRACE;

#define LOG(level, msg)                       \
    if (level >= LOG_LEVEL)                   \
    {                                         \
        std::cout << "[" << #level << "] "    \
                  << " " << msg << std::endl; \
    }

#ifdef COMPILE_DEBUG
#define EXIT exit(EXIT_FAILURE);
#else
#define EXIT ;
#endif

#define EXIT_WITH_CRITICAL(msg)    \
    LOG(CRITICAL, msg)             \
    LOG(CRITICAL, strerror(errno)) \
    EXIT

#define LOG_TRACE(msg) \
    LOG(TRACE, msg)

#define LOG_DEBUG(msg) \
    LOG(DEBUG, msg)

#define LOG_INFO(msg) \
    LOG(INFO, msg)

#define LOG_WARN(msg) \
    LOG(WARN, msg)

#define LOG_ERROR(msg) \
    LOG(ERROR, msg)

#define LOG_CRITICAL(msg) \
    LOG(CRITICAL, msg)

/// END LOGGING