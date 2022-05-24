/* Copied from
 * https://github.com/hubenchang0515/EasyGL/blob/master/EasyGL/include/EasyGL/common.h
 * GPL-3.0 License
 */

#ifndef LOG_H
#define LOG_H

#include <cstdio>

// enum of LOG_LEVEL
#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_WARNING 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_DEBUG 3
#define LOG_LEVEL_VERBOS 4

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif // !LOG_LEVEL

#if LOG_LEVEL >= LOG_LEVEL_VERBOS
#define VERBOS(fmt, ...) fprintf(stdout, "[%s:%d][VERBOS]: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define VERBOS(fmt, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define DEBUG(fmt, ...) fprintf(stdout, "[%s:%d][DEBUG]: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(fmt, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define INFO(fmt, ...) fprintf(stdout, "[%s:%d][INFO]: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define INFO(fmt, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARNING
#define WARNING(fmt, ...)                                                                          \
    fprintf(stdout, "[%s:%d][WARNING]: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define WARNING(fmt, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_ERROR
#define ERROR(fmt, ...) fprintf(stderr, "[%s:%d][ERROR]: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define ERROR(fmt, ...)
#endif

#endif // !LOG_H
