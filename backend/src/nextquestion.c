#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errorlog.h"
#include "question_types.h"
#include "survey.h"
#include "serialisers.h"
#include <Python.h>

#define REPORT_IF_FAILED()                                                     \
  {                                                                            \
    if (retVal)                                                                \
      fprintf(stderr, "%s:%d: %s() failed.\n", __FILE__, __LINE__,             \
              __FUNCTION__);                                                   \
  }
#define CHECKPOINT                                                             \
  fprintf(stderr, "%s:%d:%s() CHECKPOINT\n", __FILE__, __LINE__, __FUNCTION__)

#define MAX_WLEN 65536
wchar_t as_wchar_out[MAX_WLEN];
wchar_t *as_wchar(const char *s) {
  int retVal = 0;
  do {
    if (strlen(s) >= (MAX_WLEN - 1))
      LOG_ERRORV("String too long '%s', should be < %d characters", s,
                 MAX_WLEN);
    size_t result = mbstowcs(as_wchar_out, s, MAX_WLEN - 1);
    if (result == (size_t)-1)
      LOG_ERRORV("mbstowcs('%s') failed", s);
  } while (0);

  if (retVal) {
    return NULL;
  }

  return as_wchar_out;
}

// #361 implement dynamic loading of python
int end_python(void);

// TODO, make thread-safe
int is_python_started = 0;
PyObject *py_module = NULL;

void log_python_error() {
  int retVal = 0;
  char *tb_function = "cmodule_traceback";
  do {
    if (1) {
      PyObject *ptype = NULL, *pvalue = NULL, *ptraceback = NULL;
      PyErr_Fetch(&ptype, &pvalue, &ptraceback);
      PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);
      PyObject *objectsRepresentation = PyObject_Repr(ptype);
      const char *s = PyUnicode_AsUTF8(objectsRepresentation);
      LOG_WARNV("Python exception type: %s", s);
      objectsRepresentation = PyObject_Repr(pvalue);
      const char *s2 = PyUnicode_AsUTF8(objectsRepresentation);
      LOG_WARNV("Python error value: %s", s2);

      // Fetch a printable traceback string via an optional python callback. There seems to be no convenient way to serialise this via the bindings.
      // Another way to do this might be to create a Python CustomError class with a traceback string injected into the message.
      // However, this would only work if the CustomError is deliberately raised. This method might seem like an overhead but provides tracebacks for any exception type.
      // todo: enable/disable this via a debug flag
      if (!py_module) {
        break;
      }
      PyObject *callable =
          PyObject_GetAttrString(py_module, tb_function);

      if (callable) {
        PyObject *args = PyTuple_Pack(3, ptype, pvalue, ptraceback);
        PyObject *result = PyObject_CallObject(callable, args);
        Py_DECREF(args);

        if (!result) {
          LOG_WARNV("Could not build Python error traceback with function (%s)",
                    tb_function);
        } else {
          LOG_WARNV("Python error traceback: %s", PyUnicode_AsUTF8(result));
        }
      } else {
        LOG_WARNV("Unable to build traceback Python function (%s) missing!",
                  tb_function);
      }
    } // endif
  } while (0);

  (void)retVal;
  return;
}

int setup_python() {
  int retVal = 0;
  do {

    // // #361 force re-initalisation. use this only for tests!
    char *force_init = getenv("SURVEY_FORCE_PYINIT");
    if (force_init && atoi(force_init) == 1) {
        LOG_INFO(" => env 'SURVEY_FORCE_PYINIT' was set restarting python");
        // TODO check retVal, should we harcd exit here?
        // sets is_python_started to 0
        end_python();
    }

    // Check if the python source has changed. If so, destroy the current python
    // instance, and start again.

    if (!is_python_started) {
      LOG_INFO(" => starting python setup");
      if (!getenv("SURVEY_HOME"))
        LOG_ERROR("SURVEY_HOME environment variable not set");

      wchar_t *name = as_wchar("nextquestion");
      if (!name)
        LOG_ERROR("Could not convert string 'nextquestion' to wchar_t *");

      Py_SetProgramName(name);
      Py_Initialize();
    }

    PyObject *syspath_o = PySys_GetObject("path");
    PyObject *rep = PyObject_Repr(syspath_o);
    const char *syspath = PyUnicode_AsUTF8(rep);

    if (!is_python_started) {
      char append_cmd[1024];
      if (generate_python_path(append_cmd, 1024)) {
        LOG_ERRORV("Failed to generate python search path using \"%s\"",
                   append_cmd);
      }

      // Add python directory to python's search path
      if (!strstr(syspath, append_cmd)) {
        PyObject *sys_path = PySys_GetObject("path");
        if (PyList_Append(sys_path, PyUnicode_FromString(append_cmd))) {
          log_python_error();
          LOG_ERRORV("Failed to setup python search path using \"%s\"",
                     append_cmd);
        }
      } else {
        LOG_WARNV("Python sys.path already contains '%s'", append_cmd);
      }
    }

    py_module = PyImport_ImportModule("nextquestion");

    if (!py_module) {
      LOG_WARNV("PyImport_ImportModule('nextquestion') failed. returned NULL", 0);
      log_python_error();
      PyErr_Clear();
    }

    if (PyErr_Occurred()) {
      LOG_WARNV("PyImport_ImportModule('nextquestion') failed.", 0);
      log_python_error();
      PyErr_Clear();
    }

    if (!py_module) {
      char append_cmd[1024];
      // So now try to get the module a different way
      snprintf(append_cmd, 1024, "import nextquestion");
      int res = PyRun_SimpleString(append_cmd);
      if (res || PyErr_Occurred()) {
        log_python_error();
        PyErr_Clear();
        LOG_ERRORV("'import nextquestion' failed.", 0);
      } else {
        LOG_INFOV("import command '%s' appparently succeeded (result = %d).",
                  append_cmd, res);
      }

      PyObject *module_name_o = PyUnicode_FromString(append_cmd);
      py_module = PyImport_GetModule(module_name_o);
      if (!py_module) {
        log_python_error();
        LOG_WARNV("Using PyImport_GetModule() didn't work either", 0);
      }

      // And if that fails, try loading the file directly as a string.
      char python_file[8192];
      if (generate_python_path(python_file, 8192)) {
        LOG_ERRORV("Failed to generate python search path using \"%s\"",
                   python_file);
      }

      FILE *f = fopen(python_file, "r");
      if (f) {
        char python_code[1048576] = "";
        int bytes = fread(python_code, 1, 1048576, f);
        if (bytes > 0) {
          int result = PyRun_SimpleString(python_code);
          LOG_WARNV("PyRun_SimpleString returned %d when processing %d bytes "
                    "of python",
                    result, bytes);
        }

        fclose(f);
      } else {
        LOG_ERRORV("Could not open python file '%s' for reading", python_file);
      }

      module_name_o = PyUnicode_FromString("__main__");
      py_module = PyImport_GetModule(module_name_o);
      if (!py_module) {
        log_python_error();
        LOG_WARNV("Using PyImport_GetModule() didn't work either", 0);
      } else {
        LOG_WARNV("Using module __main__ with manually loaded python functions "
                  "instead of import nextquestion.",
                  0);
      }
    }

    if (!py_module) {
      PyErr_Print();
      LOG_ERROR("Failed to load python module 'nextquestion'");
    }

    is_python_started = 1;
  } while (0);
  return retVal;
}

// #361 implement dynamic loading of python
int end_python(void) {
  int retVal = 0;
  do {
    if (!is_python_started) {
      break;
    }

    fprintf(stderr, "STOPPING python\n");

    // Free any python objects we have hanging about
    if (py_module) {
      Py_DECREF(py_module);
      py_module = NULL;
    }

    is_python_started = 0;
    if (Py_FinalizeEx()) { // # 361 added check on finalization
        LOG_ERROR("Py_FinalizeEx() FAILED. Memory has been leaked!");
    }

    LOG_INFO("STOPPING python: python initerpreter and modules were destroyed");

  } while (0);
  return retVal;
}

/**
 * Given a question uid load the question data and append it to the givven next_questions struct
 */
int mark_next_question(struct session *s, struct question *next_questions[],
                       int *next_question_count, const char *uid) {
  int retVal = 0;
  do {
    int qn;
    if (!s)
      LOG_ERROR("session structure is NULL");
    if (!next_questions)
      LOG_ERROR("next_questions is null");
    if (!next_question_count)
      LOG_ERROR("next_question_count is null");
    if (!uid)
      LOG_ERROR("question UID is null");
    if ((*next_question_count) >= MAX_QUESTIONS)
      LOG_ERRORV("Too many questions in list when marking question uid='%s'",
                 uid);

    for (qn = 0; qn < s->question_count; qn++) {
      if (!strcmp(s->questions[qn]->uid, uid)) {
        break;
      }
    }

    if (qn == s->question_count)
      LOG_ERRORV("Asked to mark non-existent question UID '%s'", uid);

    for (int j = 0; j < (*next_question_count); j++) {
      if (next_questions[j] == s->questions[qn]) {
        LOG_ERRORV("Duplicate question UID '%s' in list of next questions",
                   uid);
      }
    }

    next_questions[*next_question_count] = s->questions[qn];
    (*next_question_count)++;
  } while (0);
  return retVal;
}

void log_python_object(char *msg, PyObject *result) {
  char fname[1024];
  snprintf(fname, 1024, "/tmp/pyobj.%d.txt", getpid());
  FILE *f = fopen(fname, "w");

  if (f) {
    PyObject_Print(result, f, 0);
    fclose(f);
    f = fopen(fname, "r");
    char buffer[8192] = "";

    if (f) {
      int bytes = fread(buffer, 1, 8192, f);
      (void)bytes;
      fclose(f);
    }

    if (buffer[0])
      LOG_INFOV("%s = %s", msg, buffer);
  }
}

/**
 * Creates an answer Py_Dict
 *
 * the caller is responsible for derferencing the dict:  if(dict) Py_DECREF(dict);
 */
PyObject *py_create_answer(struct answer *a) {

  // #358 include question type
  char stype[50];
  if(!serialise_question_type(a->type, stype, 50)) { // serialise_question_type returns str length
      return NULL;
  }

  PyObject *dict = PyDict_New();
  PyObject *uid = PyUnicode_FromString(a->uid);
  PyObject *type = PyUnicode_FromString(stype);
  PyObject *text = PyUnicode_FromString(a->text);
  PyObject *value = PyLong_FromLongLong(a->value);
  PyObject *lat = PyLong_FromLongLong(a->lat);
  PyObject *lon = PyLong_FromLongLong(a->lon);
  PyObject *time_begin = PyLong_FromLongLong(a->time_begin);
  PyObject *time_end = PyLong_FromLongLong(a->time_end);
  PyObject *time_zone_delta = PyLong_FromLongLong(a->time_zone_delta);
  PyObject *dst_delta = PyLong_FromLongLong(a->dst_delta);
  //# 299
  PyObject *unit = PyUnicode_FromString(a->unit);
  PyObject *flags = PyLong_FromLongLong(a->flags);
  PyObject *stored = PyLong_FromLongLong(a->stored);

  PyObject *uid_l = PyUnicode_FromString("uid");
  PyObject *type_l = PyUnicode_FromString("type");
  PyObject *text_l = PyUnicode_FromString("text");
  PyObject *value_l = PyUnicode_FromString("value");
  PyObject *lat_l = PyUnicode_FromString("latitude");
  PyObject *lon_l = PyUnicode_FromString("longitude");
  PyObject *time_begin_l = PyUnicode_FromString("time_begin");
  PyObject *time_end_l = PyUnicode_FromString("time_end");
  PyObject *time_zone_delta_l = PyUnicode_FromString("time_zone_delta");
  PyObject *dst_delta_l = PyUnicode_FromString("dst_delta");
  //# 299
  PyObject *unit_l = PyUnicode_FromString("unit");
  PyObject *flags_l = PyUnicode_FromString("flags");
  PyObject *stored_l = PyUnicode_FromString("stored");

  int errors = PyDict_SetItem(dict, uid_l, uid);
  errors += PyDict_SetItem(dict, type_l, type);
  errors += PyDict_SetItem(dict, text_l, text);
  errors += PyDict_SetItem(dict, value_l, value);
  errors += PyDict_SetItem(dict, lat_l, lat);
  errors += PyDict_SetItem(dict, lon_l, lon);
  errors += PyDict_SetItem(dict, time_begin_l, time_begin);
  errors += PyDict_SetItem(dict, time_end_l, time_end);
  errors += PyDict_SetItem(dict, time_zone_delta_l, time_zone_delta);
  errors += PyDict_SetItem(dict, dst_delta_l, dst_delta);
  //# 299
  errors += PyDict_SetItem(dict, unit_l, unit);
  errors += PyDict_SetItem(dict, flags_l, flags);
  errors += PyDict_SetItem(dict, stored_l, stored);

  if (errors) {
    Py_DECREF(dict);
    return NULL;
  }
  return dict;
}

int call_python_nextquestion(struct session *s, struct next_questions *nq) {
  int retVal = 0;
  int is_error = 0;

  do {
    if (!s->survey_id) {
      LOG_ERROR("surveyname is NULL");
    }
    if (!s->session_id) {
      LOG_ERROR("session_uuid is NULL");
    }
    if (!nq) {
      LOG_ERROR("next_questions is NULL");
    }
    if (nq->question_count) {
      LOG_ERROR("next_questions->question_count is > 0");
    }
    // Setup python
    if (setup_python()) {
      LOG_ERROR("Failed to initialise python.\n");
    }
    if (!py_module) {
      LOG_ERROR( "Python module 'nextquestion' not loaded. Does it have an error?");
    }

    // Build names of candidate functions.
    // nextquestion_<survey_id>_<hash of survey>
    // or failing that, nextquestion_<survey_id>
    // or failing that, nextquestion
    // In all cases, we pass in an list of questions, and an list of answers,
    // and expect a list of strings of question UIDs to ask as the return value.

    char function_name[1024];

    // Try all three possible function names
    snprintf(function_name, 1024, "nextquestion_%s", s->survey_id);
    for (int i = 0; function_name[i]; i++) {
      if (function_name[i] == '/') {
        function_name[i] = '_';
      }
    }
    LOG_INFOV("Searching for python function '%s'", function_name);

    PyObject *myFunction =
        PyObject_GetAttrString(py_module, function_name);

    if (!myFunction) {
      // Try again without _hash on the end
      snprintf(function_name, 1024, "nextquestion_%s", s->survey_id);
      for (int i = 0; function_name[i]; i++) {
        if (function_name[i] == '/') {
          function_name[i] = 0;
        }
      }
      LOG_INFOV("Searching for python function '%s'", function_name);
      myFunction = PyObject_GetAttrString(py_module, function_name);
    }

    if (!myFunction) {
      snprintf(function_name, 1024, "nextquestion");
      LOG_INFOV("Searching for python function '%s'", function_name);
      myFunction = PyObject_GetAttrString(py_module, function_name);
    }

    if (!myFunction) {
      LOG_ERRORV("No matching python function for survey '%s'", s->survey_id);
    }

    if (!PyCallable_Check(myFunction)) {
      is_error = 1;
      LOG_ERRORV("Python function '%s' is not callable", function_name);
    }

    LOG_INFOV("Preparing to call python function '%s' to get next question(s)",
              function_name);

    // Okay, we have the function object, so build the argument list and call it.
    //    #227 initialize answer list with the correct length (excluding ANSWER_DELETED)
    //    #363, answer offset, exclude session header

    PyObject *questions = PyList_New(s->question_count);
    int count_given_answers = 0;
    for (int i = s->answer_offset; i < s->answer_count; i++) {
      if (!(s->answers[i]->flags & ANSWER_DELETED)) {
        count_given_answers++;
      }
    }
    PyObject *answers = PyList_New(count_given_answers);

    for (int i = 0; i < s->question_count; i++) {
      PyObject *item = PyUnicode_FromString(s->questions[i]->uid);
      if (PyList_SetItem(questions, i, item)) {
        Py_DECREF(item);
        LOG_ERRORV("Error inserting question name '%s' into Python list",
                   s->questions[i]->uid);
      }
    }

    // #311, PyList increment
    // #363, answer offset, exclude session header
    int listIndex = 0;
    for (int i = s->answer_offset; i < s->answer_count; i++) {
      // Don't include deleted answers in the list fed to Python. #186
      if (!(s->answers[i]->flags & ANSWER_DELETED)) {
        PyObject *dict = py_create_answer(s->answers[i]);

        if (!dict) {
          LOG_ERRORV("Could not construct answer structure '%s' for Python. WARNING: Memory has been leaked.", s->answers[i]->uid);
        }

        if (PyList_SetItem(answers, listIndex, dict)) {
          Py_DECREF(dict);
          LOG_ERRORV("Error inserting answer name '%s' into Python list", s->answers[i]->uid);
        }
        listIndex++;
      }
    }

    //    log_python_object("Answers",answers);
    PyObject *args = PyTuple_Pack(2, questions, answers);

    PyObject *result = PyObject_CallObject(myFunction, args);
    Py_DECREF(args);

    if (PyErr_Occurred()) {
      is_error = 1;
      log_python_error();
      LOG_ERRORV("Python function '%s' exited with an Error "
                 "(PyErr_Occurred()). Check the backtrace and error messages "
                 "above, in case they give you any clues.)",
                 function_name);
      PyErr_Clear();
    }

    if (!result) {
      is_error = 1;
      log_python_error();
      LOG_ERRORV("Python function '%s' did not return anything (does it have "
                 "the correct arguments defined? If not, this can happen. "
                 "Check the backtrace and error messages above, in case they "
                 "give you any clues.)",
                 function_name);
    }

    // #332 add instance check for exported Python class 'NextQuestions'

    if(!PyDict_Check(result)) {
      is_error = 1;
      Py_DECREF(result);
      LOG_ERRORV("Reply from Python function '%s' is of invalid type (not a dict)",
                   function_name);
    }

    // 1. extract status
    PyObject *py_status = PyDict_GetItemString(result, "status");
    if (!py_status) {
      is_error = 1;
      Py_DECREF(result);
      LOG_ERRORV("Reply from Python function '%s' has no member 'status'",
                   function_name);
    }

    long int status = PyLong_AsLong(py_status);
    if(status == -1) {
      is_error = 1;
      Py_DECREF(result);
      LOG_ERRORV("Reply from Python function '%s': invalid member 'status' (int)",
                   function_name);
    }

    // 2. assign status
    nq->status = status;

    // 3. extract message
    PyObject *py_message = PyDict_GetItemString(result, "message");
    if (!py_message) {
      is_error = 1;
      Py_DECREF(result);
      LOG_ERRORV("Reply from Python function '%s' has no member 'message'",
                   function_name);
    }

    const char *message = PyUnicode_AsUTF8(py_message);
    if(!message) {
      is_error = 1;
      Py_DECREF(result);
      LOG_ERRORV("Reply from Python function '%s': invalid member 'message' (str)",
                   function_name);
    }

    // 4. assign message
    nq->message = strdup(message);

    // 5. extract next_questions
    PyObject *py_next_questions = PyDict_GetItemString(result, "next_questions");
    if (!py_next_questions) {
      is_error = 1;
      Py_DECREF(result);
      LOG_ERRORV("Reply from Python function '%s' has no member 'next_questions'",
                   function_name);
    }

    if(!PyList_Check(py_next_questions)) {
      is_error = 1;
      Py_DECREF(result);
      LOG_ERRORV("Reply from Python function '%s': invalid member 'next_questions' (list(str))",
                   function_name);
    }

    int list_len = PyList_Size(py_next_questions);

    // XXX Go through list adding values
    for (int i = 0; i < list_len; i++) {
      PyObject *item = PyList_GetItem(py_next_questions, i);
      if (PyUnicode_Check(item)) {
        // Get value and put it as single response
        const char *uid = PyUnicode_AsUTF8(item);
        if (!uid) {
          is_error = 1;
          Py_DECREF(result);
          LOG_ERRORV("String in reply from Python function '%s' is null",
                      function_name);
        }

        // 6. assign next_questions
        // 7. assign question_count
        nq->next_questions[i] = NULL; // intialize target pointer to NULL!
        if (mark_next_question(s, nq->next_questions, &nq->question_count, uid)) {
          is_error = 1;
          Py_DECREF(result);
          LOG_ERRORV("Error adding question '%s' to list of next questions.  "
                      "Is it a valid question UID?",
                      uid);
        }

      } else {
        Py_DECREF(result);
        LOG_ERRORV(
            "result.next_questions[%d] list item is not a string in response from Python function '%s'",
            i, function_name);
      }

    } // endfor

    Py_DECREF(result);
  } while (0);


  if (is_error) {
    free_next_questions(nq);
    retVal = -99;
  }
  return retVal;
}

/*
  Generic next question selector, which selects the first question lacking an answer.
  #332 next_questions data struct
*/
int get_next_questions_generic(struct session *s, struct next_questions *nq) {
  int retVal = 0;

  do {
    int i, j;

    if (!s)
      LOG_ERROR("struct session is NULL");
    if (!s->survey_id)
      LOG_ERROR("surveyname is NULL");
    if (!s->session_id)
      LOG_ERROR("session_uuid is NULL");
    if (!nq)
      LOG_ERROR("next_questions is NULL");
    if (nq->question_count)
      LOG_ERROR("next_questions->question_count is > 0");

    LOG_INFOV("Calling get_next_questions_generic()", 0);

    // Check each question to see if it has been answered already
    // #363, answer offset, exclude session header
    for (i = 0; i < s->question_count; i++) {
      for (j = s->answer_offset; j < s->answer_count; j++) {
        if (!(s->answers[j]->flags & ANSWER_DELETED)) {
          if (!strcmp(s->answers[j]->uid, s->questions[i]->uid)) {
            break;
          }
        }
      }

      if (j < s->answer_count) {
        LOG_INFOV("Answer to question %d exists.", i);
        continue;
      } else {
        if (nq->question_count < MAX_NEXTQUESTIONS) {
          nq->next_questions[nq->question_count] = s->questions[i];
          nq->question_count++;

          LOG_INFOV("Need answer to question %d.", i);

          // XXX - For now, just return exactly the first unanswered question
          break;
        }
      } // endif

    } // endfor

  } while (0);

  return retVal;
}

// #332 next_questions data struct
int get_next_questions(struct session *s, struct next_questions *nq) {
  // Call the function to get the next question(s) to ask.
  // First see if we have a python function to do the job.
  // If not, then return the list of all not-yet-answered questions
  int retVal = 0;
  do {
    if (!s)
      LOG_ERROR("session structure is NULL");
    if (!nq)
      LOG_ERROR("next_questions structure is null");

    if (s->nextquestions_flag & NEXTQUESTIONS_FLAG_PYTHON) {
      LOG_INFO("NEXTQUESTIONS_FLAG_PYTHON set, calling call_python_nextquestion())");
      int r = call_python_nextquestion(s, nq);

      if (r == -99) {
        free_next_questions(nq);
        retVal = -1;
        break;
      }

      if (!r) {
        retVal = 0;
        break;
      }
    }

    // PGS: Disabled generic implementation of nextquestion, since if you have a python version and it can't be loaded
    // for some reason we should NOT fall back, because it may expose questions and IP in a survey that should not be revealed.
    if (s->nextquestions_flag & NEXTQUESTIONS_FLAG_GENERIC) {
      LOG_INFO("NEXTQUESTIONS_FLAG_GENERIC set, calling "
               "get_next_questions_generic())");
      retVal = get_next_questions_generic(s, nq);
    } else {
      free_next_questions(nq);
      LOG_ERROR("Could not call python nextquestion function.");
    }
  } while (0);

  return retVal;
}

/*
 * #300 prepare analysis string response
 * TODO (not yet implemeneted in main.c)
 * **output needs to be freed outside of this function
 */
int get_analysis_generic(struct session *s, const char **output) {
  int retVal = 0;

  do {
    *output = strdup("\"NOT IMPLEMENTED\""); // write something (valid json)
  } while (0);

  return retVal;
}

/*
 * Fetch analysis json string via Python script
 * #288, the parent unit is responsible for freeing *output pointer
 * **output needs to be freed outside of this function
 */
int call_python_analysis(struct session *s, const char **output) {
  int retVal = 0;
  int is_error = 0;
  do {
    if (!s)
      LOG_ERROR("session structure is NULL");
    if (!output)
      LOG_ERROR("output is NULL");

    // Setup python
    if (setup_python()) {
      LOG_ERROR("Failed to initialise python.\n");
    }
    if (!py_module)
      LOG_ERROR(
          "Python module 'nextquestion' not loaded. Does it have an error?");

    // Build names of candidate functions.
    // nextquestion_<survey_id>_<hash of survey>
    // or failing that, nextquestion_<survey_id>
    // or failing that, nextquestion
    // In all cases, we pass in an list of questions, and an list of answers,
    // and expect a list of strings of question UIDs to ask as the return value.

    char function_name[1024];

    // Try all three possible function names
    snprintf(function_name, 1024, "analyse_%s", s->survey_id);
    for (int i = 0; function_name[i]; i++) {
      if (function_name[i] == '/') {
        function_name[i] = '_';
      }
    }
    LOG_INFOV("Searching for python function '%s'", function_name);

    PyObject *myFunction =
        PyObject_GetAttrString(py_module, function_name);

    if (!myFunction) {
      // Try again without _hash on the end
      snprintf(function_name, 1024, "analyse_%s", s->survey_id);
      for (int i = 0; function_name[i]; i++) {
        if (function_name[i] == '/') {
          function_name[i] = 0;
        }
      }
      LOG_INFOV("Searching for python function '%s'", function_name);
      myFunction = PyObject_GetAttrString(py_module, function_name);
    }

    if (!myFunction) {
      snprintf(function_name, 1024, "analyse");
      LOG_INFOV("Searching for python function '%s'", function_name);
      myFunction = PyObject_GetAttrString(py_module, function_name);
    }

    if (!myFunction)
      LOG_ERRORV("No matching python function for survey '%s'", s->survey_id);

    if (!PyCallable_Check(myFunction)) {
      is_error = 1;
      LOG_ERRORV("Python function '%s' is not callable", function_name);
    }

    LOG_INFOV("Preparing to call python function '%s' to get next question(s)",
              function_name);

    // TODO see get_analysis() - move answers and questions py list generation in separate unit
    // Okay, we have the function object, so build the argument list and call it.
    PyObject *questions = PyList_New(s->question_count);

    // #227 initialize answer list with the correct length (excluding ANSWER_DELETED)
    // #363, answer offset, exclude session header
    int count_given_answers = 0;
    for (int i = s->answer_offset; i < s->answer_count; i++) {
      if (!(s->answers[i]->flags & ANSWER_DELETED)) {
        count_given_answers++;
      }
    }
    PyObject *answers = PyList_New(count_given_answers);

    for (int i = 0; i < s->question_count; i++) {
      PyObject *item = PyUnicode_FromString(s->questions[i]->uid);
      if (PyList_SetItem(questions, i, item)) {
        Py_DECREF(item);
        LOG_ERRORV("Error inserting question name '%s' into Python list",
                   s->questions[i]->uid);
      }
    }

    // #311, PyList increment
    // #363, answer offset, exclude session header
    int listIndex = 0;
    for (int i = s->answer_offset; i < s->answer_count; i++) {
      // Don't include deleted answers in the list fed to Python. #186
      if (!(s->answers[i]->flags & ANSWER_DELETED)) {
        PyObject *dict = py_create_answer(s->answers[i]);

        if (!dict) {
          LOG_ERRORV("Could not construct answer structure '%s' for Python. "
                     "WARNING: Memory has been leaked.",
                     s->answers[i]->uid);
        }

        if (PyList_SetItem(answers, listIndex, dict)) {
          Py_DECREF(dict);
          LOG_ERRORV("Error inserting answer name '%s' into Python list",
                     s->answers[i]->uid);
        }
        listIndex++;
      }
    }

    PyObject *args = PyTuple_Pack(2, questions, answers);
    PyObject *result = PyObject_CallObject(myFunction, args);
    Py_DECREF(args);

    if (PyErr_Occurred()) {
      is_error = 1;
      log_python_error();
      LOG_ERRORV("Python function '%s' exited with an Error "
                 "(PyErr_Occurred()). Check the backtrace and error messages "
                 "above, in case they give you any clues.)",
                 function_name);
      PyErr_Clear();
    }

    if (!result) {
      is_error = 1;
      log_python_error();
      LOG_ERRORV("Python function '%s' did not return anything (does it have "
                 "the correct arguments defined? If not, this can happen. "
                 "Check the backtrace and error messages above, in case they "
                 "give you any clues.)",
                 function_name);
    }

    // PyObject_Print(result,stderr,0);
    if (PyUnicode_Check(result)) {

      // Get value and put it as single response
      // #288, replaced PyUnicode_AsUTF8 with PyUnicode_AsUTF8AndSize
      Py_ssize_t size;
      const char *return_string = PyUnicode_AsUTF8AndSize(result, &size);

      if (!return_string) {
        is_error = 1;
        Py_DECREF(result);
        LOG_ERRORV("String in reply from Python function '%s' is null",
                   function_name);
      }

      // #288, allocate dedicated memory, managed by backend and write string
      *output = strndup(return_string, (size_t)size);
      // TODO should issues occur then consider hard-setting a \0 token at the last position of *output
    } else {
      Py_DECREF(result);
      LOG_ERRORV("Return value from Python function '%s' is not a string.",
                 function_name);
    }

    Py_DECREF(result);
  } while (0);

  if (is_error) {
    retVal = -99;
  }
  return retVal;
}

int get_analysis(struct session *s, const char **output) {
  // Call the function to get the next question(s) to ask.
  // First see if we have a python function to do the job.
  // If not, then return the list of all not-yet-answered questions
  int retVal = 0;
  do {
    if (!s) {
      LOG_ERROR("session structure is NULL");
    }
    if (!output) {
      LOG_ERROR("output is NULL");
    }

    if (s->nextquestions_flag & NEXTQUESTIONS_FLAG_PYTHON) {
      LOG_INFO("NEXTQUESTIONS_FLAG_PYTHON set, calling call_python_analysis())");
      int r = call_python_analysis(s, output);

      if (r == -99) {
        retVal = -1;
        break;
      }

      if (!r) {
        retVal = 0;
        break;
      }
    }

    // PGS: Disabled generic implementation of nextquestion, since if you have a python version and it can't be loaded
    // for some reason we should NOT fall back, because it may expose questions and IP in a survey that should not be revealed.
    if (s->nextquestions_flag & NEXTQUESTIONS_FLAG_GENERIC) {
      LOG_INFO("NEXTQUESTIONS_FLAG_GENERIC set, calling "
               "get_next_qanalysis_generic())");
      retVal = get_analysis_generic(s, output);
    } else {
      LOG_ERROR("Could not call python analysis function.");
    }
  } while (0);

  return retVal;
}
