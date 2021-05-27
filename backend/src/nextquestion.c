#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errorlog.h"
#include "question_types.h"
#include "survey.h"
#include "serialisers.h"
#include "utils.h"
#include <Python.h>

#define REPORT_IF_FAILED()                                                     \
  {                                                                            \
    if (retVal)                                                                \
      fprintf(stderr, "%s:%d: %s() failed.\n", __FILE__, __LINE__,             \
              __FUNCTION__);                                                   \
  }

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
 * flags whether a survey answer has been answered, excluding internal system answers
 *  - answer not deleted
 *  - answer not of QTYPE_META
 *  - answer not uid not pefixed with '@'
 */
int is_given_answer(struct answer *a) {
  if (!a) {
    LOG_WARNV("ERROR: is_given_answer() received NULL answer", 0);
    return 0;
  }

  if (a->flags & ANSWER_DELETED) {
    return 0;
  }

  if (a->type == QTYPE_META) {
    return 0;
  }

  if (a->uid[0] == '@') {
    return 0;
  }

  return 1;
}

/**
 * flags whether a survey answer is an internal system answer
 *  - answer is of QTYPE_META
 *  - answer uid is pefixed with '@'
 */
int is_system_answer(struct answer *a) {
  if (!a) {
    LOG_WARNV("ERROR: is_system_answer() received NULL answer", 0);
    return 1;
  }

  if (a->type == QTYPE_META) {
    return 1;
  }

  if (a->uid[0] == '@') {
    return 1;
  }

  return 0;
}

/**
 * Given a question uid load the question data and append it to the givven next_questions struct
 */
int mark_next_question(struct session *s, struct question *next_questions[], int *next_question_count, const char *uid) {
  int retVal = 0;
  do {
    int qn;
    if (!s) {
      LOG_ERROR("session structure is NULL");
    }
    if (!next_questions) {
      LOG_ERROR("next_questions is null");
    }
    if (!next_question_count) {
      LOG_ERROR("next_question_count is null");
    }
    if (!uid) {
      LOG_ERROR("question UID is null");
    }
    if ((*next_question_count) >= MAX_QUESTIONS) {
      LOG_ERRORV("Too many questions in list when marking question uid='%s'", uid);
    }

    for (qn = 0; qn < s->question_count; qn++) {
      if (!strcmp(s->questions[qn]->uid, uid)) {
        break;
      }
    }

    if (qn == s->question_count) {
      LOG_ERRORV("Asked to mark non-existent question UID '%s'", uid);
    }

    for (int j = 0; j < (*next_question_count); j++) {
      if (next_questions[j] == s->questions[qn]) {
        LOG_ERRORV("Duplicate question UID '%s' in list of next questions", uid);
        break;
      }
    }

    // #373 separate allocated space for questions
    next_questions[*next_question_count] = copy_question(s->questions[qn]);
    if (!next_questions[*next_question_count]) {
      LOG_ERRORV("Copying question '%s' in list of next questions failed", uid);
    }
    (*next_question_count)++;
  } while (0);
  return retVal;
}

void log_python_object(char *msg, PyObject *result) {
  PyObject* repr = PyObject_Str(result);
  PyObject* str = PyUnicode_AsEncodedString(repr, "utf-8", "~E~");

  // https://docs.python.org/3/c-api/bytes.html#c.PyBytes_AsString: "It must not be deallocated"
  const char *bytes = PyBytes_AS_STRING(str);
  LOG_INFOV("%s: '%s'", msg, bytes);

  Py_XDECREF(repr);
  Py_XDECREF(str);
}

/**
 * Loads Python next_question module and searches for defined functions with the following naming patterns:
 *  - <base_function>_<survey_id>_<survey_hash>
 *  - <base_function_<survey_id>
 *  - <base_function>
 * Pattern example, given the base_function name is 'nextquestion' and syrvey_id is 'foo/12345'
 *  - nextquestion_foo_12345
 *  - nextquestion_foo
 *  - nextquestion
 * returns a Python callable object and sets function_name (mainly for parent logging)
 */
PyObject *py_get_hook_function(char *base_function, char *survey_id, char *function_name, size_t len) {
  int retVal = 0;

  PyObject *func = NULL;

  do {
    if (!py_module) {
      LOG_ERROR("python module not initialised");
    }
    if (!base_function || base_function[0] == 0) {
      LOG_ERROR("base_function is empty");
    }
    if (!survey_id || survey_id[0] == 0) {
      LOG_ERROR("survey_id is empty");
    }

    // 1. <base_function>_<survey_id>_<survey_hash>()

    snprintf(function_name, len, "%s_%s", base_function, survey_id);
    for (int i = 0; function_name[i]; i++) {
        if (function_name[i] == '/') {
          function_name[i] = '_';
        }
    }
    LOG_INFOV("Searching for python function '%s'", function_name);
    func = PyObject_GetAttrString(py_module, function_name);

    // 2. <base_function>_<survey_id>()

    if (!func) {
        // Try again without _hash on the end
        snprintf(function_name, len, "%s_%s", base_function, survey_id);
        for (int i = 0; function_name[i]; i++) {
          if (function_name[i] == '/') {
              function_name[i] = 0;
          }
        }
        LOG_INFOV("Searching for python function '%s'", function_name);
        func = PyObject_GetAttrString(py_module, function_name);
    }

    // 2. <base_function>()

    if (!func) {
        snprintf(function_name, len, "%s", base_function);
        LOG_INFOV("Searching for python function '%s'", function_name);
        func = PyObject_GetAttrString(py_module, function_name);
    }

    if (!func) {
        LOG_ERRORV("No matching python function for base '%s', survey '%s'", base_function, survey_id);
    }

    if (!PyCallable_Check(func)) {
        LOG_ERRORV("Python function '%s' is not a callable, survey '%s'", function_name, survey_id);
    }

    LOG_INFOV("Preparing to call python hook '%s()' for base function '%s()'", base_function, function_name);

  } while(0);

  if (retVal) {
    Py_XDECREF(func);
    return NULL;
  }

  return func;
};

/**
 * Creates an answer Py_Dict
 *
 * the caller is responsible for derferencing the dict:  if(dict) Py_DECREF(dict);
 */
PyObject *py_create_answer(struct answer *a) {
  int retVal = 0;
  PyObject *dict = NULL;

  do {
    // #358 include question type
    char stype[50];
    if(!serialise_question_type(a->type, stype, 50)) { // serialise_question_type returns str length
        return NULL;
    }

    dict = PyDict_New();
    if(!dict) {
      LOG_ERROR("failed to create answer dict");
    }

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
      LOG_ERRORV("setting dict item failed with %d errors", errors);
    }

  } while (0);

  if (retVal) {
    Py_XDECREF(dict);
    return NULL;
  }

  return dict;
}

/**
 * Creates and a Python list of question uids for an existing session
 * #445
 * returns PyObject pointer or NULL on error
 */
PyObject *py_create_questions_list(struct session *ses) {
    int retVal = 0;
    PyObject *questions = NULL;

    do {
        questions = PyList_New(ses->question_count);

        // build arguments: questions list
        for (int i = 0; i < ses->question_count; i++) {
            PyObject *item = PyUnicode_FromString(ses->questions[i]->uid);
            if (PyList_SetItem(questions, i, item)) {
                Py_DECREF(item);
                LOG_ERRORV("Error inserting question name '%s' into Python list", ses->questions[i]->uid);
            }
        }
    } while(0);

    if (retVal) {
        Py_XDECREF(questions);
        return NULL;
    }

    return questions;
}

/**
 * Merges some question properties into py dict answer
 * Question properties are prefixed with an underscore. For the sake of simplicitiy we refrain from creating a nested question dict.
 * Note: This function will not DECREF the answer dict on failure
 * #449
 */
int py_answer_merge_question(PyObject *ans, struct question *qn) {
  int retVal = 0;

  do {
    if(!PyDict_Check(ans)) {
      LOG_ERROR("py dict answer is NULL or not a dict");
    }
    if(!qn) {
      LOG_ERROR("question is NULL");
    }

    PyObject *_flags = PyUnicode_FromString("_flags");
    PyObject *_default_value = PyUnicode_FromString("_default_value");
    PyObject *_min_value = PyUnicode_FromString("_min_value");
    PyObject *_max_value = PyUnicode_FromString("_max_value");
    PyObject *_choices = PyUnicode_FromString("_choices");
    PyObject *_unit = PyUnicode_FromString("_unit"); // #448

    PyObject *flags = PyLong_FromLongLong(qn->flags);
    PyObject *default_value = PyUnicode_FromString(qn->default_value);
    PyObject *min_value = PyLong_FromLongLong(qn->min_value);
    PyObject *max_value = PyLong_FromLongLong(qn->max_value);
    PyObject *choices = PyUnicode_FromString(qn->choices);
    PyObject *unit = PyUnicode_FromString(qn->unit);

    int errors = PyDict_SetItem(ans, _flags, flags);
    errors += PyDict_SetItem(ans, _default_value, default_value);
    errors += PyDict_SetItem(ans, _min_value, min_value);
    errors += PyDict_SetItem(ans, _max_value, max_value);
    errors += PyDict_SetItem(ans, _choices, choices);
    errors += PyDict_SetItem(ans, _unit, unit);

    if (errors) {
      LOG_ERRORV("setting dict item to py answer failed with %d errors", errors);
    }

  } while(0);

  return retVal;
}

/**
 * Creates and a Python list of answer dicts for an existing session.
 * Header answers and system answers are excluded
 * #445
 * returns PyObject pointer or NULL on error
 */
PyObject *py_create_answers_list(struct session *ses) {
    int retVal = 0;
    PyObject *answers = NULL;

    do {
        answers = PyList_New(ses->given_answer_count);

        // build arguments: answers list
        int listIndex = 0;
        for (int i = ses->answer_offset; i < ses->answer_count; i++) {

          if (is_given_answer(ses->answers[i])) {
            PyObject *item = py_create_answer(ses->answers[i]);
            if (!item) {
              LOG_ERRORV("Could not construct answer structure '%s' for Python. WARNING: Memory has been leaked.", ses->answers[i]->uid);
            }

            struct question *qn = session_get_question(ses->answers[i]->uid, ses);
            if (py_answer_merge_question(item, qn)) {
              Py_DECREF(item);
              LOG_ERRORV("Error merging question properties into PyDict answer '%s'", ses->answers[i]->uid);
            }

            if (PyList_SetItem(answers, listIndex, item)) {
              Py_DECREF(item);
              LOG_ERRORV("Error inserting answer name '%s' into Python list", ses->answers[i]->uid);
            }

            listIndex++;
          } // if is_given_answer

        }

    } while(0);

    if (retVal) {
        Py_XDECREF(answers);
        return NULL;
    }

    return answers;
}

/**
 * Compiles arguments for a python callable (hook) and executes it
 * returns result
 *
 * The result can be NULL. It's up to the callee to evaluate whether this is considered as an error or not.
 */
PyObject *py_invoke_hook_function(PyObject *myFunction, struct session *ses, enum actions action, int affected_answers_count) {
  int retVal = 0;

  PyObject *kwargs = NULL;
  PyObject *questions = NULL;
  PyObject *answers = NULL;

  PyObject *result = NULL;

  do {
    if (!ses) {
      LOG_ERROR("session is null");
    }
    if (!ses->survey_id || ses->survey_id[0] == 0) {
        LOG_ERROR("session->survey_id is empty");
    }
    if (!ses->session_id|| ses->session_id[0] == 0) {
        LOG_ERROR("session->session_id is empty");
    }
    if (action < ACTION_NONE || action >= NUM_SESSION_ACTIONS) {
        LOG_ERROR("invalid action");
    }

    // build positional args

    questions = py_create_questions_list(ses);
    if (!questions) {
        LOG_ERROR("Error building positional arg (questions list)");
    }

    answers = py_create_answers_list(ses);
    if (!answers) {
        LOG_ERROR("Error building positional arg (answers list)");
    }

    PyObject *args = PyTuple_Pack(2, questions, answers);

    // build context args

    kwargs = Py_BuildValue(
      "{s:s:s:s,s:s:s:i}",
      "survey_id", ses->survey_id,
      "session_id", ses->session_id,
      "action",  session_action_names[action],
      "affected_count", affected_answers_count
    );

    if(!kwargs) {
      LOG_INFO("Failed to build keyword args (context)");
    }

    result = PyObject_Call(myFunction, args, kwargs);
    Py_XDECREF(args);
    Py_XDECREF(kwargs);

    if (PyErr_Occurred()) {
      log_python_error();
      LOG_ERROR("Python function exited with an Error "
                 "(PyErr_Occurred()). Check the backtrace and error messages "
                 "above, in case they give you any clues.)");
      PyErr_Clear();
    }

  } while(0);

  if (retVal) {
    Py_XDECREF(kwargs);
    Py_XDECREF(questions);
    Py_XDECREF(answers);

    Py_XDECREF(result);
    return NULL;
  }

  return result;
}

/**
 * handler function for processing python nextquestions progress values
 * on error, PyObject result remains open and has to be destroyed by callee
 * #13
 */
int py_nextquestions_handle_progress(PyObject *result, struct nextquestions *nq) {
  int retVal = 0;

  do {

    if (!result || !PyDict_Check(result)) {
      LOG_ERROR("progress: PyObject *result is NULL or not a PyDict");
    }

    if (!nq) {
      LOG_ERROR("progress: struct *nq is NULL");
    }

    PyObject *list = PyDict_GetItemString(result, "progress");
    if (!list) {
      LOG_ERROR("progress PyObject *result has no member 'next_questions'");
    }

    if(!PyList_Check(list)) {
      LOG_ERROR("progress PyObject *result['next_questions'] is not a PyList");
    }

    int len = PyList_Size(list);
    if(len != 2) {
      LOG_ERRORV("progress PyObject *result['next_questions'] list length must be exact 2, %d given", len);
    }

    PyObject *item;
    long int count;

    // note: PyLong_AsLong() returns '-1' on error, docs recommend to use PyErr_Occurred() to disambiguate.
    // We are skipping this here and pass -1 on to the frontends
    item = PyList_GetItem(list, 0);
    count = PyLong_AsLong(item);
    nq->progress[0] = (int) count;

    item = PyList_GetItem(list, 1);
    count = PyLong_AsLong(item);
    nq->progress[1] = (int) count;

  } while (0);

  return retVal;
}

int dump_next_questions(FILE *f, struct nextquestions *nq) {
  int retVal = 0;
  int i;
  do {
    if (!f) {
      LOG_ERROR("dump_next_questions(): invalid file pointer.");
    }

    fprintf(f, "{\n");
    if (!nq) {
      fprintf(f, "nextquestions { <NULL> }\n");
      break;
    }

    fprintf(
      f,
      "nextquestions {\n"
      "  status: %d\n"
      "  message: %s\n"
      "  question_count: %d\n"
      "  progress: [%d, %d]\n"
      "  questions: [\n",
      nq->status,
      nq->message,
      nq->question_count,
      // #13 add suport for progress indicator
      nq->progress[0],
      nq->progress[1]
    );

    for (i = 0; i < nq->question_count; i++) {
      fprintf(f, "    %s%s\n", nq->next_questions[i]->uid, (i < nq->question_count - 1) ? ",": "");
    }

    fprintf(f , "  ]\n}\n");
  } while (0);

  return retVal;
}

// #332 initialise nextquestions data struct
struct nextquestions *init_next_questions() {
  int retVal = 0;
  struct nextquestions *nq = NULL;

  do {
    nq = calloc(sizeof(struct nextquestions), 1);
    if (!nq) {
      LOG_ERRORV("calloc(%d,1) failed when loading struct nextquestions", sizeof(struct nextquestions));
    }
    nq->status = 0;
    nq->message = NULL;
    nq->question_count = 0;
    // #13 add suport for progress indicator
    nq->progress[0] = 0;
    nq->progress[1] = 0;
  } while (0);

  if (retVal) {
    nq = NULL;
  }
  return nq;
}

// #332 free nextquestions data struct
void free_next_questions(struct nextquestions *nq) {
  if (!nq) {
    return;
  }
  freez(nq->message);
  for (int i = 0; i < nq->question_count; i++) {
    free_question(nq->next_questions[i]);
  }
  nq->question_count = 0;
  // #13 add suport for progress indicator
  nq->progress[0] = 0;
  nq->progress[1] = 0;
  free(nq);
  return;
}

/**
 * Python api next question selector.
 * Loads Python next_question module, loads and calls defined "nextquestion_* function.
 * We pass into the function:
 * - list of questions
 * - list of answers
 * - context information about the current session and performend action
 * We expect a list of strings of question UIDs to ask as the return value.
 *
 * #332 next_questions data struct
 * #445 add action, affected_answers_count args (to be used in later development), out sourcetasks into units
 */
int call_python_nextquestion(struct session *s, struct nextquestions *nq, enum actions action, int affected_answers_count) {
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
      LOG_ERROR("nextquestions is NULL");
    }
    if (nq->question_count) {
      LOG_ERROR("nextquestions->question_count is > 0");
    }
    // Setup python
    if (setup_python()) {
      LOG_ERROR("Failed to initialise python.\n");
    }
    if (!py_module) {
      LOG_ERROR( "Python module 'nextquestion' not loaded. Does it have an error?");
    }

    // select from avaliable hook functions
    char function_name[1024];
    PyObject *myFunction = py_get_hook_function("nextquestion", s->survey_id, function_name, 1024);
    if (!myFunction) {
      LOG_ERROR("Failed to get hook function for 'nextquestion'");
    }

    // Okay, we have the function object, so build the argument list and call it.
    PyObject *result = py_invoke_hook_function(myFunction, s, action, affected_answers_count);

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
      LOG_ERRORV("Reply from Python function '%s' is of invalid type (not a dict)", function_name);
    }

    // 1. extract status
    PyObject *py_status = PyDict_GetItemString(result, "status");
    if (!py_status) {
      is_error = 1;
      Py_DECREF(result);
      LOG_ERRORV("Reply from Python function '%s' has no member 'status'", function_name);
    }

    long int status = PyLong_AsLong(py_status);
    if(status == -1) {
      is_error = 1;
      Py_DECREF(result);
      LOG_ERRORV("Reply from Python function '%s': invalid member 'status' (int)", function_name);
    }

    // 2. assign status
    nq->status = status;

    // 3. extract message
    PyObject *py_message = PyDict_GetItemString(result, "message");
    if (!py_message) {
      is_error = 1;
      Py_DECREF(result);
      LOG_ERRORV("Reply from Python function '%s' has no member 'message'", function_name);
    }

    const char *message = PyUnicode_AsUTF8(py_message);
    if(!message) {
      is_error = 1;
      Py_DECREF(result);
      LOG_ERRORV("Reply from Python function '%s': invalid member 'message' (str)", function_name);
    }

    // 4. assign message
    nq->message = strdup(message);

    // 5. progress values
    if (py_nextquestions_handle_progress(result, nq)) {
      is_error = 1;
      Py_DECREF(result);
      LOG_ERRORV("Reply from Python function '%s': progess handler failed", function_name);
    }

    // 6. extract nextquestions
    PyObject *py_next_questions = PyDict_GetItemString(result, "next_questions");
    if (!py_next_questions) {
      is_error = 1;
      Py_DECREF(result);
      LOG_ERRORV("Reply from Python function '%s' has no member 'next_questions'", function_name);
    }

    if(!PyList_Check(py_next_questions)) {
      is_error = 1;
      Py_DECREF(result);
      LOG_ERRORV("Reply from Python function '%s': invalid member 'next_questions' (list(str))", function_name);
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
          LOG_ERRORV("String in reply from Python function '%s' is null", function_name);
        }

        // 6. assign next_questions
        // 7. assign question_count
        nq->next_questions[i] = NULL; // intialize target pointer to NULL!
        if (mark_next_question(s, nq->next_questions, &nq->question_count, uid)) {
          is_error = 1;
          Py_DECREF(result);
          LOG_ERRORV("Error adding question '%s' to list of next questions, question uid valid?", uid);
        }

      } else {
        Py_DECREF(result);
        LOG_ERRORV(
            "result.next_questions[%d] item is not a string in response from Python function '%s'", i, function_name);
      }

    } // endfor

    Py_DECREF(result);
    LOG_INFO("call python next question(s) finished.");
  } while (0);


  if (is_error) {
    retVal = -99;
  }
  return retVal;
}

/**
 * Generic next question selector, which selects the first question lacking an answer.
 * #332 next_questions data struct
 * #445 add action, affected_answers_count args (to be used in later development)
 */
int get_next_questions_generic(struct session *s, struct nextquestions *nq, enum actions action, int affected_answers_count) {
  int retVal = 0;

  do {
    int i, j;

    if (!s) {
      LOG_ERROR("struct session is NULL");
    }
    if (!s->survey_id) {
      LOG_ERROR("surveyname is NULL");
    }
    if (!s->session_id) {
      LOG_ERROR("session_uuid is NULL");
    }
    if (!nq) {
      LOG_ERROR("nextquestions is NULL");
    }
    if (nq->question_count) {
      LOG_ERROR("nextquestions->question_count is > 0");
    }

    LOG_INFOV("Calling get_next_questions_generic()", 0);

    // #13 add suport for progress indicator, in generic (linear) mode we just copy the session counters
    nq->progress[0] = s->given_answer_count;
    nq->progress[1] = s->question_count;

    // Check each question to see if it has been answered already
    // #363, answer offset, exclude session header
    // #363, note: system answers (QTYPE_META or @uid) answers are not affected here because they should not have questions
    //   TODO it might be neccessary to add guards for questions 'abusing' the notation rules for automatted backend answers
    for (i = 0; i < s->question_count; i++) {

      for (j = s->answer_offset; j < s->answer_count; j++) {
        if (!(s->answers[j]->flags & ANSWER_DELETED)) {
          if (!strcmp(s->answers[j]->uid, s->questions[i]->uid)) {
            break;
          }
        }
      }

      if (j < s->answer_count) {
        // LOG_INFOV("Answer to question %d exists.", i);
        continue;
      } else {
        if (nq->question_count < MAX_NEXTQUESTIONS) {
          // #373 separate allocated space for questions
          nq->next_questions[nq->question_count] = copy_question(s->questions[i]);
          if (!nq->next_questions[nq->question_count]) {
            LOG_ERRORV("Copying question '%s' in list of next questions failed", s->questions[i]->uid);
            break;
          }
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

/**
 * Get next questions
 * dispatcher function (generic or python)
 * #332 nextquestions data struct
 * #379 refactor
 */
struct nextquestions *get_next_questions(struct session *s, enum actions action, int affected_answers_count) {
  // Call the function to get the next question(s) to ask.
  // First see if we have a python function to do the job.
  // If not, then return the list of all not-yet-answered questions
  int retVal = 0;
  int fail = 0;
  struct nextquestions *nq = NULL;

  do {
    if (!s) {
      LOG_ERROR("session structure is NULL");
    }

    nq = init_next_questions();
    if (!nq) {
      LOG_ERROR("init_next_questions() failed");
    }

    if (s->nextquestions_flag & NEXTQUESTIONS_FLAG_PYTHON) {

      LOG_INFO("NEXTQUESTIONS_FLAG_PYTHON set, calling call_python_nextquestion())");
      fail = call_python_nextquestion(s, nq, action, affected_answers_count);

      if (fail) {
        LOG_ERRORV("call_python_nextquestion() failed with return code %d", fail);
      }

    } else if (s->nextquestions_flag & NEXTQUESTIONS_FLAG_GENERIC) {

      // PGS: Disabled generic implementation of nextquestion, since if you have a python version and it can't be loaded
      // for some reason we should NOT fall back, because it may expose questions and IP in a survey that should not be revealed.
      LOG_INFO("NEXTQUESTIONS_FLAG_GENERIC set, calling get_next_questions_generic())");
      fail = get_next_questions_generic(s, nq, action, affected_answers_count);

      if (fail) {
        LOG_ERRORV("get_next_questions_generic() failed with return code %d", fail);
      }

    } else {
      LOG_ERRORV("Could not identify nextquestion mode, Unknown next_questions_flag. %d", s->nextquestions_flag);
    }

    // #379 update state (finished)
    if (nq->question_count == 0) {
      // #408, nextquestion queries can pass now after a session was closed, prevent regression
      if (s->state < SESSION_FINISHED) {
        s->state = SESSION_FINISHED;
      }
      LOG_INFO("Set session state to SESSION_FINISHED");
    }

    // #379 update state (re-open finished session)
    if (nq->question_count > 0) {
      if (s->state == SESSION_FINISHED) {
        s->state = SESSION_OPEN;
        LOG_INFO("Set session state from SESSION_FINISHED to SESSION_OPEN");
      }
    }

  } while (0);

  if (retVal) {
    free_next_questions(nq);
    LOG_WARNV("get_next_questions() failed.", 0);
    return NULL;
  }



  return nq;
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
 * Note! output needs to be freed outside of this function
 */
int call_python_analysis(struct session *s, const char **output) {
  int retVal = 0;
  int is_error = 0;
  do {
    if (!s->survey_id) {
      LOG_ERROR("surveyname is NULL");
    }
    if (!s->session_id) {
      LOG_ERROR("session_uuid is NULL");
    }
    // Setup python
    if (setup_python()) {
      LOG_ERROR("Failed to initialise python.\n");
    }
    if (!py_module) {
      LOG_ERROR( "Python module 'nextquestion' not loaded. Does it have an error?");
    }

    // select from avaliable hook functions
    char function_name[1024];
    PyObject *myFunction = py_get_hook_function("analyse", s->survey_id, function_name, 1024);
    if (!myFunction) {
      LOG_ERROR("Failed to get hook function for 'analyse'");
    }

    // Okay, we have the function object, so build the argument list and call it.
    PyObject *result = py_invoke_hook_function(myFunction, s, ACTION_SESSION_ANALYSIS, 0);

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
        LOG_ERRORV("String in reply from Python function '%s' is null", function_name);
      }

      // #288, allocate dedicated memory, managed by backend and write string
      *output = strndup(return_string, (size_t)size);
      // TODO should issues occur then consider hard-setting a \0 token at the last position of *output
    } else {
      Py_DECREF(result);
      LOG_ERRORV("Return value from Python function '%s' is not a string.", function_name);
    }

    Py_DECREF(result);
    LOG_INFO("call python next question(s) finished.");
  } while (0);

  if (is_error) {
    retVal = -99;
  }
  return retVal;
}

/**
 * Get analysis
 * dispatcher function (generic or python)
 * #379 refactor
 */
int get_analysis(struct session *s, const char **output) {
  // Call the function to get the next question(s) to ask.
  // First see if we have a python function to do the job.
  // If not, then return the list of all not-yet-answered questions
  int retVal = 0;
  int fail = 0;
  do {
    if (!s) {
      LOG_ERROR("session structure is NULL");
    }

    if (s->nextquestions_flag & NEXTQUESTIONS_FLAG_PYTHON) {

      LOG_INFO("NEXTQUESTIONS_FLAG_PYTHON set, calling call_python_analysis())");
      fail = call_python_analysis(s, output);

      if (fail) {
        LOG_ERRORV("call_python_analysis() failed with return code %d", fail);
      }

    } else if (s->nextquestions_flag & NEXTQUESTIONS_FLAG_GENERIC) {

      // PGS: Disabled generic implementation of nextquestion, since if you have a python version and it can't be loaded
      // for some reason we should NOT fall back, because it may expose questions and IP in a survey that should not be revealed.
      LOG_INFO("NEXTQUESTIONS_FLAG_GENERIC set, calling get_analysis_generic())");
      fail = get_analysis_generic(s, output);

      if (fail) {
        LOG_ERRORV("get_analysis_generic() failed with return code %d", fail);
      }

    } else {
      LOG_ERRORV("Could not identify nextquestion mode, Unknown next_questions_flag. %d", s->nextquestions_flag);
    }

    // #379 update state (if not closed already) and save session
    if (s->state < SESSION_CLOSED) {
      s->state = SESSION_CLOSED;
      if (save_session(s)) {
        LOG_ERROR("save_session( on SESSION_CLOSED failed");
      }
    }

  } while (0);

  if (retVal) {
    LOG_WARNV("get_analysis() failed.", 0)
  }

  return retVal;
}
