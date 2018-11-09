#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#include "errorlog.h"

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

int remember_error(const char *file,const int line, const char *function,const char *format,...)
{

  char message[65536];

  int retVal=0;

  do {
  
    if (error_count>=MAX_ERRORS) {
      fprintf(stderr,"Too many errors encountered.  Error trace:\n");
      dump_errors(stderr);
      exit(-1);
    }

    va_list argp;
    va_start(argp,format);
    vsnprintf(message,65536,format,argp);
    va_end(argp);
    
    snprintf(error_messages[error_count],1024,
	     "%s:%d:%s(): %s",
	     file,line,function,message);
    
    error_count++;

    // Also record the error into the general log
    LOG_INFOV("LOG_ERROR : %s",message);

  } while(0);
    
  return retVal;
}

