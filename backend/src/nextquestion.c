
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
    if (strlen(s)>=(MAX_WLEN-1)) LOG_ERRORV("String too long '%s', should be < %d characters",s,MAX_WLEN);
    size_t result=mbstowcs(as_wchar_out,s,MAX_WLEN-1);
    if (result<0) LOG_ERRORV("mbstowcs('%s') failed",s);
  } while (0);
  if (retVal) return NULL;
  else return as_wchar_out;
}

int is_python_started=0;
PyObject *nq_python_module=NULL;

void log_python_error()
{
  int retVal=0;
  char *tb_function = "cmodule_traceback";
  do {
    if (1) {
      PyObject *ptype=NULL,*pvalue=NULL,*ptraceback=NULL;
      PyErr_Fetch(&ptype,&pvalue,&ptraceback);
      PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);
      PyObject* objectsRepresentation = PyObject_Repr(ptype);
      const char* s = PyUnicode_AsUTF8(objectsRepresentation);
      LOG_WARNV("Python exception type: %s",s);
      objectsRepresentation = PyObject_Repr(pvalue);
      const char *s2 = PyUnicode_AsUTF8(objectsRepresentation);
      LOG_WARNV("Python error value: %s",s2);
      
      // Fetch a printable traceback string via an optional python callback. There seems to be no convenient way to serialise this via the bindings.
      // Another way to do this might be to create a Python CustomError class with a traceback string injected into the message.
      // However, this would only work if the CustomError is deliberately raised. This method might seem like an overhead but provides tracebacks for any exception type.
      // todo: enable/disable this via a debug flag
      if(!nq_python_module) break;
      PyObject* callable = PyObject_GetAttrString(nq_python_module,tb_function);
      
      if(callable) {
	PyObject* args = PyTuple_Pack(3,ptype,pvalue,ptraceback);
	PyObject* result = PyObject_CallObject(callable, args);
	Py_DECREF(args);
	if (!result) {
	  LOG_WARNV("Could not build Python error traceback with function (%s)",tb_function);
	} else {
	  LOG_WARNV("Python error traceback: %s",PyUnicode_AsUTF8(result));
	}
      } else {
	LOG_WARNV("Unable to build traceback Python function (%s) missing!",tb_function);
      }
    }
  } while(0);

  (void)retVal;
  return;
}

int setup_python(char *search_path)
{
  int retVal=0;
  do {

    // Check if the python source has changed. If so, destroy the current python
    // instance, and start again.
    
    if (!is_python_started) { 

    if (!getenv("SURVEY_HOME")) LOG_ERROR("SURVEY_HOME environment variable not set");
    
    wchar_t *name=as_wchar("nextquestion");
    if (!name) LOG_ERROR("Could not convert string 'nextquestion' to wchar_t *");

    Py_SetProgramName(name);
    Py_Initialize();

    }

    PyObject *syspath_o=PySys_GetObject("path");
    PyObject* rep = PyObject_Repr(syspath_o);
    const char *syspath = PyUnicode_AsUTF8(rep);

    if (!is_python_started) {
    char append_cmd[1024];
    snprintf(append_cmd,1024,"%s/python/", getenv("SURVEY_HOME"));
    if (!strstr(syspath,append_cmd)) {
      // Add python directory to python's search path

      PyObject *sys_path = PySys_GetObject("path");
      if (PyList_Append(sys_path, PyUnicode_FromString(append_cmd)))
	{
	  log_python_error();
	  LOG_ERRORV("Failed to setup python search path using \"%s\"",append_cmd);
	}
    } else {
      LOG_WARNV("Python sys.path already contains '%s'",append_cmd);
    }
    }
  
    // Add python dir, if it exists 
    if (search_path&&search_path[0]) {
      char append_cmd[1024];
      snprintf(append_cmd,1024,"%s", search_path);
      if (!strstr(syspath,append_cmd)) {
      // Add python directory to python's search path

      PyObject *sys_path = PySys_GetObject("path");
      if (PyList_Append(sys_path, PyUnicode_FromString(append_cmd)))
        {
          log_python_error();
          LOG_ERRORV("Failed to setup python search path using \"%s\"",append_cmd);
        }
    } else {
      LOG_WARNV("Python sys.path already contains '%s'",append_cmd);
    } 
    }
 
    //    else
    //      LOG_WARNV("Set up python search path using \"%s\"",append_cmd);

#if 0
    wchar_t path_as_wchar[4096];
    snprintf(append_cmd,1024,"%s/python", getenv("SURVEY_HOME"));
    int len= mbstowcs(path_as_wchar, append_cmd, 100);
    PySys_SetPath(path_as_wchar);
    syspath_o=PySys_GetObject("path");
    rep = PyObject_Repr(syspath_o);
    syspath = PyUnicode_AsUTF8(syspath_o);
    LOG_WARNV("AFTER Python sys.path='%s'",syspath);
#endif
    
    nq_python_module = PyImport_ImportModule("nextquestion");

    if (PyErr_Occurred()) {
      log_python_error();
      PyErr_Clear();
      LOG_WARNV("PyImport_ImportModule('nextquestion') failed.",0);
    }

    if (!nq_python_module) {      
      char append_cmd[1024];
      // So now try to get the module a different way
      snprintf(append_cmd,1024,"import nextquestion");
      int res=PyRun_SimpleString(append_cmd);
      if (res||PyErr_Occurred()) {
	log_python_error();
	PyErr_Clear();
	LOG_ERRORV("'import nextquestion' failed.",0);
      } else LOG_INFOV("import command '%s' appparently succeeded (result = %d).",append_cmd,res);

      
      PyObject *module_name_o= PyUnicode_FromString(append_cmd);
      nq_python_module=PyImport_GetModule(module_name_o);
      if (!nq_python_module) {
	log_python_error();
	LOG_WARNV("Using PyImport_GetModule() didn't work either",0);
      }

      // And if that fails, try loading the file directly as a string.
      char python_file[8192];
      snprintf(python_file,8192,"%s/python/nextquestion.py",getenv("SURVEY_HOME"));
      FILE *f=fopen(python_file,"r");
      if (f) {
	char python_code[1048576]="";
	int bytes=fread(python_code,1,1048576,f);
	if (bytes>0) {
	  int result=PyRun_SimpleString(python_code);
	  LOG_WARNV("PyRun_SimpleString returned %d when processing %d bytes of python",result,bytes);
	}
	
	fclose(f);
      } else LOG_ERRORV("Could not open python file '%s' for reading",python_file);

      module_name_o= PyUnicode_FromString("__main__");
      nq_python_module=PyImport_GetModule(module_name_o);
      if (!nq_python_module) {
	log_python_error();
	LOG_WARNV("Using PyImport_GetModule() didn't work either",0);
      } else LOG_WARNV("Using module __main__ with manually loaded python functions instead of import nextquestion.",0);


      
    }
    
    if (!nq_python_module) {

      PyErr_Print();
      
      LOG_ERROR("Failed to load python module 'nextquestion'");
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
    if (!s) LOG_ERROR("session structure is NULL");
    if (!next_questions) LOG_ERROR("next_questions is null");
    if (!next_question_count) LOG_ERROR("next_question_count is null");
    if (!uid) LOG_ERROR("question UID is null");
    if ((*next_question_count)>=MAX_QUESTIONS) LOG_ERRORV("Too many questions in list when marking question uid='%s'",uid);
    
    for(qn=0;qn<s->question_count;qn++)
      if (!strcmp(s->questions[qn]->uid,uid)) break;
    if (qn==s->question_count) LOG_ERRORV("Asked to mark non-existent question UID '%s'",uid);
    for(int j=0;j<(*next_question_count);j++)
      if (next_questions[j]==s->questions[qn])
	LOG_ERRORV("Duplicate question UID '%s' in list of next questions",uid);
    next_questions[*next_question_count]=s->questions[qn];
    (*next_question_count)++;
  } while(0);
  return retVal;
}

void log_python_object(char *msg,PyObject *result)
{
  char fname[1024];
  snprintf(fname,1024,"/tmp/pyobj.%d.txt",getpid());
  FILE *f=fopen(fname,"w");
  if (f) {
    PyObject_Print(result, f, 0);
    fclose(f);
    f=fopen(fname,"r");	
    char buffer[8192]="";
    if (f) {
      int bytes=fread(buffer,1,8192,f);
      (void)bytes;
      fclose(f);
    }
    if (buffer[0]) LOG_INFOV("%s = %s",msg,buffer);
  }
}

int call_python_nextquestion(struct session *s,
			     struct question *next_questions[],int max_next_questions,int *next_question_count)
{
  int retVal=0;
  int is_error=0;
  do {
    if (!s) LOG_ERROR("session structure is NULL");
    if (!next_questions) LOG_ERROR("next_questions is null");
    if (!next_question_count) LOG_ERROR("next_question_count is null");
    if ((*next_question_count)>=MAX_QUESTIONS) LOG_ERROR("Too many questions in list.");

    // Setup python
    if (setup_python(s->pythondir)) {
      LOG_ERROR("Failed to initialise python.\n");
    }
    if (!nq_python_module) LOG_ERROR("Python module 'nextquestion' not loaded. Does it have an error?");

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
    LOG_INFOV("Searching for python function '%s'",function_name);

    PyObject* myFunction = PyObject_GetAttrString(nq_python_module,function_name);

    if (!myFunction) {
      // Try again without _hash on the end
      snprintf(function_name,1024,"nextquestion_%s",s->survey_id);      
      for(int i=0;function_name[i];i++) if (function_name[i]=='/') function_name[i]=0;
      LOG_INFOV("Searching for python function '%s'",function_name);
      myFunction = PyObject_GetAttrString(nq_python_module,function_name);      
    }
    if (!myFunction) {
      snprintf(function_name,1024,"nextquestion");
      LOG_INFOV("Searching for python function '%s'",function_name);
      myFunction = PyObject_GetAttrString(nq_python_module,function_name);
    }

    if (!myFunction) LOG_ERRORV("No matching python function for survey '%s'",s->survey_id);

    if (!PyCallable_Check(myFunction)) {
      is_error=1;
      LOG_ERRORV("Python function '%s' is not callable",function_name);
    }

    LOG_INFOV("Preparing to call python function '%s' to get next question(s)",function_name);
    
    // TODO see get_analysis() - move answers and questions py list generation in separate unit
    // Okay, we have the function object, so build the argument list and call it.
    PyObject* questions = PyList_New(s->question_count);
    // #227 initialize answer list with the correct length (excluding ANSWER_DELETED)
    int count_given_answers = 0;
    for(int i=0;i<s->answer_count;i++) {
      if (!(s->answers[i]->flags&ANSWER_DELETED)) {
	count_given_answers++;
      }
    }
    PyObject* answers = PyList_New(count_given_answers);

    for(int i=0;i<s->question_count;i++) {
      PyObject *item = PyUnicode_FromString(s->questions[i]->uid);
      if (PyList_SetItem(questions,i,item)) {
	Py_DECREF(item);
	LOG_ERRORV("Error inserting question name '%s' into Python list",s->questions[i]->uid); 
      }
    }
    
    for(int i=0;i<s->answer_count;i++)
      // Don't include deleted answers in the list fed to Python. #186
      if (!(s->answers[i]->flags&ANSWER_DELETED))
	{
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
	    LOG_ERRORV("Could not construct answer structure '%s' for Python. WARNING: Memory has been leaked.",s->answers[i]->uid);
	  }
	  
	  if (PyList_SetItem(answers,i,dict)) {
	    Py_DECREF(dict);
	    LOG_ERRORV("Error inserting question name '%s' into Python list",s->questions[i]->uid); 
	  }
	}
    
    //    log_python_object("Answers",answers);
    
    PyObject* args = PyTuple_Pack(2,questions,answers);
    
    PyObject* result = PyObject_CallObject(myFunction, args);
    Py_DECREF(args);
    
    if(PyErr_Occurred()) {
      is_error=1;
      log_python_error();
      LOG_ERRORV("Python function '%s' exited with an Error (PyErr_Occurred()). Check the backtrace and error messages above, in case they give you any clues.)",function_name);
      PyErr_Clear();
    }
    
    if (!result) {
      is_error=1;
      log_python_error();
      LOG_ERRORV("Python function '%s' did not return anything (does it have the correct arguments defined? If not, this can happen. Check the backtrace and error messages above, in case they give you any clues.)",function_name);
    }
    // PyObject_Print(result,stderr,0);
    if (PyUnicode_Check(result)) {
      // Get value and put it as single response
      const char *question = PyUnicode_AsUTF8(result);
      if (!question) {
	is_error=1;
	Py_DECREF(result);
	LOG_ERRORV("String in reply from Python function '%s' is null",function_name);
      }
      if (mark_next_question(s,next_questions,next_question_count,question)) {
	is_error=1;
	Py_DECREF(result);
	LOG_ERRORV("Error adding question '%s' to list of next questions.  Is it a valid question UID?",question);
      }
    } else if (PyList_Check(result)) {
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
	    LOG_ERRORV("String in reply from Python function '%s' is null",function_name);
	  }
	  if (mark_next_question(s,next_questions,next_question_count,question)) {
	    is_error=1;
	    Py_DECREF(result);
	    LOG_ERRORV("Error adding question '%s' to list of next questions.  Is it a valid question UID?",question);
	  }
	} else {
	  Py_DECREF(result);    
	  LOG_ERRORV("List item is not a string in response from Python function '%s'",function_name);
	}
      }
    } else {
      log_python_object("Return value",result);

      Py_DECREF(result);
      LOG_ERRORV("Return value from Python function '%s' is neither string nor list.  Empty return should be an empty list.",function_name);
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

    if (!s) LOG_ERROR("struct session is NULL");
    if (!s->survey_id) LOG_ERROR("surveyname is NULL");
    if (!s->session_id) LOG_ERROR("session_uuid is NULL");
    if (!next_questions) LOG_ERROR("next_questions is NULL");
    if (max_next_questions<1) LOG_ERROR("max_next_questions < 1");
    if (!next_question_count) LOG_ERROR("next_question_count is NULL");

    LOG_INFOV("Calling get_next_questions_generic()",0);
    
    // Check each question to see if it has been answered already
    for(i=0;i<s->question_count;i++)
	{
	  for(j=0;j<s->answer_count;j++)
	    if (!(s->answers[j]->flags&ANSWER_DELETED))
	      if (!strcmp(s->answers[j]->uid,s->questions[i]->uid)) break;
	  // LOG_INFOV("Answer to question %d is answer %d/%d",i,j,s->answer_count);
	  if (j<s->answer_count) {
	    LOG_INFOV("Answer to question %d exists.",i);
	    continue;
	  }
	  else {
	    if ((*next_question_count)<max_next_questions) {
	      next_questions[*next_question_count]=s->questions[i];
	      (*next_question_count)++;
	      LOG_INFOV("Need answer to question %d.",i);
	      
	      // XXX - For now, just return exactly the first unanswered question
	      break;
	      
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
  int retVal=0;
  do {
    if (!s) LOG_ERROR("session structure is NULL");
    if (!next_questions) LOG_ERROR("next_questions is null");
    if (!next_question_count) LOG_ERROR("next_question_count is null");
    if ((*next_question_count)>=MAX_QUESTIONS) LOG_ERROR("Too many questions in list.");

    if (s->nextquestions_flag & NEXTQUESTIONS_FLAG_PYTHON) {
      int r=call_python_nextquestion(s,next_questions,max_next_questions,next_question_count);
      if (r==-99) { retVal=-1; break; }
      if (!r) { retVal=0; break; }
    }
    // PGS: Disabled generic implementation of nextquestion, since if you have a python version and it can't be loaded
    // for some reason we should NOT fall back, because it may expose questions and IP in a survey that should not be revealed. 
    if (s->nextquestions_flag & NEXTQUESTIONS_FLAG_GENERIC) {
      retVal=get_next_questions_generic(s,next_questions,max_next_questions,next_question_count); 
    } else {
      LOG_ERROR("Could not call python nextquestion function.");
    }
  } while (0);

  return retVal;
}

int get_analysis(struct session *s,const unsigned char **output)
{
  int retVal=0;
  int is_error=0;
  do {
    if (!s) LOG_ERROR("session structure is NULL");
    if (!output) LOG_ERROR("output is NULL");

    // Setup python
    if (setup_python(s->pythondir)) {
      LOG_ERROR("Failed to initialise python.\n");
    }
    if (!nq_python_module) LOG_ERROR("Python module 'nextquestion' not loaded. Does it have an error?");

    // Build names of candidate functions.
    // nextquestion_<survey_id>_<hash of survey>
    // or failing that, nextquestion_<survey_id>
    // or failing that, nextquestion
    // In all cases, we pass in an list of questions, and an list of answers,
    // and expect a list of strings of question UIDs to ask as the return value.

    char function_name[1024];

    // Try all three possible function names
    snprintf(function_name,1024,"analyse_%s",s->survey_id);
    for(int i=0;function_name[i];i++) if (function_name[i]=='/') function_name[i]='_';
    LOG_INFOV("Searching for python function '%s'",function_name);

    PyObject* myFunction = PyObject_GetAttrString(nq_python_module,function_name);

    if (!myFunction) {
      // Try again without _hash on the end
      snprintf(function_name,1024,"analyse_%s",s->survey_id);      
      for(int i=0;function_name[i];i++) if (function_name[i]=='/') function_name[i]=0;
      LOG_INFOV("Searching for python function '%s'",function_name);
      myFunction = PyObject_GetAttrString(nq_python_module,function_name);      
    }
    if (!myFunction) {
      snprintf(function_name,1024,"analyse");
      LOG_INFOV("Searching for python function '%s'",function_name);
      myFunction = PyObject_GetAttrString(nq_python_module,function_name);
    }

    if (!myFunction) LOG_ERRORV("No matching python function for survey '%s'",s->survey_id);

    if (!PyCallable_Check(myFunction)) {
      is_error=1;
      LOG_ERRORV("Python function '%s' is not callable",function_name);
    }

    LOG_INFOV("Preparing to call python function '%s' to get next question(s)",function_name);
    
    // TODO see get_analysis() - move answers and questions py list generation in separate unit
    // Okay, we have the function object, so build the argument list and call it.
    PyObject* questions = PyList_New(s->question_count);
    // #227 initialize answer list with the correct length (excluding ANSWER_DELETED)
    int count_given_answers = 0;
    for(int i=0;i<s->answer_count;i++) {
      if (!(s->answers[i]->flags&ANSWER_DELETED)) {
	count_given_answers++;
      }
    }
    PyObject* answers = PyList_New(count_given_answers);

    for(int i=0;i<s->question_count;i++) {
      PyObject *item = PyUnicode_FromString(s->questions[i]->uid);
      if (PyList_SetItem(questions,i,item)) {
	Py_DECREF(item);
	LOG_ERRORV("Error inserting question name '%s' into Python list",s->questions[i]->uid); 
      }
    }
    for(int i=0;i<s->answer_count;i++)
      // Don't include deleted answers in the list fed to Python. #186
      if (!(s->answers[i]->flags&ANSWER_DELETED))
	{      
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
	    LOG_ERRORV("Could not construct answer structure '%s' for Python. WARNING: Memory has been leaked.",s->answers[i]->uid);
	  }
	  
	  if (PyList_SetItem(answers,i,dict)) {
	    Py_DECREF(dict);
	    LOG_ERRORV("Error inserting question name '%s' into Python list",s->questions[i]->uid); 
	  }
	}
    
    PyObject* args = PyTuple_Pack(2,questions,answers);
    
    PyObject* result = PyObject_CallObject(myFunction, args);
    Py_DECREF(args);

    if(PyErr_Occurred()) {
      is_error=1;
      log_python_error();
      LOG_ERRORV("Python function '%s' exited with an Error (PyErr_Occurred()). Check the backtrace and error messages above, in case they give you any clues.)",function_name);
      PyErr_Clear();
    }
    
    if (!result) {
      is_error=1;
      log_python_error();
      LOG_ERRORV("Python function '%s' did not return anything (does it have the correct arguments defined? If not, this can happen. Check the backtrace and error messages above, in case they give you any clues.)",function_name);
    }
    // PyObject_Print(result,stderr,0);
    if (PyUnicode_Check(result)) {
      // Get value and put it as single response
      const char *return_string = PyUnicode_AsUTF8(result);
      if (!return_string) {
	is_error=1;
	Py_DECREF(result);
	LOG_ERRORV("String in reply from Python function '%s' is null",function_name);
      }
      *output=(const unsigned char *)return_string;
    } else {
      Py_DECREF(result);    
      LOG_ERRORV("Return value from Python function '%s' is not a string.",function_name);
    }

    Py_DECREF(result);    
  } while(0);
  if (is_error) retVal=-99;
  return retVal;
}
