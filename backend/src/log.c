#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "errorlog.h"
#include "survey.h"

int log_recursed=0;

int log_message(char *file,char *function,int line,char *format,...)
{

  int retVal=0;

  char log_name[1024];
  char log_file[1024];

  char message[65536];
  
  do {

    // Don't allow us reporting errors via LOG_ERROR cause infinite recursion
    log_recursed++;
    if (log_recursed>1) break;
    
    time_t now = time(0);
    struct tm *tm = localtime(&now);
    
    if (!tm)
      snprintf(log_name,1024,"logs/surveysystem-UNKNOWNTIME.log");    
    else
      snprintf(log_name,1024,"logs/surveysystem-%04d%02d%02d.%02d.log",
	       tm->tm_year,tm->tm_mon,tm->tm_mday,tm->tm_hour);	   
    
    if (generate_path(log_name,log_file,1024)) {
      LOG_ERROR("generate_path() failed to build path for log file",log_name);
    }

    va_list argp;
    va_start(argp,format);
    vsnprintf(message,65536,format,argp);
    va_end(argp);

    FILE *lf=fopen(log_file,"a");
    if (!lf) LOG_ERROR("Could not open log file '%s' for append: %s",log_file,strerror(errno));
    fprintf(lf,"%04d/%02d/%02d.%02d:%02d.%d:%s:%d:%s():%s\n",
	    tm->tm_year,tm->tm_mon,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec,
	    file,line,function,
	    message);
    fclose(lf);
    
  } while(0);

  log_recursed--;
  return retVal;
}
