#include <string.h>
#include <time.h>
#include <stdlib.h>

/*
  Various functions for freeing data structures.
  freez() is the same as free(), but just checks to make sure that it hasn't been
  passed a null pointer.  This makes the latter functions a little simpler.
 */
void freez(void *p) {
  if (p) {
    free(p);
  }
  return;
}

/*
  Remove any trailing line feed or carriage returns from the input string.
 */
void trim_crlf(char *line) {
  if (!line) {
    return;
  }
  int len = strlen(line);

  while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
    line[--len] = 0;
  }
  return;
}

struct tm *format_time_ISO8601(time_t t, char *buf, size_t len) {
  struct tm *lt = localtime(&t);
  if (!lt) {
    return lt;
  }
  strftime(buf, len, "%FT%T%z", lt);
  // 2019-07-24T11:49:20+09:30
  return lt;
}
