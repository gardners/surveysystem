
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "errorlog.h"
#include "survey.h"
#include "serialisers.h"

void usage(void)
{
  fprintf(stderr,
	  "usage: surveyfcgi -- Start fast CGI service\n");
};

int main(int argc,char **argv)
{
  int retVal=0;

  do {
    if (argc>1) { usage(); retVal=-1; break; }

    if (!getenv("SURVEY_HOME")) {
      fprintf(stderr,"SURVEY_HOME environment variable must be set to data directory for survey system.\n");
      usage();
      retVal=-1;
      break;
    }
      
  } while(0);

  if (retVal) {
    fprintf(stderr,"Survey FASTCGI service failed:\n");
    dump_errors(stderr);
  }
  return retVal;
}
