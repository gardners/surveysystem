#ifndef __ERRORLOG_H__
#define __ERRORLOG_H__

#define LOG_MSG_LONG 65536
#define LOG_MSG_SHORT 1024

enum ss_err {
  SS_ERROR = -1,
  SS_OK = 0,
};
const char *ss_strerr(enum ss_err err); // errorlog.c

void clear_errors(void);
void dump_errors(FILE *F);
int remember_error(const char *severity, const char *file, const int line,const char *function, const char *message, ...);
void code_instrumentation_mute();
void code_instrumentation_unmute();

#define LOG_MUTE() code_instrumentation_mute()
#define LOG_UNMUTE() code_instrumentation_unmute()

#define LOG_INFOV(MSG, ...) log_message("INFO", __FILE__, __FUNCTION__, __LINE__, MSG, __VA_ARGS__)
#define LOG_INFO(MSG) log_message("INFO", __FILE__, __FUNCTION__, __LINE__, "%s", MSG)

#define LOG_WARNV(MSG, ...) {                                                              \
    remember_error("WARN", __FILE__, __LINE__, __FUNCTION__, MSG, __VA_ARGS__);            \
  }

#define BREAK_ERRORV(MSG, ...) {                                                           \
    retVal = SS_ERROR;                                                                     \
    remember_error(ss_strerr(retVal), __FILE__, __LINE__, __FUNCTION__, MSG, __VA_ARGS__); \
    break;                                                                                 \
  }

#define BREAK_ERROR(MSG) {                                                                 \
    retVal = SS_ERROR;                                                                     \
    remember_error(ss_strerr(retVal), __FILE__, __LINE__, __FUNCTION__, "%s", MSG);        \
    break;                                                                                 \
  }

#define BREAK_CODEV(CODE, MSG, ...) {                                                      \
    retVal = CODE;                                                                         \
    remember_error(ss_strerr(retVal), __FILE__, __LINE__, __FUNCTION__, MSG, __VA_ARGS__); \
    break;                                                                                 \
  }

#define BREAK_CODE(CODE, MSG) {                                                            \
    retVal = CODE;                                                                         \
    remember_error(ss_strerr(retVal), __FILE__, __LINE__, __FUNCTION__, "%s", MSG);        \
    break;                                                                                 \
  }

#define CHECKPOINT() fprintf(stderr, "%s:%d:%s():pid=%d: Checkpoint\n", __FILE__, __LINE__, __FUNCTION__, getpid())

FILE *open_log(char *name);
int log_message(const char *severity, const char *file, const char *function, const int line, char *format, ...);

#define MAX_ERRORS 20
extern char error_messages[MAX_ERRORS][LOG_MSG_SHORT];
extern int error_count;

#endif
