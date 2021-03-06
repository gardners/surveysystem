#ifndef __ERRORLOG_H__
#define __ERRORLOG_H__

void clear_errors(void);
void dump_errors(FILE *F);
int remember_error(const char *severity, const char *file, const int line,
                   const char *function, const char *message, ...);
void code_instrumentation_mute();
void code_instrumentation_unmute();

#define LOG_MUTE() code_instrumentation_mute()
#define LOG_UNMUTE() code_instrumentation_unmute()

#define LOG_ERRORV(MSG, ...)                                                   \
  {                                                                            \
    retVal = -1;                                                               \
    remember_error("ERROR", __FILE__, __LINE__, __FUNCTION__, MSG,             \
                   __VA_ARGS__);                                               \
    break;                                                                     \
  }
#define LOG_WARNV(MSG, ...)                                                    \
  {                                                                            \
    remember_error("WARNING", __FILE__, __LINE__, __FUNCTION__, MSG,           \
                   __VA_ARGS__);                                               \
  }
#define LOG_ERROR(MSG)                                                         \
  {                                                                            \
    retVal = -1;                                                               \
    remember_error("ERROR", __FILE__, __LINE__, __FUNCTION__, "%s", MSG);      \
    break;                                                                     \
  }
#define LOG_MAYBE_ERRORV(LOGP, MSG, ...)                                       \
  {                                                                            \
    retVal = -1;                                                               \
    if (LOGP) {                                                                \
      remember_error("ERROR", __FILE__, __LINE__, __FUNCTION__, MSG,           \
                     __VA_ARGS__);                                             \
    }                                                                          \
    break;                                                                     \
  }
#define LOG_MAYBE_ERROR(LOGP, MSG)                                             \
  {                                                                            \
    retVal = -1;                                                               \
    if (LOGP) {                                                                \
      remember_error("ERROR", __FILE__, __LINE__, __FUNCTION__, "%s", MSG);    \
    }                                                                          \
    break;                                                                     \
  }

#define LOG_INFOV(MSG, ...)  log_message(__FILE__, __FUNCTION__, __LINE__, MSG, __VA_ARGS__)
#define LOG_INFO(MSG) log_message(__FILE__, __FUNCTION__, __LINE__, "%s", MSG)

#define CHECKPOINT() fprintf(stderr, "%s:%d:%s():pid=%d: Checkpoint\n", __FILE__, __LINE__, __FUNCTION__, getpid())


FILE *open_log(char *name);
int log_message(const char *file, const char *function, const int line, char *format, ...);

#define MAX_ERRORS 20
extern char error_messages[MAX_ERRORS][1024];
extern int error_count;

#endif
