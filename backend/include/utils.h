#ifndef __UTILS_H__
#define __UTILS_H__

#include <time.h>

struct tm *format_time_ISO8601(time_t t, char *buf, size_t len);

#endif