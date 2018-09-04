
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>

#include "survey.h"
#include "serialisers.h"
#include "errorlog.h"
#include "question_types.h"
#include "sha1.h"


int validate_session_id(char *session_id)
{
  int retVal=0;
  do {
    if (strlen(session_id)!=36) LOG_ERROR("session_id must be exactly 36 characters long",session_id);
    if (session_id[0]=='-') LOG_ERROR("session_id may not begin with a dash",session_id);
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

int validate_survey_id(char *survey_id)
{
  int retVal=0;
  do {
    if (!survey_id) LOG_ERROR("survey_id is NULL","");
    if (!survey_id[0]) LOG_ERROR("survey_id is empty string","");
    for(int i=0;survey_id[i];i++)
      switch(survey_id[i]) {
      case ' ': case '.': case '-': case '_':
	break;
      default:
	if (!isalnum(survey_id[i]))
	  LOG_ERROR("Illegal character in survey_id.  Must be 0-9, a-z, or space, period, comma or underscore",survey_id);
	break;
      }
  } while(0);
  return retVal;
}

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

int random_session_id(char *session_id_out)
{
  int retVal=0;
  do {
    if (!session_id_out) LOG_ERROR("session_id_out is NULL","");
				   
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
    snprintf(session_id_out,64,"%02x%02x%04x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x",
	     // Here is that randomness at the start
	     hash[6],hash[7],
	     time_low&0xffff,
	     (time_high>>16)&0xffff,
	     (0x0<<12)+((time_high>>12)&0xfff),
	     // Also, we use the process ID as the clock sequence and related things field
	     getpid()&0xffff,
	     hash[0],hash[1],hash[2],hash[3],hash[4],hash[5]);

  } while(0);
  return retVal;
}

int create_session(char *survey_id,char *session_id_out)
{
  int retVal=0;
  
  do {
    char session_id[256];
    char session_prefix[5];
    char session_path_suffix[1024];
    char session_path[1024];

    if (!survey_id) LOG_ERROR("survey_id is NULL","");
    if (!session_id_out) LOG_ERROR("session_id_out is NULL","");

    if (validate_survey_id(survey_id)) LOG_ERROR("Invalid survey ID","");
    
    snprintf(session_path_suffix,1024,"surveys/%s/current",survey_id);
    if (generate_path(session_path_suffix,session_path,1024)) LOG_ERROR("generate_path() failed to build path for new session",survey_id);
    if ( access( session_path, F_OK ) == -1 ) LOG_ERROR("Survey does not exist",survey_id);
    // Get sha1 hash of survey file
    char survey_sha1[1024];
    if (sha1_file(session_path,survey_sha1)) LOG_ERROR("Could not hash survey specification file",session_path);

    snprintf(session_path_suffix,1024,"surveys/%s/%s",survey_id,survey_sha1);
    if (generate_path(session_path_suffix,session_path,1024)) LOG_ERROR("generate_path() failed to build path for new session",survey_id);
    if ( access( session_path, F_OK ) == -1 ) {
      // No copy of the survey exists with the hash, so make one.
      // To avoid a race condition where the current survey form could be modified,
      // we copy it, and hash it as we go, and rename the copy based on the hash value
      // of what we read.

      // Open input file
      snprintf(session_path_suffix,1024,"surveys/%s/current",survey_id);
      if (generate_path(session_path_suffix,session_path,1024)) LOG_ERROR("generate_path() failed to build path for new session",survey_id);
      FILE *in=fopen(session_path,"r");
      if (!in) LOG_ERROR("Could not read from survey specification file",survey_id);
      
      // So make a hopefully unique name for the temporary file
      char temp_path[1024];
      snprintf(session_path_suffix,1024,"surveys/%s/temp.%lld.%d.%s",survey_id,(long long)time(0),getpid(),survey_sha1);
      if (generate_path(session_path_suffix,temp_path,1024))
	{
	  fclose(in);
	  LOG_ERROR("generate_path() failed to build path for new session",survey_id);
	}
      FILE *c=fopen(temp_path,"w");
      if (!c) {
	fclose(in);
	LOG_ERROR("Could not create temporary file",temp_path);
      }

      char buffer[8192];
      int count;
      sha1nfo s;
      sha1_init(&s);
      
      do {
	count=fread(buffer,1,8192,in);
	if (count<0) LOG_ERROR("Error hashing file",session_path);
	if (count>0) {
	  sha1_write(&s,buffer,count);
	  int wrote=fwrite(buffer,count,1,c);
	  if (wrote!=1) {
	    fclose(in);
	    fclose(c);
	    unlink(temp_path);
	    LOG_ERROR("Failed to write all bytes during survey specification copy",temp_path);
	  }
	}
	if (retVal) break;
      } while(count>0);
            
      fclose(in);
      fclose(c);

      // Rename temporary file
      snprintf(session_path_suffix,1024,"surveys/%s/%s",survey_id,survey_sha1);
      if (generate_path(session_path_suffix,session_path,1024)) LOG_ERROR("generate_path() failed to build path for new session",survey_id);

      if (rename(temp_path,session_path)) LOG_ERROR("Could not rename survey specification copy to name of hash",session_path);
      
    }
    
    
    // Generate new unique session ID
    int tries=0;
    for(tries=0;tries<5;tries++)
    {
      if (random_session_id(session_id)) LOG_ERROR("random_session_id() failed to generate new session_id","");
      for(int i=0;i<4;i++) session_prefix[i]=session_id[i];
      session_prefix[4]=0;

      fprintf(stderr,"session_id='%s'\n",session_id);
    
      snprintf(session_path_suffix,1024,"sessions/%s/%s",session_prefix,session_id);
      if (generate_path(session_path_suffix,session_path,1024)) LOG_ERROR("generate_path() failed to build path for new session",survey_id);
      
      // Check if session already exists
      fprintf(stderr,"Considering session '%s'\n",session_path);

      // Try again if session ID already exists
      if ( access( session_path, F_OK ) != -1 ) { session_id[0]=0; continue; } else break;
    }
    if (!session_id[0]) LOG_ERROR("Failed to generate unique session ID after several tries","");

    // Make directories if they don't already exist
    if (generate_path("sessions",session_path,1024)) LOG_ERROR("generate_path() failed to build path for new session",survey_id);
    mkdir(session_path,0750);
    snprintf(session_path_suffix,1024,"sessions/%s",session_prefix);
    if (generate_path(session_path_suffix,session_path,1024)) LOG_ERROR("generate_path() failed to build path for new session",survey_id);
    mkdir(session_path,0750);

    // Get full filename of session file again
    snprintf(session_path_suffix,1024,"sessions/%s/%s",session_prefix,session_id);
    if (generate_path(session_path_suffix,session_path,1024)) LOG_ERROR("generate_path() failed to build path for new session",survey_id);
    
    FILE *f=fopen(session_path,"w");
    if (!f) LOG_ERROR("Cannot create new session file","");

    // Write survey_id to new empty session.
    // This must take the form <survey id>/<sha1 hash of current version of survey>
    fprintf(f,"%s/%s\n",survey_id,survey_sha1);
    
    fclose(f);
    
  } while(0);

  return retVal;
}

void freez(void *p)
{
  if (p) free(p);
  return;
}

void free_answer(struct answer *a)
{
  if (!a) return;

  freez(a->uid);
  freez(a->text);
  
  free(a);
  return;
}

void free_question(struct question *q)
{
  if (!q) return;
  freez(q->uid);
  freez(q->question_text);
  freez(q->question_html);
  freez(q->default_value);
  freez(q);
  return;
}

void free_session(struct session *s)
{
  if (!s) return;

  freez(s->survey_id);
  freez(s->survey_description);
  freez(s->session_id);
  
  for(int i=0;i<s->question_count;i++) free_question(s->questions[i]);
  for(int i=0;i<s->answer_count;i++) free_answer(s->answers[i]);
  s->answer_count=0;
  s->question_count=0;
  
  free(s);
  return;
}

int load_survey_questions(struct session *ses)
{
  int retVal=0;
  FILE *f=NULL;
  
  do {

    if (!ses) LOG_ERROR("session structure is NULL","");
    if (!ses->survey_id) LOG_ERROR("survey_id in session structure is NULL","");
    
    char survey_path_suffix[1024];
    char survey_path[1024];
    snprintf(survey_path_suffix,1024,"surveys/%s",ses->survey_id);
    if (generate_path(survey_path_suffix,survey_path,1024))
      LOG_ERROR("generate_path() failed to build path for loading session",ses->session_id);

    fprintf(stderr,"Reading questions from '%s'\n",survey_path);
    
    f=fopen(survey_path,"r");
    if (!f) LOG_ERROR("Could not open survey file",survey_path);
    char line[8192];

    // Check survey file format version
    line[0]=0;fgets(line,8192,f);
    if (!line[0]) LOG_ERROR("Failed to read survey file format version in survey specification file",survey_path);
    int format_version=0;
    if (sscanf(line,"version %d",&format_version)!=1)
      LOG_ERROR("Error parsing file format version in survey file",survey_path);
    if (format_version!=1) LOG_ERROR("Unknown survey file format version in survey file",survey_path);

    // Get survey file description
    line[0]=0;fgets(line,8192,f);
    if (!line[0]) LOG_ERROR("Failed to read survey description in survey specification file",survey_path);
    int len=strlen(line);
    // Trim CR and LF chars from description
    while (len&&(line[len-1]=='\n'||line[len-1]=='\r')) line[--len]=0;
    ses->survey_description=strdup(line);
    if (!ses->survey_description) LOG_ERROR("strdup(survey_description) failed when loading survey file",survey_path);

    fprintf(stderr,"Survey description is '%s'\n",ses->survey_description);
    
    fclose(f); f=NULL;    
  } while(0);
  if (f) fclose(f);
  return retVal;
}

struct session *load_session(char *session_id)
{
  int retVal=0;
  struct session *ses=NULL;
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

    // Session file consists of:
    // First line = survey name in form <survey id>/<sha1 hash>
    // (this allows the survey to change without messing up sessions that are
    // in progress).
    // Subesequent lines:
    // <timestamp in seconds since 1970> <add|del> <serialised answer>

    // Read survey ID line
    char survey_id[1024];
    survey_id[0]=0; fgets(survey_id,1024,s);
    if (!survey_id[0]) { fclose(s); LOG_ERROR("Could not read survey ID from session file",session_path); }
    // Trim CR / LF characters from the end
    while (survey_id[0]&&((survey_id[strlen(survey_id)-1]=='\n')
			  ||(survey_id[strlen(survey_id)-1]=='\r'))) survey_id[strlen(survey_id)-1]=0;
    fprintf(stderr,"Survey ID in session file is '%s'\n",survey_id);

    ses=calloc(sizeof(struct session),1);
    if (!ses) { fclose(s); LOG_ERROR("calloc() failed when loading session",session_id); }
    ses->survey_id=strdup(survey_id);
    if (!ses->survey_id) LOG_ERROR("strdup(survey_id) failed when loading session",session_id);
    
    // Load survey
    if (load_survey_questions(ses))
      { fclose(s); LOG_ERROR("Failed to load questions from survey",survey_id); }
    if (!ses->question_count) LOG_ERROR("Failed to load questions from survey, or survey contains no questions",survey_id);
    
    // Load answers from session file
    char line[65536];
    line[0]=0;
    do {
      // Get next line with an answer in it
      line[0]=0; fgets(line,65536,s);
      if (!line[0]) break;
      int len=strlen(line);
      if (!len) LOG_ERROR("Empty line in session file",session_path);
      if (line[len-1]!='\n'&&line[len-1]!='\r') LOG_ERROR("Line too long in session file (limit = 64K)",session_path);      

      // Add answer to list of answers
      if (ses->answer_count>=MAX_QUESTIONS) LOG_ERROR("Too many answers in session file (increase MAX_QUESTIONS?)",session_path);
      ses->answers[ses->answer_count]=calloc(sizeof(struct answer),1);
      if (!ses->answers[ses->answer_count]) LOG_ERROR("calloc() failed while reading session file ",session_path);
      if (deserialise_answer(line,ses->answers[ses->answer_count]))
	LOG_ERROR("Failed to deserialise answer from session file",session_path);
      ses->answer_count++;

    } while(0);
    if (retVal) { fclose(s); if (ses) { free_session(ses); } ses=NULL; break; }
    
    fclose(s);
    
  } while(0);
  if (retVal) ses=NULL;
  return ses;
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
