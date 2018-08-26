#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_ERRORS 20
char error_messages[MAX_ERRORS][1024];
int error_count=0;

void clear_errors(void)
{
  error_count=0;
  return;
}

void dump_errors(FILE *f)
{
  for(int i=0;i<error_count;i++)
    {
      fprintf(f,"   %s\n",error_messages[i]);	     
    }
  return;
}

int remember_error(const char *file,const int line, const char *function,const char *message,const char *data)
{
  if (error_count>=MAX_ERRORS) {
    fprintf(stderr,"Too many errors encountered.  Error trace:\n");
    dump_errors(stderr);
    exit(-1);
  }

  snprintf(error_messages[error_count],1024,
	   "%s:%d:%s(): %s (data='%s')",
	   file,line,function,message,data);
  error_count++;
  return 0;
}
