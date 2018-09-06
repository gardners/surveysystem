
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "survey.h"
#include "errorlog.h"
#include "question_types.h"

#include <Python.h>

#define REPORT_IF_FAILED() { if (retVal) fprintf(stderr,"%s:%d: %s() failed.\n",__FILE__,__LINE__,__FUNCTION__); }

#define MAX_WLEN 65536
wchar_t as_wchar_out[MAX_WLEN];
wchar_t *as_wchar(const char *s)
{
  int retVal=0;
  do {
    if (strlen(s)>=(MAX_WLEN-1)) LOG_ERROR("String too long",s);
    size_t result=mbstowcs(as_wchar_out,s,MAX_WLEN-1);
    if (result<0) LOG_ERROR("mbstowcs() failed",s);
  } while (0);
  if (retVal) return NULL;
  else return as_wchar_out;
}

int is_python_started=0;
int setup_python(void)
{
  int retVal=0;
  do {
    if (is_python_started) break;

    wchar_t *name=as_wchar("nextquestion");
    if (!name) LOG_ERROR("Could not convert string to wchar_t *","nextquestion");

    Py_SetProgramName(name);
    Py_Initialize();
    
    is_python_started=1;
  } while(0);
  return retVal;
}

int end_python(void)
{
  int retVal=0;
  do {
    if (!is_python_started) break;

    Py_Finalize();
  } while(0);
  return retVal;
}

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
  return get_next_questions_generic(surveyname,session_uuid,q,a,next_questions,max_next_questions,next_question_count);
}
