
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "survey.h"
#include "errorlog.h"
#include "question_types.h"

#include <Python.h>

#define REPORT_IF_FAILED() { if (retVal) fprintf(stderr,"%s:%d: %s() failed.\n",__FILE__,__LINE__,__FUNCTION__); }
#define CHECKPOINT    fprintf(stderr,"%s:%d:%s() CHECKPOINT\n",__FILE__,__LINE__,__FUNCTION__)

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
PyObject *nq_python_module=NULL;
int setup_python(void)
{
  int retVal=0;
  do {

    // Check if the python source has changed. If so, destroy the current python
    // instance, and start again.
    
    if (is_python_started) break;

    if (!getenv("SURVEY_HOME")) LOG_ERROR("SURVEY_HOME environment variable not set","");
    
    wchar_t *name=as_wchar("nextquestion");
    if (!name) LOG_ERROR("Could not convert string to wchar_t *","nextquestion");

    Py_SetProgramName(name);
    Py_Initialize();

    // Add python directory to python's search path
    char append_cmd[1024];
    snprintf(append_cmd,1024,
	     "import sys\n"
	     "sys.path.append('%s/python')\n", getenv("SURVEY_HOME"));
    PyRun_SimpleString(append_cmd);
		      
    nq_python_module = PyImport_ImportModule("nextquestion");

    if (!nq_python_module) {
      PyErr_Print();
      
      LOG_ERROR("Failed to load python module","nextquestion");
    }

    Py_INCREF(nq_python_module);
    
    is_python_started=1;
  } while(0);
  return retVal;
}

int end_python(void)
{
  int retVal=0;
  do {
    if (!is_python_started) break;

    fprintf(stderr,"STOPPING python\n");

    // Free any python objects we have hanging about
    if (nq_python_module) Py_DECREF(nq_python_module); nq_python_module = NULL;
    
    Py_Finalize();
  } while(0);
  return retVal;
}

int call_python_nextquestion(struct session *s,
			     struct question *next_questions[],int max_next_questions,int *next_question_count)
{
  int retVal=0;
  do {

    // Setup python
    if (setup_python()) {
      fprintf(stderr,"Failed to initialise python:\n");
      dump_errors(stderr); retVal=-1; break;
    }
    if (!nq_python_module) LOG_ERROR("Python module not loaded. Does it have an error?","nextquestion");

    fprintf(stderr,"%s:%d:module: ",__FILE__,__LINE__); PyObject_Print(nq_python_module,stderr,0); fprintf(stderr,"\n");
        
    // Build names of candidate functions.
    // nextquestion_<survey_id>_<hash of survey>
    // or failing that, nextquestion_<survey_id>
    // or failing that, nextquestion
    // In all cases, we pass in an list of questions, and an list of answers,
    // and expect a list of strings of question UIDs to ask as the return value.

    CHECKPOINT;
      
    char function_name[1024];

    // Try all three possible function names
    snprintf(function_name,1024,"nextquestion_%s",s->survey_id);
    for(int i=0;function_name[i];i++) if (function_name[i]=='/') function_name[i]='_';

    fprintf(stderr,"%s:%d:module: ",__FILE__,__LINE__); PyObject_Print(nq_python_module,stderr,0); fprintf(stderr,"\n");

    PyObject* myFunction = PyObject_GetAttrString(nq_python_module,function_name);

    fprintf(stderr,"%s:%d:module: ",__FILE__,__LINE__); PyObject_Print(nq_python_module,stderr,0); fprintf(stderr,"\n");

    if (!myFunction) {
      // Try again without _hash on the end
      snprintf(function_name,1024,"nextquestion_%s",s->survey_id);      
      for(int i=0;function_name[i];i++) if (function_name[i]=='/') function_name[i]=0;
      myFunction = PyObject_GetAttrString(nq_python_module,function_name);      
    }
    if (!myFunction) {
      snprintf(function_name,1024,"nextquestion");
      myFunction = PyObject_GetAttrString(nq_python_module,function_name);
    }

    fprintf(stderr,"module: "); PyObject_Print(nq_python_module,stderr,0);
    fprintf(stderr,"\nfunc: "); PyObject_Print(myFunction,stderr,0);
    fprintf(stderr,"\n");

    CHECKPOINT;
    
    if (!myFunction) LOG_ERROR("No matching python function for survey",s->survey_id);
    if (myFunction) fprintf(stderr,"Found python function '%s'\n",function_name);

    if (!PyCallable_Check(myFunction)) LOG_ERROR("Python function is not callable",function_name);
    
    // Okay, we have the function object, so build the argument list and call it.
    PyObject* args = PyTuple_Pack(1,PyFloat_FromDouble(2.0));
    PyObject* result = PyObject_CallObject(myFunction, args);

    Py_DECREF(args);
    Py_DECREF(result);
    
  } while(0);
  return retVal;
}

/*
  Generic next question selector, which selects the first question lacking an answer.
  
*/
int get_next_questions_generic(struct session *s,
			       struct question *next_questions[],int max_next_questions,int *next_question_count)
{
  int retVal=0;
  
  do {
    int i,j;

  if (!s->survey_id) LOG_ERROR("surveyname is NULL","");
  if (!s->session_id) LOG_ERROR("session_uuid is NULL","");
  if (!next_questions) LOG_ERROR("next_questions is NULL","");
  if (max_next_questions<1) LOG_ERROR("max_next_questions < 1","");
  if (!next_question_count) LOG_ERROR("next_question_count is NULL","");

  // Check each question to see if it has been answered already
  for(i=0;i<s->question_count;i++)
    {
      for(j=0;j<s->answer_count;j++)
	if (!strcmp(s->answers[j]->uid,s->questions[i]->uid)) break;
      if (j<s->answer_count) break;
      else {
	if ((*next_question_count)<max_next_questions) {
	  next_questions[*next_question_count]=s->questions[i];
	  (*next_question_count)++;
	}
      }
    }
  
  } while(0);

  return retVal;
  
}

int get_next_questions(struct session *s,
		       struct question *next_questions[],int max_next_questions,int *next_question_count)
{
  // Call the function to get the next question(s) to ask.
  // First see if we have a python function to do the job.
  // If not, then return the list of all not-yet-answered questions
  if (!call_python_nextquestion(s,next_questions,max_next_questions,next_question_count)) return 0;
  return get_next_questions_generic(s,next_questions,max_next_questions,next_question_count);
}
