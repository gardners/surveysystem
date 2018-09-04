
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#include "survey.h"
#include "errorlog.h"
#include "question_types.h"
#include "sha1.h"

int urandombytes(unsigned char *buf, size_t len)
{
  int retVal = -1;
  
  do {
    if (! buf) 
      {
	LOG_ERROR("buf is null","");
	break;
      }
    
    static int urandomfd = -1;
    
    int tries = 0;
    
    if (urandomfd == -1) {
      for (tries = 0; tries < 4; ++tries) 
	{
	  urandomfd = open("/dev/urandom",O_RDONLY);
	  if (urandomfd != -1) 
	    {
	      break;
	    }
	  // LOG_WARN("failed to open /dev/urandom on try #%d, retrying", tries + 1);
	  sleep(1);
	}

      if (urandomfd == -1) {
        LOG_ERROR("failed to open /dev/urandom, stop retrying","");
        perror("open(/dev/urandom)");
        break;
      }
      
    }

    tries = 0;
    while (len > 0) {
      ssize_t i = read(urandomfd, buf, (len < 1048576) ? len : 1048576);
      if (i == -1) 
	{
	  if (++tries > 4) 
	    {
	      LOG_ERROR("failed to read from /dev/urandom, even after retries","");
	      perror("read(/dev/urandom)");
	      if (errno==EBADF) 
		{
		  LOG_ERROR("EBADF on /dev/urandom, resetting urandomfd to -1","");
		  urandomfd=-1;
		}
	      break; // while
	    }
	  else 
	    {
	      // LOG_ERROR("failed to read from /dev/urandom, retry %d, retrying", tries);
	    }
	} 
      else 
	{
	  tries = 0;
	  buf += i;
	  len -= i;
	}
    }
    
    if (len == 0) 
      {
	retVal = 0;
      }
  }
  while (0);

  return retVal;
}

	
int create_session(char *survey_id,char *session_id_out)
{
  int retVal=0;
  
  do {
    char session_id[256];
    char session_prefix[256];
    char session_path_suffix[1024];
    char session_path[1024];

    if (!survey_id) LOG_ERROR("survey_id is NULL","");
    if (!session_id_out) LOG_ERROR("session_id_out is NULL","");

    // Generate new unique session ID
    {
      unsigned char randomness[32];
      if (urandombytes(randomness,32)) LOG_ERROR("get_randomness() failed","");
      sha1nfo s1;
      sha1_init(&s1);
      sha1_write(&s1,(char *)randomness,32);
      unsigned char *hash=sha1_result(&s1);
      time_t t=time(0);
      unsigned int time_low=t&0xffffffffU;
      unsigned int time_high=(t>>32L);

      // Our session IDs look like RFD 4122 UUIDs, and are mostly the same,
      // except we put randomness at the front of the string, since we use
      // that to workout which directory each lives in, and we want them
      // evenly distributed from the outset, instead of directories being
      // filled up one at a time as time advances
      snprintf(session_id,64,"%02x%02x%04x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x",
	       // Here is that randomness at the start
	       hash[6],hash[7],
	       time_low&0xffff,
	       (time_high>>16)&0xffff,
	       (0x0<<12)+((time_high>>12)&0xfff),
	       // Also, we use the process ID as the clock sequence and related things field
	       getpid()&0xffff,
	       hash[0],hash[1],hash[2],hash[3],hash[4],hash[5]);
    }

    snprintf(session_path_suffix,1024,"sessions/%s/%s",session_prefix,session_id);
    if (generate_path(session_path_suffix,session_path,1024)) LOG_ERROR("generate_path() failed to build path for new session",survey_id);

    // Check if session already exists
    
  } while(0);

  return retVal;
}

int validate_session_id(char *session_id)
{
  int retVal=0;
  do {
    if (strlen(session_id)!=30) LOG_ERROR("session_id must be exactly 30 characters long",session_id);
    for(int i=0;session_id[i];i++)
      switch (session_id[i]) {
      case '0': case '1': case '2': case '3':
      case '4': case '5': case '6': case '7':
      case '8': case '9': case 'a': case 'b':
      case 'c': case 'd': case 'e': case 'f':
      case '-':
	// Acceptable characters
	break;
      case 'A': case 'B': case 'C': case 'D':
      case 'E': case 'F':
	LOG_ERROR("session_id must be lower case",session_id);
	break;
      default:
	LOG_ERROR("Illegal character in session_id. Must be a valid UUID",session_id);
	break;
      }
  } while(0);
  return retVal;
}

struct session *load_session(char *session_id)
{
  int retVal=0;
  do {
    if (!session_id) LOG_ERROR("session_id is NULL","");

    if (validate_session_id(session_id)) LOG_ERROR("validate_session_id failed",session_id);

    char session_path[1024];
    char session_path_suffix[1024];
    char session_prefix[5];
    for(int i=0;i<4;i++) session_prefix[i]=session_id[i];
    session_prefix[4]=0;
    
    snprintf(session_path_suffix,1024,"sessions/%s/%s",session_prefix,session_id);
    if (generate_path(session_path_suffix,session_path,1024))
      LOG_ERROR("generate_path() failed to build path for loading session",session_id);

    FILE *s=fopen(session_path,"r");
    if (!s) LOG_ERROR("Could not read session file",session_path);

    LOG_ERROR("load_session() not implemented","COMPLETE ME");
    
    fclose(s);
    
  } while(0);
  return NULL;
}

int save_session(struct session *s)
{
  int retVal=0;
  do {
    if (!s) LOG_ERROR("session structure is NULL","");
    if (s->session_id) LOG_ERROR("s->session_id is NULL","");

    if (validate_session_id(s->session_id)) LOG_ERROR("validate_session_id failed",s->session_id);

    char session_path[1024];
    char session_path_suffix[1024];
    char session_prefix[5];
    for(int i=0;i<4;i++) session_prefix[i]=s->session_id[i];
    session_prefix[4]=0;
    
    snprintf(session_path_suffix,1024,"sessions/%s/%s",session_prefix,s->session_id);
    if (generate_path(session_path_suffix,session_path,1024))
      LOG_ERROR("generate_path() failed to build path for loading session",s->session_id);

    FILE *s=fopen(session_path,"w");
    if (!s) LOG_ERROR("Could not create or open session file for write",session_path);

    LOG_ERROR("save_session() not implemented","COMPLETE ME");
    
    fclose(s);
    
  } while(0);
  return retVal;
}

int session_add_answer(struct session *s,struct answer *a)
{
  int retVal=0;
  do {
    LOG_ERROR("Not implemented","");
  } while(0);
  return retVal;
}
