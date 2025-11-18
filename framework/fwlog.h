#include <stdio.h>
#ifdef _WIN32
#define LOG_EMERG   0
#define LOG_ALERT   1
#define LOG_CRIT    2
#define LOG_ERR     3
#define LOG_WARNING 4
#define LOG_NOTICE  5
#define LOG_INFO    6
#define LOG_DEBUG   7

#define openlog(ident, option, facility)  ((void)0)
#define closelog()                        ((void)0)
#define syslog(level, fmt, ...) \
do { fprintf(stderr, "[LOG %d] " fmt "\n", level, ##__VA_ARGS__); } while (0)
#else
#include <syslog.h>

#endif /* COMPAT_SYSLOG_H */
