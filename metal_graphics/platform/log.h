#ifndef MXI_LOG_H
#define MXI_LOG_H 1

#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_INFO 3

void log_message(int level, char const *format, ...);

#if !defined(LOG_LEVEL)
#  define LOG_LEVEL 3
#endif

#if LOG_LEVEL_ERROR <= LOG_LEVEL
#  define log_error(...) log_message(LOG_LEVEL_ERROR, __VA_ARGS__)
#endif

#if LOG_LEVEL_WARN <= LOG_LEVEL
#  define log_warn(...) log_message(LOG_LEVEL_WARN, __VA_ARGS__)
#endif

#if LOG_LEVEL_INFO <= LOG_LEVEL 
#  define log_info(...) log_message(LOG_LEVEL_INFO, __VA_ARGS__)
#endif

#endif /* MXI_METAL_GRAPHICS_LOG_H */