#ifndef __ERRORLOG_H__
#define __ERRORLOG_H__

#include <math.h>

#define LOG_MSG_LONG 65536
#define LOG_MSG_SHORT 1024

enum ss_err {
  SS_EVAL = -1,                 // generic errors
  SS_OK = 0,

  SS_ERROR,
  SS_ERROR_MEM,                 // memory
  SS_ERROR_ARG,                 // function args
  SS_ERROR_OPEN_FILE,           // error opening file
  SS_ERROR_CREATE_DIR,          // error creating directory

  // section: request validation
  SS_INVALID = 100,
  SS_INVALID_METHOD,
  SS_INVALID_SURVEY_ID,
  SS_INVALID_SESSION_ID,
  SS_INVALID_ACTION,
  SS_INVALID_CREDENTIALS,
  SS_INVALID_CREDENTIALS_PROXY,
  SS_INVALID_CONSISTENCY_HASH,
  SS_NOSUCH_SESSION,            // session not found
  SS_SESSION_EXISTS,            // (new session) session exoists
  SS_MISSING_ANSWER,            // missing in request
  SS_NOSUCH_ANSWER,             // not in session
  SS_INVALID_ANSWER,            // malformed (serialised)
  SS_INVALID_UUID,              // malformed value for QTYPE_UUID
  SS_MISMATCH_NEXTQUESTIONS,    // answers don't match ses->next_questions
  SS_NOSUCH_QUESTION,           // question not defined

  // section: system errors
  SS_SYSTEM = 200,
  SS_SYSTEM_FILE_PATH,
  SS_SYSTEM_CREATE_SURVEY_SHA,
  SS_SYSTEM_LOCK_SESSION,
  SS_SYSTEM_LOAD_SESSION,
  SS_SYSTEM_LOAD_SESSION_META,
  SS_SYSTEM_WRITE_SESSION_META,
  SS_SYSTEM_GET_NEXTQUESTIONS,
  SS_SYSTEM_GET_ANALYSIS,
  SS_SYSTEM_SAVE_SESSION,

  // section: configuration errors
  SS_CONFIG = 300,
  SS_CONFIG_PROXY,
  SS_CONFIG_MALFORMED_SURVEY,
  SS_CONFIG_MALFORMED_SESSION,
  SS_CONFIG_SURVEY_HOME,

  SS_ERROR_MAX,
};
#define ERROR_SECTION(CODE) (int) ceil(CODE/100) * 100

const char *get_error(int err, int is_section, char *nope); // errorlog.c

void clear_errors(void);
void dump_errors(FILE *F);
int remember_error(const char *severity, const char *file, const int line,const char *function, const char *message, ...);
void code_instrumentation_mute();
void code_instrumentation_unmute();

#define LOG_MUTE() code_instrumentation_mute()
#define LOG_UNMUTE() code_instrumentation_unmute()

#define LOG_INFOV(MSG, ...) log_message("[INFO]", __FILE__, __FUNCTION__, __LINE__, MSG, __VA_ARGS__)
#define LOG_INFO(MSG) log_message("[INFO]", __FILE__, __FUNCTION__, __LINE__, "%s", MSG)

#define LOG_CODEV(CODE, MSG, ...) log_message(get_error(CODE, 0, "[INFO]"), __FILE__, __FUNCTION__, __LINE__, MSG, __VA_ARGS__)
#define LOG_CODE(CODE, MSG) log_message(get_error(CODE, 0, "[INFO]"), __FILE__, __FUNCTION__, __LINE__, "%s", MSG)

#define LOG_MSGV(SEV, MSG, ...) {                                                                             \
  remember_error(SEV, __FILE__, __LINE__, __FUNCTION__, MSG, __VA_ARGS__);                                    \
}

#define LOG_WARNV(MSG, ...) {                                                                                 \
  remember_error("[WARN]", __FILE__, __LINE__, __FUNCTION__, MSG, __VA_ARGS__);                               \
}

#define BREAK_ERRORV(MSG, ...) {                                                                              \
  retVal = SS_EVAL;                                                                                          \
  remember_error(get_error(retVal, 0, "[ERROR] unkown"), __FILE__, __LINE__, __FUNCTION__, MSG, __VA_ARGS__); \
  break;                                                                                                      \
}

#define BREAK_ERROR(MSG) {                                                                                    \
  retVal = SS_EVAL;                                                                                          \
  remember_error(get_error(retVal, 0, "[ERROR] unkown"), __FILE__, __LINE__, __FUNCTION__, "%s", MSG);        \
  break;                                                                                                      \
}

#define BREAK_CODEV(CODE, MSG, ...) {                                                                         \
  retVal = CODE;                                                                                              \
  remember_error(get_error(retVal, 0, "[ERROR] unkown"), __FILE__, __LINE__, __FUNCTION__, MSG, __VA_ARGS__); \
  break;                                                                                                      \
}

#define BREAK_CODE(CODE, MSG) {                                                                               \
  retVal = CODE;                                                                                              \
  remember_error(get_error(retVal, 0, "[ERROR] unkown"), __FILE__, __LINE__, __FUNCTION__, "%s", MSG);        \
  break;                                                                                                      \
}

#define BREAK_IF(EXPR, CODE, MSG) {                                                                           \
  if (EXPR) {                                                                                                 \
    BREAK_CODEV(CODE, "'%s' failed: %s", #EXPR, MSG);                                                         \
  }                                                                                                           \
}

#define CHECKPOINT() fprintf(stderr, "%s:%d:%s():pid=%d: Checkpoint\n", __FILE__, __LINE__, __FUNCTION__, getpid())

FILE *open_log(char *name);
int log_message(const char *severity, const char *file, const char *function, const int line, char *format, ...);

#define MAX_ERRORS 20
extern char error_messages[MAX_ERRORS][LOG_MSG_SHORT];
extern int error_count;

#endif
