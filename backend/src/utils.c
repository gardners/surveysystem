#include <time.h>

struct tm *format_time_ISO8601(time_t t, char *buf, size_t len) {
  struct tm *lt = localtime(&t);
  if (!lt) {
    return lt;
  }
  strftime(buf, len, "%FT%T%z", lt);
  // 2019-07-24T11:49:20+09:30
  return lt;
}
