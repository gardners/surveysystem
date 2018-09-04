
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "errorlog.h"
#include "survey.h"
#include "serialisers.h"

void usage(void)
{
  fprintf(stderr,
	  "usage: survey newsession <survey name> -- create a new session\n"
	  "       survey addanswer <sessionid> <serialised answer> -- add an answer to an existing session\n"
	  "       survey nextquestion <sessionid> -- get the next question that should be asked\n"
	  "       survey delanswer <sessionid> <question id> -- delete an answer from an existing session\n"
	  "       survey delsession <sessionid> -- delete an existing session\n");
};

int main(int argc,char **argv)
{
  int retVal=0;

  do {
    if (argc<2) { usage(); retVal=-1; break; }
    if (!strcmp(argv[1],"newsession")) {
      if (argc!=3) { usage(); retVal=-1; break; }
      char session_id[1024];
      if (create_session(argv[2],session_id)) LOG_ERROR("create_session() failed","");
      if (!retVal) printf("%s\n",session_id);
    } else if (!strcmp(argv[1],"addanswer")) {
      if (argc!=4) { usage(); retVal=-1; break; }
      char *session_id=argv[2];
      char *serialised_answer=argv[3];
      struct session *s=load_session(session_id);
      if (!s) LOG_ERROR("load_session failed",session_id);
      struct answer a;
      bzero(&a,sizeof(struct answer));
      if (deserialise_answer(serialised_answer,&a)) LOG_ERROR("deserialise_answer() failed",serialised_answer);
      if (!s) LOG_ERROR("load_session() failed",session_id);
      if (session_add_answer(s,&a)) LOG_ERROR("session_add_answer() failed","");
      if (save_session(s)) LOG_ERROR("save_session() failed",session_id);
    } else if (!strcmp(argv[1],"nextquestion")) {
      if (argc!=3) { usage(); retVal=-1; break; }
      char *session_id=argv[2];
      struct session *s=load_session(session_id);
      struct question *q[1024];
      int next_question_count=0;
      if (get_next_questions(s->survey_id,session_id,s->questions,s->answers,q,1024,&next_question_count))
	LOG_ERROR("get_next_questions() failed",session_id);

    } else if (!strcmp(argv[1],"delanswer")) {
      if (argc!=3) { usage(); retVal=-1; break; }

    } else if (!strcmp(argv[1],"delsession")) {
      if (argc!=3) { usage(); retVal=-1; break; }

    } else { usage(); retVal=-1; break; }
  } while(0);

  if (retVal) {
    fprintf(stderr,"Command failed:\n");
    dump_errors(stderr);
  }
  return retVal;
}
