#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "errorlog.h"

char error_messages[MAX_ERRORS][LOG_MSG_SHORT];
int error_count = 0;

// migrated from code_instrumentation.c/code_instrumentation.h (removed)
int instrumentation_muted = 0;

const char *ss_strerr(enum ss_err err) {
  switch (err) {
    case SS_ERROR: return "ERROR";
    case SS_OK: return "OK";
    default: return "ERROR UNKNOWN";
  }
}

void code_instrumentation_mute() {
  if (instrumentation_muted < 1) {
    instrumentation_muted = 1;
  } else {
    instrumentation_muted++;
  }
}

void code_instrumentation_unmute() {
  if (instrumentation_muted > 0) {
    instrumentation_muted--;
  } else {
    instrumentation_muted = 0;
  }
}

void clear_errors(void) {
  error_count = 0;
  return;
}

void dump_errors(FILE *f) {
  if (instrumentation_muted) {
    return;
  }

  for (int i = 0; i < error_count; i++) {
    fprintf(f, "   %s\n", error_messages[i]);
  }
  return;
}

int remember_error(const char *severity, const char *file, const int line, const char *function, const char *format, ...) {
  if (instrumentation_muted) {
    return 0;
  }

  char message[LOG_MSG_LONG];
  int retVal = 0;

  do {

    if (error_count >= MAX_ERRORS) {
      fprintf(stderr, "Too many errors encountered.  Error trace:\n");
      dump_errors(stderr);
      exit(-1);
    }

    va_list argp;
    va_start(argp, format);
    vsnprintf(message, LOG_MSG_LONG, format, argp);
    va_end(argp);

    snprintf(error_messages[error_count], LOG_MSG_SHORT, "[%s] %s:%d:%s(): %s", severity, file, line, function, message);
    error_count++;

    // Also record the error into the general log
    log_message(severity, file, function, line, "%s",  message);

  } while (0);

  return retVal;
}
