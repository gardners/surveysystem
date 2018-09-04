
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


    
  } while(0);

  return retVal;
}
