void clear_errors(void);
void dump_errors(FILE *F);
int remember_error(const char *file,const int line, const char *function,const char *message,const char *data);
#define LOG_ERROR(MSG,DATA) { retVal=-1; remember_error(__FILE__,__LINE__,__FUNCTION__,MSG,DATA); break; }
#define LOG_MAYBE_ERROR(LOGP,MSG,DATA) { retVal=-1; if (LOGP) { remember_error(__FILE__,__LINE__,__FUNCTION__,MSG,DATA); } break; }

#define MAX_ERRORS 20
extern char error_messages[MAX_ERRORS][1024];
extern int error_count;
