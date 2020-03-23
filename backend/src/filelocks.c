#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/file.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "errorlog.h"
#include "survey.h"

struct locked_files {
  FILE *file_handle;
  char *path;
};

#define MAX_LOCKS 16
struct locked_files locks[MAX_LOCKS];
int lock_count=0;

int lock_session(char *session_id)
{
  int retVal=0;
  do {

    time_t now = time(0);
    struct tm *tm = localtime(&now);
    
    if (!tm) LOG_ERROR("localtime() failed. Cannot acquire lock.");
    struct timeval nowtv;

    // If gettimeofday() fails or returns an invalid value, all else is lost!
    if (gettimeofday(&nowtv, NULL) == -1) LOG_ERROR("gettimeofday() failed");
    
    char session_prefix[5];
    char session_path_suffix[1024];
    char lock_path[1024];

    if (!session_id) LOG_ERROR("session_id is NULL");
    if (validate_session_id(session_id)) LOG_ERRORV("Session ID '%s' is malformed",session_id);

    for (int i=0;i<4;i++) {
	session_prefix[i]=session_id[i];
    }
    session_prefix[4]=0;

    // Create subdirectory in locks directory if required
    snprintf(session_path_suffix,1024,"locks");
    if (generate_path(session_path_suffix,lock_path,1024)) LOG_ERRORV("generate_path('%s') failed to build path for lock path when locking session '%s'",session_path_suffix,session_id);
    mkdir(lock_path,0750);

    snprintf(session_path_suffix,1024,"locks/%s",session_prefix);
    if (generate_path(session_path_suffix,lock_path,1024)) LOG_ERRORV("generate_path('%s') failed to build path for lock path when locking session '%s'",session_path_suffix,session_id);
    mkdir(lock_path,0750);
    
    snprintf(session_path_suffix,1024,"locks/%s/lock.%s",session_prefix,session_id);
    if (generate_path(session_path_suffix,lock_path,1024)) LOG_ERRORV("generate_path('%s') failed to build path while locking session '%s'",session_path_suffix,session_id);

    // See if we already hold a lock to this session
    int i;
    for (i=0;i<lock_count;i++) {
      if (!strcmp(lock_path,locks[i].path)) {
	  break;
      }
    }
    
    if (i<lock_count) {
      // We already hold a lock. Nothing to do
      retVal=0;
      break;
    }

    // No existing lock, so acquire one

    if (lock_count>=MAX_LOCKS) LOG_ERROR("Too many file locks open. Bug or increase MAX_LOCKS?");
    
    FILE *f=fopen(lock_path,"a");
    if (!f) LOG_ERRORV("Could not open lock file '%s' for append.",lock_path);
    if (flock(fileno(f),LOCK_EX)) LOG_ERRORV("flock('%s',LOCK_EX) failed",lock_path);
    fprintf(f,"%04d/%02d/%02d"
	    ".%02d:%02d.%d"
	    ":%ld.%06ld"
	    ":Lock acquired by pid %d.\n",
	    1900+tm->tm_year,tm->tm_mon+1,tm->tm_mday,
	    tm->tm_hour,tm->tm_min,tm->tm_sec,
	    nowtv.tv_sec,nowtv.tv_usec,
	    getpid());
    fflush(f);

    // Record the lock
    locks[lock_count].file_handle=f;
    locks[lock_count].path=strdup(lock_path);
    if (!locks[lock_count].path) LOG_ERROR("strdup() failed when remembering file lock");
    lock_count++;
    
  } while(0);

  return retVal;
}

int release_my_session_locks(void)
{
  int retVal=0;

  do {
    // Release locks and flush lock list
    for (int i=0;i<lock_count;i++) {
      if (flock(fileno(locks[i].file_handle),LOCK_UN)) LOG_ERRORV("flock('%s',LOCK_UN) failed",locks[i].path);
      
      // #284 close file handle
      if (locks[i].file_handle) {
	fclose(locks[i].file_handle);
      }
      
      locks[i].file_handle=NULL;
      free(locks[i].path);
    }
    lock_count=0;

    
  } while(0);

  return retVal;
}
