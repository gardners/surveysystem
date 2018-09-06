
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

int mark_next_question(struct session *s,struct question *next_questions[],
		       int *next_question_count,const char *uid)
{
  int retVal=0;
  do {
    int qn;
    if (!s) LOG_ERROR("session structure is NULL","");
    if (!next_questions) LOG_ERROR("next_questions is null","");
    if (!next_question_count) LOG_ERROR("next_question_count is null","");
    if (!uid) LOG_ERROR("question UID is null","");
    if ((*next_question_count)>=MAX_QUESTIONS) LOG_ERROR("Too many questions in list.",uid);
    
    for(qn=0;qn<s->question_count;qn++)
      if (!strcmp(s->questions[qn]->uid,uid)) break;
    if (qn==s->question_count) LOG_ERROR("Asked to mark non-existent question UID",uid);
    next_questions[*next_question_count]=s->questions[qn];
    (*next_question_count)++;
  } while(0);
  return retVal;
}

int call_python_nextquestion(struct session *s,
			     struct question *next_questions[],int max_next_questions,int *next_question_count)
{
  int retVal=0;
  int is_error=0;
  do {

    // Setup python
    if (setup_python()) {
      fprintf(stderr,"Failed to initialise python:\n");
      dump_errors(stderr); retVal=-1; break;
    }
    if (!nq_python_module) LOG_ERROR("Python module not loaded. Does it have an error?","nextquestion");

    // Build names of candidate functions.
    // nextquestion_<survey_id>_<hash of survey>
    // or failing that, nextquestion_<survey_id>
    // or failing that, nextquestion
    // In all cases, we pass in an list of questions, and an list of answers,
    // and expect a list of strings of question UIDs to ask as the return value.

    char function_name[1024];

    // Try all three possible function names
    snprintf(function_name,1024,"nextquestion_%s",s->survey_id);
    for(int i=0;function_name[i];i++) if (function_name[i]=='/') function_name[i]='_';

    PyObject* myFunction = PyObject_GetAttrString(nq_python_module,function_name);

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

    if (!myFunction) LOG_ERROR("No matching python function for survey",s->survey_id);

    if (!PyCallable_Check(myFunction)) {
      is_error=1;
      LOG_ERROR("Python function is not callable",function_name);
    }

    // Okay, we have the function object, so build the argument list and call it.
    PyObject* questions = PyList_New(s->question_count);
    PyObject* answers = PyList_New(s->answer_count);

    for(int i=0;i<s->question_count;i++) {
      PyObject *item = PyUnicode_FromString(s->questions[i]->uid);
      if (PyList_SetItem(questions,i,item)) {
	Py_DECREF(item);
	LOG_ERROR("Error inserting question name into Python list",s->questions[i]->uid); 
      }
    }
    for(int i=0;i<s->answer_count;i++) {
      PyObject *dict = PyDict_New();
      PyObject *uid = PyUnicode_FromString(s->answers[i]->uid);
      PyObject *text = PyUnicode_FromString(s->answers[i]->text);
      PyObject *value = PyLong_FromLongLong(s->answers[i]->value);
      PyObject *lat = PyLong_FromLongLong(s->answers[i]->lat);
      PyObject *lon = PyLong_FromLongLong(s->answers[i]->lon);
      PyObject *time_begin = PyLong_FromLongLong(s->answers[i]->time_begin);
      PyObject *time_end = PyLong_FromLongLong(s->answers[i]->time_end);
      PyObject *time_zone_delta = PyLong_FromLongLong(s->answers[i]->time_zone_delta);
      PyObject *dst_delta = PyLong_FromLongLong(s->answers[i]->dst_delta);
      PyObject *uid_l = PyUnicode_FromString("uid");
      PyObject *text_l = PyUnicode_FromString("text");
      PyObject *value_l = PyUnicode_FromString("value");
      PyObject *lat_l = PyUnicode_FromString("latitude");
      PyObject *lon_l = PyUnicode_FromString("longitude");
      PyObject *time_begin_l = PyUnicode_FromString("time_begin");
      PyObject *time_end_l = PyUnicode_FromString("time_end");
      PyObject *time_zone_delta_l = PyUnicode_FromString("time_zone_delta");
      PyObject *dst_delta_l = PyUnicode_FromString("dst_delta");
      int errors = PyDict_SetItem(dict,uid_l,uid);
      errors += PyDict_SetItem(dict,text_l,text);
      errors += PyDict_SetItem(dict,value_l,value);
      errors += PyDict_SetItem(dict,lat_l,lat);
      errors += PyDict_SetItem(dict,lon_l,lon);
      errors += PyDict_SetItem(dict,time_begin_l,time_begin);
      errors += PyDict_SetItem(dict,time_end_l,time_end);
      errors += PyDict_SetItem(dict,time_zone_delta_l,time_zone_delta);
      errors += PyDict_SetItem(dict,dst_delta_l,dst_delta);

      if (errors) {
	Py_DECREF(dict);
	LOG_ERROR("Could not construct answer structure for Python. Memory has been leaked.",s->answers[i]->uid);
      }
      
      if (PyList_SetItem(answers,i,dict)) {
	Py_DECREF(dict);
	LOG_ERROR("Error inserting question name into Python list",s->questions[i]->uid); 
      }
    }
    
    PyObject* args = PyTuple_Pack(2,questions,answers);
    
    PyObject* result = PyObject_CallObject(myFunction, args);
    Py_DECREF(args);

    if (!result) {
      is_error=1;
      LOG_ERROR("Python function did not return anything (does it have the correct arguments defined? If not, this can happen)",function_name);
    }
    fprintf(stderr,"Got return value:\n");
    PyObject_Print(result,stderr,0);
    if (PyUnicode_Check(result)) {
      // Get value and put it as single response
      const char *question = PyUnicode_AsUTF8(result);
      if (!question) {
	is_error=1;
	Py_DECREF(result);
	LOG_ERROR("String in reply from Python function is null",function_name);
      }
      if (mark_next_question(s,next_questions,next_question_count,question)) {
	is_error=1;
	Py_DECREF(result);
	LOG_ERROR("Error adding question to list of next questions.  Is it a valid question UID?",question);
      }
    } else if (PyList_Check(result)) {
      fprintf(stderr,"return value is a list \n");
      // XXX Go through list adding values
      int list_len=PyList_Size(result);
      for(int i=0;i<list_len;i++) {
	PyObject *item=PyList_GetItem(result,i);
	if (PyUnicode_Check(item)) {
	  // Get value and put it as single response
	  const char *question = PyUnicode_AsUTF8(item);
	  if (!question) {
	    is_error=1;
	    Py_DECREF(result);
	    LOG_ERROR("String in reply from Python function is null",function_name);
	  }
	  if (mark_next_question(s,next_questions,next_question_count,question)) {
	    is_error=1;
	    Py_DECREF(result);
	    LOG_ERROR("Error adding question to list of next questions.  Is it a valid question UID?",question);
	  }
	} else {
	  Py_DECREF(result);    
	  LOG_ERROR("List item is not a string in response from Python",function_name);
	}
      }
    } else {
      Py_DECREF(result);    
      LOG_ERROR("Return value from Python is neither string nor list.  Empty return should be an empty list.",function_name);
    }

    Py_DECREF(result);    
  } while(0);
  if (is_error) retVal=-99;
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
  int r=call_python_nextquestion(s,next_questions,max_next_questions,next_question_count);
  if (r==-99) return -1;
  if (!r) return 0;
  return get_next_questions_generic(s,next_questions,max_next_questions,next_question_count);
}
