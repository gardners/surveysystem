#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "errorlog.h"

char error_messages[MAX_ERRORS][LOG_MSG_SHORT];
int error_count = 0;

// migrated from code_instrumentation.c/code_instrumentation.h (removed)
int instrumentation_muted = 0;

/**
 * get error string for given code
 * #260, #414
 */
const char *get_error(int err, int section, char *nope) {

  int code = (section) ? ERROR_SECTION(err) : err;

  switch (code) {
    case SS_EVAL:                         return "[ERROR]";

    case SS_OK:                           return (section) ? "[Error]" : "[OK]";
    case SS_ERROR:                        return "[ERROR]";
    case SS_ERROR_MEM:                    return "[ERROR] memory allocation";
    case SS_ERROR_ARG:                    return "[ERROR] function argument";
    case SS_ERROR_READ_FILE:              return "[ERROR] opening file  (read)";
    case SS_ERROR_CREATE_DIR:             return "[ERROR] creating directory";

    case SS_INVALID:                      return "[ERROR] invalid request";
    case SS_INVALID_METHOD:               return "[ERROR] invalid method";
    case SS_INVALID_SURVEY_ID:            return "[ERROR] malformed survey_id";
    case SS_INVALID_SESSION_ID:           return "[ERROR] malformed session_id";
    case SS_INVALID_ACTION:               return "[ERROR] action";
    case SS_INVALID_CREDENTIALS:          return "[ERROR] unauthorised";
    case SS_INVALID_CREDENTIALS_PROXY:    return "[ERROR] proxy is unauthorised";
    case SS_INVALID_CONSISTENCY_HASH:     return "[ERROR] If-Match header/param invalid or missing";
    case SS_NOSUCH_SESSION:               return "[ERROR] no such session";
    case SS_SESSION_EXISTS:               return "[ERROR] session exists already";
    case SS_MISSING_ANSWER:               return "[ERROR] missing answer";
    case SS_INVALID_ANSWER:               return "[ERROR] invalid answer";
    case SS_NOSUCH_ANSWER:                return "[ERROR] no such answer";
    case SS_MISMATCH_NEXTQUESTIONS:       return "[ERROR] missing answer for required question";
    case SS_NOSUCH_QUESTION:              return "[ERROR] no such question";
    case SS_INVALID_UUID:                 return "[ERROR] malformed uuid";

    case SS_SYSTEM:                       return "[ERROR] system";
    case SS_SYSTEM_FILE_PATH:             return "[ERROR] generating file path";
    case SS_SYSTEM_CREATE_SURVEY_SHA:     return "[ERROR] failed to create sha1 copy of survey";
    case SS_SYSTEM_LOCK_SESSION:          return "[ERROR] failed to lock session";
    case SS_SYSTEM_LOAD_SESSION:          return "[ERROR] failed to load session";
    case SS_SYSTEM_LOAD_SESSION_META:     return "[ERROR] failed to load session header";
    case SS_SYSTEM_WRITE_SESSION_META:    return "[ERROR] failed to write session header";
    case SS_SYSTEM_GET_NEXTQUESTIONS:     return "[ERROR] failed to get next questions";
    case SS_SYSTEM_SAVE_SESSION:          return "[ERROR] failed to save session";

    case SS_CONFIG:                       return "[ERROR] configuration error";
    case SS_CONFIG_PROXY:                 return "[ERROR] middleware configuration";
    case SS_CONFIG_MALFORMED_SURVEY:      return "[ERROR] malformed survey";
    case SS_CONFIG_MALFORMED_SESSION:     return "[ERROR] malformed session (meta data?)";
    case SS_CONFIG_SURVEY_HOME:           return "[ERROR] missing or invalid environment variable: SURVEY_HOME";

    default:                              return nope;
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

    snprintf(error_messages[error_count], LOG_MSG_SHORT, "%s %s:%d:%s(): %s", severity, file, line, function, message);
    error_count++;

    // Also record the error into the general log
    log_message(severity, file, function, line, "%s",  message);

  } while (0);

  return retVal;
}
