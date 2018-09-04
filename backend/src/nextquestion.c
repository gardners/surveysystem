
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "survey.h"
#include "errorlog.h"
#include "question_types.h"

#define REPORT_IF_FAILED() { if (retVal) fprintf(stderr,"%s:%d: %s() failed.\n",__FILE__,__LINE__,__FUNCTION__); }

/*
  Generic next question selector, which selects the first question lacking an answer.
  
*/
int get_next_questions_generic(char *surveyname,char *session_uuid,struct question *q[],struct answer *a[],
			       struct question *next_questions[],int max_next_questions,int *next_question_count)
{
  int retVal=0;
  
  do {
    int i,j;

  if (!surveyname) LOG_ERROR("surveyname is NULL","");
  if (!session_uuid) LOG_ERROR("session_uuid is NULL","");
  if (!q) LOG_ERROR("Question list is null","");
  if (!a) LOG_ERROR("Previous answer list is null","");
  if (!next_questions) LOG_ERROR("next_questions is NULL","");
  if (max_next_questions<1) LOG_ERROR("max_next_questions < 1","");
  if (!next_question_count) LOG_ERROR("next_question_count is NULL","");

  // Check each question to see if it has been answered already
  for(i=0;q[i];i++)
    {
      for(j=0;a[j];a++)
	if (!strcmp(a[j]->uid,q[i]->uid)) break;
      if (a[j]) break;
      else {
	if ((*next_question_count)<max_next_questions) {
	  next_questions[*next_question_count]=q[i];
	  (*next_question_count)++;
	}
      }
    }
  
  } while(0);

  return retVal;
  
}

int get_next_questions(char *surveyname,char *session_uuid,struct question *q[],struct answer *a[],
		       struct question *next_questions[],int max_next_questions,int *next_question_count)
{
  // Work out which function to call to get the next question(s) to ask.
  // XXX - For now, just call the generic version
  return get_next_questions(surveyname,session_uuid,q,a,next_questions,max_next_questions,next_question_count);
}
