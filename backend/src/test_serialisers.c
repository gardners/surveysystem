
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>

#include "survey.h"
#include "errorlog.h"
#include "serialisers.h"

struct question_serialiser_test {
  char *name;
  int shouldPassP;
#define DIRECTION_SERIALISE 1
#define DIRECTION_DESERIALISE 2
  int direction;
  char *serialised;
  struct question question;
};

#define SHOULD_FAIL 0
#define SHOULD_PASS 1

struct question_serialiser_test qst[]={

  // An empty string should not be accepted
  {"Empty string not accepted",SHOULD_FAIL,DIRECTION_DESERIALISE,"",{NULL}},

  // A simple valid record should be accepted.
  {"Simple serialised question",SHOULD_PASS,DIRECTION_SERIALISE|DIRECTION_DESERIALISE,
   "dummyuid:"
   "What is the answer to life, the universe and everything?:"
   "<div>What is the answer to life, the universe and everything?</div>:"
   "INT:0:42:0:100:0:1",
   {"dummyuid",
    "What is the answer to life, the universe and evrything?",
    "<div>What is the answer to life, the universe and evrything?</div>",
    QTYPE_INT,0,"42",0,100,0,-1}},

  // Only valid question types should be accepted
  {"Illegal question type fails",SHOULD_FAIL,DIRECTION_DESERIALISE,
   "dummyuid:"
   "What is the answer to life, the universe and everything?:"
   "<div>What is the answer to life, the universe and everything?</div>:"
   "FISH:0:42:0:100:0:-1",
   {"dummyuid",
    "What is the answer to life, the universe and evrything?",
    "<div>What is the answer to life, the universe and evrything?</div>",
    QTYPE_INT,0,"42",0,100,0,-1}},

  {"Negative numbers are accepted",SHOULD_PASS,DIRECTION_DESERIALISE|DIRECTION_SERIALISE,
   "dummyuid:"
   "What is the answer to life, the universe and everything?:"
   "<div>What is the answer to life, the universe and everything?</div>:"
   "INT:0:42:0:-100:0:-1",
   {"dummyuid",
    "What is the answer to life, the universe and evrything?",
    "<div>What is the answer to life, the universe and evrything?</div>",
    QTYPE_FIXEDPOINT,0,"42",0,-100,0,-1}},

  {"Minus sign can appear only at beginning of a number",SHOULD_FAIL,DIRECTION_DESERIALISE,
   "dummyuid:"
   "What is the answer to life, the universe and everything?:"
   "<div>What is the answer to life, the universe and everything?</div>:"
   "INT:0:42:0:10-0:0:-1",
   {"dummyuid",
    "What is the answer to life, the universe and evrything?",
    "<div>What is the answer to life, the universe and evrything?</div>",
    QTYPE_FIXEDPOINT,0,"42",0,100,0,-1}},
  
  
  {NULL,-1,-1,NULL,{NULL}}
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

      clear_errors();
      
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
	    fprintf(stderr,"Internal error log:\n");
	  dump_errors(stderr);
	  fail++;
	}
	else if ((!deserialise_result)&&(!qst[i].shouldPassP)) {
	  // Deserialiation passed when it should have failed.
	  fprintf(stderr,"\r[FAIL \n  FAIL: invalid serialised string did not trigger an error during deserialisation\n");
	    fprintf(stderr,"Internal error log:\n");
	  dump_errors(stderr);
	  fail++;
	}
	else if ((!deserialise_result)&&qst[i].shouldPassP) {
	  // Deserialised successfully, so make sure the field values
	  // all match
	  if (compare_questions(&d,&qst[i].question)) {
	    fprintf(stderr,"\r[FAIL \n  FAIL: Original and serialised-then-deserialised question structures differ\n");
	    dump_question(stderr,"Deserialised result",&d);
	    dump_question(stderr,"Expected result",&qst[i].question);
	    fprintf(stderr,"Internal error log:\n");
	    dump_errors(stderr);
	    fail++;
	  } else {
	    fprintf(stderr,"\r[PASS \n");
	    pass++;
	  }
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
