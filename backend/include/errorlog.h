void clear_errors(void);
void dump_errors(FILE *F);
int remember_error(const char *file,const int line, const char *function,const char *message,...);
#define LOG_ERROR(MSG,...) { retVal=-1; remember_error(__FILE__,__LINE__,__FUNCTION__,MSG,__VA_ARGS__); break; }
#define LOG_MAYBE_ERROR(LOGP,MSG,...) { retVal=-1; if (LOGP) { remember_error(__FILE__,__LINE__,__FUNCTION__,MSG,__VA_ARGS__); } break; }

#define LOG_INFO(MSG, ...) log_message(__FILE__,__FUNCTION__,__LINE__,MSG,__VA_ARGS__)

#define MAX_ERRORS 20
extern char error_messages[MAX_ERRORS][1024];
extern int error_count;
