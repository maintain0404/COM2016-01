#include <iostream>
#include <stdio.h>
#include <stdarg.h>

/// LOGGING
#define TRACE 0
#define DEBUG 20
#define INFO 40
#define WARN 60
#define ERROR 80
#define CRITICAL 100

int LOG_LEVEL = TRACE;

#define LOG(level, msg, ...)               \
    if (level >= LOG_LEVEL)                \
    {                                      \
        printf("[%s]  %s\n", #level, msg); \
    }

#ifdef COMPILE_DEBUG
#define EXIT exit(EXIT_FAILURE);
#else
#define EXIT ;
#endif

#define EXIT_WITH_LOG_CRITICAL(msg, ...) \
    LOG(CRITICAL, msg##__VA_ARGS__)      \
    LOG(CRITICAL, strerror(errno))       \
    EXIT

#define LOG_TRACE(msg, ...) \
    LOG(TRACE, msg##__VA_ARGS__)

#define LOG_DEBUG(msg, ...) \
    LOG(DEBUG, msg##__VA_ARGS__)

#define LOG_INFO(msg, ...) \
    LOG(INFO, msg##__VA_ARGS__)

#define LOG_WARN(msg, ...) \
    LOG(WARN, msg##__VA_ARGS__)

#define LOG_ERROR(msg, ...) \
    LOG(ERROR, msg##__VA_ARGS__)

#define LOG_CRITICAL(msg, ...) \
    LOG(CRITICAL, msg##__VA_ARGS__)

/// END LOGGING