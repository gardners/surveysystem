#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "errorlog.h"
#include "survey.h"

int log_recursed = 0;

 FILE *open_log(char *name) {
  char log_name[1024];
  char log_file[1024];

  if (!name) {
      fprintf(stderr, "Log name is null\n");
      return NULL;
  }
  if (name[0] == 0) {
      fprintf(stderr, "Log name is empty\n");
      return NULL;
  }

  if (generate_path(name, log_file, 1024)) {
      fprintf(stderr, "generate_path('%s') failed to build path for log file: '%s'\n", log_name, log_file);
      fprintf(stderr, "'%s'\n", log_file);
      return NULL;
  }

  FILE *lf = fopen(log_file, "a");
  if (!lf) {
      fprintf(stderr, "Could not open log file '%s' for append: %s\n", log_file, strerror(errno));
  }

  return lf;
}

int log_message(const char *file, const char *function, const int line, char *format, ...) {

  int retVal = 0;

  char log_name[1024];
  char message[65536];
  FILE *lf = NULL;

  do {
    // Don't allow us reporting errors via LOG_ERROR cause infinite recursion
    log_recursed++;
    if (log_recursed > 1) {
      break;
    }

    time_t now = time(0);
    struct tm *tm = localtime(&now);

    char *custom_path = getenv("SS_LOG_FILE");

    if (!custom_path) {

      if (!tm) {
        snprintf(log_name, 1024, "logs/surveysystem-UNKNOWNTIME.log");
      } else {
        snprintf(log_name, 1024, "logs/surveysystem-%04d%02d%02d.%02d.log",
                1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour);
      }
      lf = open_log(log_name);

    } else {
      lf = open_log(custom_path);
    }

    if (!lf) {
      LOG_ERRORV("Could not open log file '%s'", log_name);
    }

    va_list argp;
    va_start(argp, format);
    vsnprintf(message, 65536, format, argp);
    va_end(argp);

    if (tm) {
      fprintf(lf, "%04d/%02d/%02d.%02d:%02d.%d:%s:%d:%s():%s\n",
              1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour,
              tm->tm_min, tm->tm_sec, file, line, function, message);
    } else {
      fprintf(lf, "\?\?\?\?/\?\?/\?\?.\?\?:\?\?.\?:%s:%d:%s():%s\n", file, line,
              function, message);
    }

    fclose(lf);

  } while (0);

  log_recursed--;
  return retVal;
}
