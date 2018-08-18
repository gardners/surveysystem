
#include <stdio.h>
#include <strings.h>

#include "survey.h"
#include "serialisers.h"

struct question_serialiser_test {
  char *name;
  int shouldPassP;
#define DIRECTION_SERIALISE 1
#define DIRECTION_DESERIALISE 2
  int direction;
  char *serialised;
  struct question *question;
};

struct question_serialiser_test qst[]={

  {"Empty string not accepted",0,DIRECTION_DESERIALISE,"",NULL},
  {NULL,-1,-1,NULL,NULL}
};

int main(int argc,char **argv)
{
  int retVal=0;
  
  do {

    int fail=0;
    int pass=0;
    int errors=0;
    
    for(int i=0;qst[i].name;i++) {
      fprintf(stderr,"[     ] %s",qst[i].name); fflush(stderr);
      if (qst[i].direction&DIRECTION_SERIALISE) {
	// XXX Implement
      }
      if (qst[i].direction&DIRECTION_DESERIALISE) {
	struct question d;
	bzero(&d,sizeof(struct question));
	int deserialise_result=deserialise_question(qst[i].serialised,&d);
	if (deserialise_result&&qst[i].shouldPassP) {
	  // Deserialisation failed when it should have succeeded.
	  fprintf(stderr,"\r[FAIL \n  FAIL: serialised string triggered an error during deserialisate\n");
	  fail++;
	}
	else if ((!deserialise_result)&&(!qst[i].shouldPassP)) {
	  // Deserialiation passed when it should have failed.
	  fprintf(stderr,"\r[FAIL \n  FAIL: invalid serialised string did not trigger an error during deserialisation\n");
	  fail++;
	}
	else if ((!deserialise_result)&&qst[i].shouldPassP) {
	  // Deserialised successfully, so make sure the field values
	  // all match
	  fprintf(stderr,"\r[PASS \n");
	  pass++;
	}
	else if ((deserialise_result)&&(!qst[i].shouldPassP)) {
	  fprintf(stderr,"\r[PASS \n");
	  pass++;
	} else {
	  fprintf(stderr,"\r[ERROR\n  ERROR: deserialisation failed unexpectedly: deserialise_result=%d, shouldPass=%d\n",
		  deserialise_result,qst[i].shouldPassP);
	  errors++;
	}
      }
    }

    fprintf(stderr,"Summary: %d tests passed, %d failed and %d encountered internal errors.\n",
	    pass,fail,errors);
    
  } while (0);
  
  return retVal;
}
