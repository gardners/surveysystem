#ifndef __UTILS_H__
#define __UTILS_H__

#include <time.h>

void freez(void *p);
void trim_crlf(char *line);
struct tm *format_time_ISO8601(time_t t, char *buf, size_t len);

char *parse_line(const char *body, char separator, char **saveptr); // #461
#endif
