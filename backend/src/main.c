
#include <stdio.h>
#include <string.h>
#include <strings.h>

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
    } else if (!strcmp(argv[1],"addanswer")) {
      if (argc!=4) { usage(); retVal=-1; break; }
    } else if (!strcmp(argv[1],"nextquestion")) {
      if (argc!=4) { usage(); retVal=-1; break; }
    } else if (!strcmp(argv[1],"delanswer")) {
      if (argc!=4) { usage(); retVal=-1; break; }
    } else if (!strcmp(argv[1],"delsession")) {
      if (argc!=4) { usage(); retVal=-1; break; }
    } else { usage(); retVal=-1; break; }
  } while(0);
  
  return retVal;
}
