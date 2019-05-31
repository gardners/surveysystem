
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
#include <sys/file.h>

#include "survey.h"
#include "serialisers.h"
#include "errorlog.h"
#include "question_types.h"
#include "sha1.h"

/*
  Verify that a session ID does not contain any illegal characters.
  We allow only hex and the dash character.
  The main objective is to disallow colons and slashes, to
  prevent subverting the CSV file format or the formation of file names
  and paths.
*/
int validate_session_id(char *session_id)
{
  int retVal=0;
  do {
    if (!session_id) LOG_ERROR("session_id is NULL");
    if (strlen(session_id)!=36) LOG_ERRORV("session_id '%s' must be exactly 36 characters long",session_id);
    if (session_id[0]=='-') LOG_ERRORV("session_id '%s' may not begin with a dash",session_id);
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
    LOG_ERRORV("session_id '%s' must be lower case",session_id);
    break;
      default:
    LOG_ERRORV("Illegal character 0x%02x in session_id '%s'. Must be a valid UUID",session_id[i],session_id);
    break;
      }
  } while(0);
  return retVal;
}

/*
  Similarly, here we make sure that a survey ID contains no disallowed characters.
  We allow undderscore and space as well as dash and period, to allow some greater
  freedom when specifying the symbolic name of a survey (form).  We naturally also
  allow all upper and lower case latin characters, rather than just hexadecimal
  characters.
*/
int validate_survey_id(char *survey_id)
{
  int retVal=0;
  do {
    if (!survey_id) LOG_ERROR("survey_id is NULL");
    if (!survey_id[0]) LOG_ERROR("survey_id is empty string");
    for(int i=0;survey_id[i];i++)
      switch(survey_id[i]) {
      case ' ': case '.': case '-': case '_':
    break;
      default:
    if (!isalnum(survey_id[i]))
      LOG_ERRORV("Illegal character 0x%02x in survey_id '%s'.  Must be 0-9, a-z, or space, period, comma or underscore",survey_id[i],survey_id);
    break;
      }
  } while(0);
  return retVal;
}

/*
  Get len bytes from the cryptographically secure randomness source.
 */
int urandombytes(unsigned char *buf, size_t len)
{
  int retVal = -1;

  do {
    if (! buf)
      {
    LOG_ERROR("buf is null");
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
        LOG_ERROR("failed to open /dev/urandom, stop retrying");
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
          LOG_ERROR("failed to read from /dev/urandom, even after retries");
          perror("read(/dev/urandom)");
          if (errno==EBADF)
        {
          LOG_ERROR("EBADF on /dev/urandom, resetting urandomfd to -1");
          urandomfd=-1;
        }
          break; // while
        }
      else
        {
          LOG_ERRORV("failed to read from /dev/urandom, retry %d, retrying", tries);
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

/*
  Generate a random (and hopefully unique) session ID.
  Our session IDs are modelled on RFC 4122 UUIDs, but are not exactly
  the same, largely out of convenience of implementation. There is nothing
  stopping us moving to full compliance as time permits.
 */
int random_session_id(char *session_id_out)
{
  int retVal=0;
  do {
    if (!session_id_out) LOG_ERROR("session_id_out is NULL");

    unsigned char randomness[32];
    if (urandombytes(randomness,32)) LOG_ERROR("get_randomness() failed");
    sha1nfo s1;
    sha1_init(&s1);
    sha1_write(&s1,(char *)randomness,32);
    unsigned char *hash=sha1_result(&s1);
    time_t t=time(0);
    unsigned int time_low=t&0xffffffffU;
    unsigned int time_high=(t>>32L);

    // Our session IDs look like RFC 4122 UUIDs, and are mostly the same,
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

/*
  Create a new session for a given form, and return the new session ID
  via session_id_out.  This creates the session record file, as well as
  making sure that the survey exists, and if the exact version of the survey
  has not yet been recorded persistently, it makes a copy of it named as the
  SHA1 hash of the survey definition.  This allows the survey definition in
  surveys/survey_id/current to be freely modified, and any existing sessions
  will continue to use the version of the survey that they were created under,
  unless you modify the session record file to point to the new session.

  (If you have added or deleted questions, that might cause complicated problems,
  so we don't do that automatically. As time allows, we can create a session
  upgrade function, that checks if the current version of the form is logically
  equivalent (same set of question UIDs and question types for all questions that
  have been answered), and if so, updates the session to use the latest version.
  This could even be implemented in load_session().)
 */
int create_session(char *survey_id,char *session_id_out)
{
  int retVal=0;

  do {
    char session_id[256];
    char session_prefix[5];
    char session_path_suffix[1024];
    char session_path[1024];

    if (!survey_id) LOG_ERROR("survey_id is NULL");
    if (!session_id_out) LOG_ERROR("session_id_out is NULL");

    if (validate_survey_id(survey_id)) LOG_ERROR("Invalid survey ID");

    snprintf(session_path_suffix,1024,"surveys/%s/current",survey_id);
    if (generate_path(session_path_suffix,session_path,1024)) LOG_ERRORV("generate_path('%s') failed to build path for new session for survey '%s'",session_path_suffix,survey_id);
    if ( access( session_path, F_OK ) == -1 ) LOG_ERRORV("Survey '%s' does not exist",survey_id);
    // Get sha1 hash of survey file
    char survey_sha1[1024];
    if (sha1_file(session_path,survey_sha1)) LOG_ERRORV("Could not hash survey specification file '%s'",session_path);

    snprintf(session_path_suffix,1024,"surveys/%s/%s",survey_id,survey_sha1);
    if (generate_path(session_path_suffix,session_path,1024)) LOG_ERRORV("generate_path('%s') failed to build path for new session for survey '%s'",session_path_suffix,survey_id);
    if ( access( session_path, F_OK ) == -1 ) {
      // No copy of the survey exists with the hash, so make one.
      // To avoid a race condition where the current survey form could be modified,
      // we copy it, and hash it as we go, and rename the copy based on the hash value
      // of what we read.

      // Open input file
      snprintf(session_path_suffix,1024,"surveys/%s/current",survey_id);
      if (generate_path(session_path_suffix,session_path,1024)) LOG_ERRORV("generate_path('%s') failed to build path for new session for survey '%s'",session_path_suffix,survey_id);
      FILE *in=fopen(session_path,"r");
      if (!in) LOG_ERRORV("Could not read from survey specification file '%s' for survey '%s'",session_path,survey_id);

      // So make a hopefully unique name for the temporary file
      char temp_path[1024];
      snprintf(session_path_suffix,1024,"surveys/%s/temp.%lld.%d.%s",survey_id,(long long)time(0),getpid(),survey_sha1);
      if (generate_path(session_path_suffix,temp_path,1024))
    {
      fclose(in);
      LOG_ERRORV("generate_path('%s') failed to build path for new session for survey '%s'",session_path_suffix,survey_id);
    }
      FILE *c=fopen(temp_path,"w");
      if (!c) {
    fclose(in);
    LOG_ERRORV("Could not create temporary file '%s'",temp_path);
      }

      char buffer[8192];
      int count;
      sha1nfo s;
      sha1_init(&s);

      do {
    count=fread(buffer,1,8192,in);
    if (count<0) LOG_ERRORV("Error hashing file '%s'",session_path);
    if (count>0) {
      sha1_write(&s,buffer,count);
      int wrote=fwrite(buffer,count,1,c);
      if (wrote!=1) {
        fclose(in);
        fclose(c);
        unlink(temp_path);
        LOG_ERRORV("Failed to write all bytes during survey specification copy into '%s'",temp_path);
      }
    }
    if (retVal) break;
      } while(count>0);

      fclose(in);
      fclose(c);

      // Rename temporary file
      snprintf(session_path_suffix,1024,"surveys/%s/%s",survey_id,survey_sha1);
      if (generate_path(session_path_suffix,session_path,1024)) LOG_ERRORV("generate_path('%s') failed to build path for new session for survey '%s'",session_path_suffix,survey_id);

      if (rename(temp_path,session_path)) LOG_ERRORV("Could not rename survey specification copy to name of hash from '%s' to '%s'",temp_path,session_path);
      LOG_INFOV("Created new hashed survey specification file '%s' for survey '%s'",session_path,survey_id);
    }


    // Generate new unique session ID
    int tries=0;
    for(tries=0;tries<5;tries++)
    {
      if (random_session_id(session_id)) LOG_ERROR("random_session_id() failed to generate new session_id");
      for(int i=0;i<4;i++) session_prefix[i]=session_id[i];
      session_prefix[4]=0;

      //fprintf(stderr,"session_id='%s'\n",session_id);

      snprintf(session_path_suffix,1024,"sessions/%s/%s",session_prefix,session_id);
      if (generate_path(session_path_suffix,session_path,1024)) LOG_ERRORV("generate_path('%s') failed to build path for new session for survey '%s'",session_path_suffix,survey_id);

      // Check if session already exists
      // fprintf(stderr,"Considering session '%s'\n",session_path);

      // Try again if session ID already exists
      if ( access( session_path, F_OK ) != -1 ) { session_id[0]=0; continue; } else break;
    }
    if (!session_id[0]) LOG_ERROR("Failed to generate unique session ID after several tries");

    // Make directories if they don't already exist
    if (generate_path("sessions",session_path,1024)) LOG_ERRORV("generate_path('%s') failed to build path for new session in survey '%s'","sessions",survey_id);
    mkdir(session_path,0750);
    snprintf(session_path_suffix,1024,"sessions/%s",session_prefix);
    if (generate_path(session_path_suffix,session_path,1024)) LOG_ERRORV("generate_path('%s') failed to build path for new session in survey '%s'",session_path_suffix,survey_id);
    mkdir(session_path,0750);

    // Get full filename of session file again
    snprintf(session_path_suffix,1024,"sessions/%s/%s",session_prefix,session_id);
    if (generate_path(session_path_suffix,session_path,1024)) LOG_ERRORV("generate_path('%s') failed to build path for new session of survey '%s'",session_path_suffix,survey_id);

    FILE *f=fopen(session_path,"w");
    if (!f) LOG_ERRORV("Cannot create new session file '%s'",session_path);

    // Write survey_id to new empty session.
    // This must take the form <survey id>/<sha1 hash of current version of survey>
    fprintf(f,"%s/%s\n",survey_id,survey_sha1);

    fclose(f);

    // Export new session ID
    strncpy(session_id_out,session_id,36+1);

    LOG_INFOV("Created new session file '%s' for survey '%s'",session_path,survey_id);

  } while(0);

  return retVal;
}

/*
   The opposite of create_session().  It will complain if the session does not exist,
   or cannot be deleted.
*/
int delete_session(char *session_id)
{
  int retVal=0;

  do {
    char session_prefix[5];
    char session_path_suffix[1024];
    char session_path[1024];

    if (!session_id) LOG_ERROR("session_id is NULL");

    if (validate_session_id(session_id)) LOG_ERRORV("Session ID '%s' is malformed",session_id);

    for(int i=0;i<4;i++) session_prefix[i]=session_id[i];
    session_prefix[4]=0;

    snprintf(session_path_suffix,1024,"sessions/%s/%s",session_prefix,session_id);
    if (generate_path(session_path_suffix,session_path,1024)) LOG_ERRORV("generate_path('%s') failed to build path while deleting session '%s'",session_path_suffix,session_id);

    // Try again if session ID already exists
    if ( access( session_path, F_OK ) == -1 ) LOG_ERRORV("Session file '%s' does not exist",session_path);

    if (unlink(session_path)) LOG_ERRORV("unlink('%s') failed",session_path);

    LOG_INFOV("Deleted session '%s'.",session_path);

  } while(0);

  return retVal;
}

/*
  Various functions for freeing data structures.
  freez() is the same as free(), but just checks to make sure that it hasn't been
  passed a null pointer.  This makes the latter functions a little simpler.
 */
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
  // #72 unit field
  freez(a->unit);

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
  freez(q->choices);
  // #72 unit field
  freez(q->unit);
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

/*
  Remove any trailing line feed or carriage returns from the input string.
 */
void trim_crlf(char *line)
{
  if (!line) return;
  int len=strlen(line);
  while (len&&(line[len-1]=='\n'||line[len-1]=='\r')) line[--len]=0;
  return;
}

/*
  Load and deserialise the set of questions for the form corresponding to
  this session.
 */
int load_survey_questions(struct session *ses)
{
  int retVal=0;
  FILE *f=NULL;

  do {

    if (!ses) LOG_ERROR("session structure is NULL");
    if (!ses->survey_id) LOG_ERROR("survey_id in session structure is NULL");

    char survey_path_suffix[1024];
    char survey_path[1024];
    snprintf(survey_path_suffix,1024,"surveys/%s",ses->survey_id);
    if (generate_path(survey_path_suffix,survey_path,1024))
      LOG_ERRORV("generate_path('%s') failed to build path for loading session '%s'",survey_path_suffix,ses->session_id);

    f=fopen(survey_path,"r");
    if (!f) LOG_ERRORV("Could not open survey file '%s'",survey_path);
    char line[8192];

    // Check survey file format version
    line[0]=0;fgets(line,8192,f);
    if (!line[0]) LOG_ERRORV("Failed to read survey file format version in survey specification file '%s'",survey_path);
    int format_version=0;
    int offset=0;
    trim_crlf(line);
    if (sscanf(line,"version %d%n",&format_version,&offset)!=1)
      LOG_ERRORV("Error parsing file format version in survey file '%s'",survey_path);
    if (offset<strlen(line)) LOG_ERRORV("Junk at end of version string in survey file '%s'. Line was '%s'",survey_path,line);
    if (format_version<1||format_version>2) LOG_ERRORV("Unknown survey file format version in survey file '%s'",survey_path);

    // Get survey file description
    line[0]=0;fgets(line,8192,f);
    if (!line[0]) LOG_ERRORV("Failed to read survey description in survey specification file '%s'",survey_path);
    // Trim CR and LF chars from description
    trim_crlf(line);
    ses->survey_description=strdup(line);
    if (!ses->survey_description) LOG_ERRORV("strdup('%s') failed when loading survey file '%s'",line,survey_path);

    // Only allow generic implementation of next question picker if explicitly allowed
    ses->allow_generic=0;
    if (format_version>1) {
      // Check for pythondir and without python directives
      line[0]=0;fgets(line,8192,f);
      if (!line[0]) LOG_ERRORV("Failed to read survey description in survey specification file '%s'",survey_path);
      // Trim CR and LF chars from description
      trim_crlf(line);
      if (!strcasecmp(line,"without python")) {
        ses->allow_generic=1;
        ses->pythondir[0]=0;
      } else if (sscanf(line,"pythondir=%[^\n]",ses->pythondir)==1) {
	 // We are using python, and have recorded a python library directory to add to the search path.
      } else {
	LOG_ERRORV("Missing <without python|pythondir=...> directive in survey specification file '%s'",survey_path);
      }
      
    }

    // Now read questions
    do {
      line[0]=0;fgets(line,8192,f);
      if (!line[0]) break;

      if (ses->question_count>=MAX_QUESTIONS) LOG_ERRORV("Too many questions in survey '%s' (increase MAX_QUESTIONS?)",survey_path);

      struct question *q=calloc(sizeof(struct question),1);
      if (!q) LOG_ERRORV("calloc(struct question) failed while loading survey question list from '%s'",survey_path);

      // Remove end of line markers
      trim_crlf(line);

      // Skip blank lines
      if (!line[0]) continue;

      if (deserialise_question(line,q)) { free_question(q); q=NULL; LOG_ERRORV("Error deserialising question '%s' in survey file '%s'",line,survey_path); }
      ses->questions[ses->question_count++]=q;

    } while(line[0]);

    fclose(f); f=NULL;
  } while(0);
  if (f) fclose(f);
  return retVal;
}

/*
  Load the specified session, and return the corresponding session structure.
  This will load not only the answers, but also the full set of questions.
 */
struct session *load_session(char *session_id)
{
  int retVal=0;
  struct session *ses=NULL;
  do {
    if (!session_id) LOG_ERROR("session_id is NULL");

    if (validate_session_id(session_id)) LOG_ERRORV("validate_session_id('%s') failed",session_id);

    char session_path[1024];
    char session_path_suffix[1024];
    char session_prefix[5];
    for(int i=0;i<4;i++) session_prefix[i]=session_id[i];
    session_prefix[4]=0;

    snprintf(session_path_suffix,1024,"sessions/%s/%s",session_prefix,session_id);
    if (generate_path(session_path_suffix,session_path,1024))
      LOG_ERRORV("generate_path('%s') failed to build path for loading session '%s'",session_path_suffix,session_id);

    FILE *s=fopen(session_path,"r");
    if (!s) LOG_ERRORV("Could not read from session file '%s'",session_path);

    // Session file consists of:
    // First line = survey name in form <survey id>/<sha1 hash>
    // (this allows the survey to change without messing up sessions that are
    // in progress).
    // Subesequent lines:
    // <timestamp in seconds since 1970> <add|del> <serialised answer>

    // Read survey ID line
    char survey_id[1024];
    survey_id[0]=0; fgets(survey_id,1024,s);
    if (!survey_id[0]) { fclose(s); LOG_ERRORV("Could not read survey ID from session file '%s'",session_path); }
    // Trim CR / LF characters from the end
    trim_crlf(survey_id);

    ses=calloc(sizeof(struct session),1);
    if (!ses) { fclose(s); LOG_ERRORV("calloc(%d,1) failed when loading session '%s'",sizeof(struct session),session_id); }
    ses->survey_id=strdup(survey_id);
    ses->session_id=strdup(session_id);
    if (!ses->survey_id) LOG_ERRORV("strdup(survey_id='%s') failed when loading session '%s'",survey_id,session_id);
    if (!ses->session_id) LOG_ERRORV("strdup(session_id) failed when loading session '%s'",session_id);

    // Load survey
    if (load_survey_questions(ses))
      { fclose(s); LOG_ERRORV("Failed to load questions from survey '%s'",survey_id); }
    if (!ses->question_count) LOG_ERRORV("Failed to load questions from survey '%s', or survey contains no questions",survey_id);

    // Load answers from session file
    char line[65536];
    line[0]=0;
    do {
      // Get next line with an answer in it
      line[0]=0; fgets(line,65536,s);
      if (!line[0]) break;
      int len=strlen(line);
      if (!len) LOG_ERRORV("Empty line in session file '%s'",session_path);
      if (line[len-1]!='\n'&&line[len-1]!='\r') LOG_ERRORV("Line too long in session file '%s' (limit = 64K)",session_path);
      trim_crlf(line);

      // Add answer to list of answers
      if (ses->answer_count>=MAX_QUESTIONS) LOG_ERRORV("Too many answers in session file '%s' (increase MAX_QUESTIONS?)",session_path);
      ses->answers[ses->answer_count]=calloc(sizeof(struct answer),1);
      if (!ses->answers[ses->answer_count]) LOG_ERRORV("calloc(%d,1) failed while reading session file '%s' ",sizeof(struct answer),session_path);
      if (deserialise_answer(line,ses->answers[ses->answer_count]))
    LOG_ERRORV("Failed to deserialise answer '%s' from session file '%s'",line,session_path);
      ses->answer_count++;

    } while(line[0]);
    if (retVal) { fclose(s); if (ses) { free_session(ses); } ses=NULL; break; }

    fclose(s);

  } while(0);
  if (retVal) ses=NULL;
  return ses;
}

/*
  Save the provided session, including all provided answers.
  Questions are not saved, as they are part of the survey, i.e., form specification.
 */
int save_session(struct session *s)
{
  int retVal=0;
  FILE *o=NULL;
  do {
    if (!s) LOG_ERROR("session structure is NULL");
    if (!s->session_id) LOG_ERROR("s->session_id is NULL");

    if (validate_session_id(s->session_id)) LOG_ERRORV("validate_session_id('%s') failed",s->session_id);

    char session_path[1024];
    char session_path_final[1024];
    char session_path_suffix[1024];
    char session_prefix[5];
    for(int i=0;i<4;i++) session_prefix[i]=s->session_id[i];
    session_prefix[4]=0;
    
    snprintf(session_path_suffix,1024,"sessions/%s/write.%s",session_prefix,s->session_id);
    if (generate_path(session_path_suffix,session_path,1024))
      LOG_ERRORV("generate_path('%s') failed to build path for loading session '%s'",session_path_suffix,s->session_id);

    o=fopen(session_path,"w");
    if (!o) LOG_ERRORV("Could not create or open session file '%s' for write",session_path);

    fprintf(o,"%s\n",s->survey_id);
    for(int i=0;i<s->answer_count;i++) {
      char line[65536];
      if (serialise_answer(s->answers[i],line,65536))
    LOG_ERRORV("Could not serialise answer for question '%s' for session '%s'.  Text field too long?",s->answers[i]->uid,s->session_id);
      fprintf(o,"%s\n",line);
    }
    fclose(o); o=NULL;

    snprintf(session_path_suffix,1024,"sessions/%s/%s",session_prefix,s->session_id);
    if (generate_path(session_path_suffix,session_path_final,1024))
      LOG_ERRORV("generate_path('%s') failed to build path for loading session '%s'",session_path_suffix,s->session_id);
    if (rename(session_path,session_path_final))
      LOG_ERRORV("rename('%s','%s') failed when updating file for session '%s' (errno=%d)",
		 session_path,session_path_final,s->session_id,errno);
    
    LOG_INFOV("Updated session file '%s'.",session_path_final);


  }  while(0);
  if (o) fclose(o);
  return retVal;
}

struct answer *copy_answer(struct answer *aa)
{
  int retVal=0;
  struct answer *a=NULL;
  do {
    // Duplicate aa into a, so that we don't put pointers to structures that are on the
    // stack into our list.
    a=malloc(sizeof(struct answer));
    if (!a) LOG_ERROR("malloc() of struct answer failed.");
    bcopy(aa,a,sizeof(struct answer));
    if (a->uid) { a->uid=strdup(a->uid); if (!a->uid) { LOG_ERROR("Could not copy a->uid"); } }
    if (a->text) { a->text=strdup(a->text);if (!a->text) { LOG_ERROR("Could not copy a->text"); } }
    if (a->unit) { a->unit=strdup(a->unit); if (!a->unit) { LOG_ERROR("Could not copy a->unit"); } }
  } while(0);
  if (retVal) return NULL;
  else return a;
}

/*
  Add the provided answer to the set of answers in the provided session.
  If another answer exists for the same question, it will trigger an error.
 */
int session_add_answer(struct session *ses,struct answer *a)
{
  int retVal=0;
  int undeleted=0;
  do {
    // Add answer to list of answers
    if (!ses) LOG_ERROR("Session structure is NULL");
    if (!a) LOG_ERROR("Asked to add null answer to session");

    // Don't allow answers to questions that don't exist
    int question_number=0;
    for(question_number=0;question_number<ses->question_count;
    question_number++)
      if (!strcmp(ses->questions[question_number]->uid,
          a->uid)) break;
    if (question_number==ses->question_count)
      LOG_ERRORV("There is no such question '%s'",a->uid);

    // Don't allow multiple answers to the same question
    for(int i=0;i<ses->answer_count;i++)
      if (!strcmp(ses->answers[i]->uid,a->uid)) {
	if (ses->answers[i]->flags&ANSWER_DELETED) {
	  // Answer exists, but was deleted, so we can just update the values
	  // by replacing the answer structure. #186
	  free(ses->answers[i]);
	  ses->answers[i]=copy_answer(a);
	  undeleted=1;
	} else
	  {
	    LOG_ERRORV("Question '%s' has already been answered in session '%s'. Delete old answer before adding a new one",a->uid,ses->session_id);
	  }
	
	
      }
    if (retVal) break;

    if (ses->answer_count>=MAX_QUESTIONS) LOG_ERRORV("Too many answers in session '%s' (increase MAX_QUESTIONS?)",ses->session_id);

    // #186 Don't append answer if we are undeleting it.
    if (!undeleted) {
      ses->answers[ses->answer_count]=copy_answer(a);
      ses->answer_count++;
    }

    char serialised_answer[65536]="(could not serialise)";
    serialise_answer(a,serialised_answer,65536);
    LOG_INFOV("Added to session '%s' answer '%s'.",ses->session_id,serialised_answer);

  } while(0);
  return retVal;
}

/*
  Delete any and all answers to a given question from the provided session structure.
  It is not an error if there were no matching answers to delete
*/
int session_delete_answers_by_question_uid(struct session *ses,char *uid, int deleteFollowingP)
{
  int retVal=0;
  int deletions=0;
  do {
    if (!ses) LOG_ERROR("Session structure is NULL");
    if (!uid) LOG_ERRORV("Asked to remove answers to null question UID from session '%s'",ses->session_id);

    for(int i=0;i<ses->answer_count;i++)
      while ((i<ses->answer_count)&&(!strcmp(ses->answers[i]->uid,uid)))
    {
      // Delete matching questions
      // #186 - Deletion now just sets the ANSWER_DELETED flag in the flags field for the answer.
      // If deleteFollowingP is non-zero, then this is applied to all later answers in the session also.

      char serialised_answer[65536]="(could not serialise)";
      serialise_answer(ses->answers[i],serialised_answer,65536);
      LOG_INFOV("Deleted from session '%s' answer '%s'.",ses->session_id,serialised_answer);

      ses->answers[i]->flags |= ANSWER_DELETED;

      // Mark all following answers deleted, if required
      if (deleteFollowingP) {
	for(int j=i+1;j<ses->answer_count;j++)
	  ses->answers[j]->flags |= ANSWER_DELETED;	  
      }
    }
    if (retVal) break;

    if (!retVal) retVal=deletions;
  } while(0);
  return retVal;
}

/*
  Delete exactly the provided answer from the provided session,
  and return the number of answers deleted.
  If there is no exactly matching answer, it will return 0.
 */
int session_delete_answer(struct session *ses,struct answer *a, int deleteFollowingP)
{
  int retVal=0;
  int deletions=0;
  do {
    if (!ses) LOG_ERROR("Session structure is NULL");
    if (!a) LOG_ERRORV("Asked to remove null answer from session '%s'",ses->session_id?ses->session_id:"(null)");

    for(int i=0;i<ses->answer_count;i++)
      while ((i<ses->answer_count)&&(!compare_answers(a,ses->answers[i],MISMATCH_IS_NOT_AN_ERROR)))
    {
      // Delete matching questions
      // (Actually just mark them deleted. #186)

      char serialised_answer[65536]="(could not serialise)";
      serialise_answer(ses->answers[i],serialised_answer,65536);
      LOG_INFOV("Deleted from session '%s' answer '%s'.",ses->session_id,serialised_answer);

      ses->answers[i]->flags |= ANSWER_DELETED;

      // Mark all following answers deleted, if required
      if (deleteFollowingP) {
	for(int j=i+1;j<ses->answer_count;j++)
	  ses->answers[j]->flags |= ANSWER_DELETED;	  
      }
    }
    if (retVal) break;

    if (!retVal) retVal=deletions;
  } while(0);
  return retVal;
}
