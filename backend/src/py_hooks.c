#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Python.h>

#include "errorlog.h"
#include "question_types.h"
#include "survey.h"
#include "serialisers.h"
#include "utils.h"
#include "py_module.h"

// TODO remove
extern PyObject *py_module;

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
static PyObject *py_get_hook_function(char *base_function, char *survey_id, char *function_name, size_t len) {
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

    PyObject *parent = PyObject_GetAttrString(py_module, "nq");
    if (!parent) {
        LOG_ERROR("cannot fetch parent reference 'nq' from Python module");
    }

    // 1. <base_function>_<survey_id>_<survey_hash>()

    snprintf(function_name, len, "%s_%s", base_function, survey_id);
    for (int i = 0; function_name[i]; i++) {
        if (function_name[i] == '/') {
          function_name[i] = '_';
        }
    }
    LOG_INFOV("Searching for python function '%s'", function_name);
    func = PyObject_GetAttrString(parent, function_name);

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
        func = PyObject_GetAttrString(parent, function_name);
    }

    // 2. <base_function>()

    if (!func) {
        snprintf(function_name, len, "%s", base_function);
        LOG_INFOV("Searching for python function '%s'", function_name);
        func = PyObject_GetAttrString(parent, function_name);
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
static PyObject *py_create_answer(struct answer *a) {
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
static PyObject *py_create_questions_list(struct session *ses) {
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
static int py_answer_merge_question(PyObject *ans, struct question *qn) {
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
static PyObject *py_create_answers_list(struct session *ses) {
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
static PyObject *py_invoke_hook_function(PyObject *function_reference, char *function_name, struct session *ses, enum actions action, int affected_answers_count) {
  int retVal = 0;

  PyObject *kwargs = NULL;
  PyObject *questions = NULL;
  PyObject *answers = NULL;

  PyObject *result = NULL;

  do {
    if (!ses) {
      LOG_ERROR("session is null");
    }
    if (!ses->survey_id) {
      LOG_ERROR("survey id is NULL");
    }
    if (!ses->session_id) {
      LOG_ERROR("session id is NULL");
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

    result = PyObject_Call(function_reference, args, kwargs);
    Py_XDECREF(args);
    Py_XDECREF(kwargs);

    if (PyErr_Occurred()) {
      py_log_error(NULL);
      LOG_ERRORV("Python function '%s' exited with an Error (PyErr_Occurred()). Check Python error message above", function_name);
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
static int py_nextquestions_handle_progress(PyObject *result, struct nextquestions *nq) {
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
int get_next_question_python(struct session *ses, struct nextquestions *nq, enum actions action, int affected_answers_count) {
  int retVal = 0;
  int is_error = 0;
  PyObject *result = NULL;

  do {
    if (!ses) {
      LOG_ERROR("session is null");
    }
    if (!ses->survey_id) {
      LOG_ERROR("survey id is NULL");
    }
    if (!ses->session_id) {
      LOG_ERROR("session id is NULL");
    }
    if (!nq) {
      LOG_ERROR("nextquestions is NULL");
    }
    if (nq->question_count) {
      LOG_ERROR("nextquestions->question_count is > 0");
    }
    // int python
    // #TODO #459 move to bootstrap, check py_module
    if (py_init()) {
      LOG_ERROR("Failed to initialise python.\n");
    }
    if (!py_module) {
      LOG_ERROR( "Python module 'nextquestion' not loaded. Does it have an error?");
    }

    // select from avaliable hook functions
    char function_name[1024];
    PyObject *function_reference = py_get_hook_function("nextquestion", ses->survey_id, function_name, 1024);
    if (!function_reference) {
      LOG_ERROR("Failed to get hook function for 'nextquestion'");
    }

    // Okay, we have the function object, so build the argument list and call it.
    result = py_invoke_hook_function(function_reference, function_name, ses, action, affected_answers_count);

    if (!result) {
      LOG_ERRORV("Python function '%s' did not return anything. Check Python error message above", function_name);
    }

    // #332 add instance check for exported Python class 'NextQuestions'

    if(!PyDict_Check(result)) {
      LOG_ERRORV("Reply from Python function '%s' is of invalid type (not a dict)", function_name);
    }

    // 1. extract status
    PyObject *py_status = PyDict_GetItemString(result, "status");
    if (!py_status) {
      LOG_ERRORV("Reply from Python function '%s' has no member 'status'", function_name);
    }

    long int status = PyLong_AsLong(py_status);
    if(status == -1) {
      LOG_ERRORV("Reply from Python function '%s': invalid member 'status' (int)", function_name);
    }

    // 2. assign status
    nq->status = status;

    // 3. extract message
    PyObject *py_message = PyDict_GetItemString(result, "message");
    if (!py_message) {
      LOG_ERRORV("Reply from Python function '%s' has no member 'message'", function_name);
    }

    const char *message = PyUnicode_AsUTF8(py_message);
    if(!message) {
      LOG_ERRORV("Reply from Python function '%s': invalid member 'message' (str)", function_name);
    }

    // 4. assign message
    nq->message = strdup(message);

    // 5. progress values
    if (py_nextquestions_handle_progress(result, nq)) {
      LOG_ERRORV("Reply from Python function '%s': progess handler failed", function_name);
    }

    // 6. extract nextquestions
    PyObject *py_next_questions = PyDict_GetItemString(result, "next_questions");
    if (!py_next_questions) {
      LOG_ERRORV("Reply from Python function '%s' has no member 'next_questions'", function_name);
    }

    if(!PyList_Check(py_next_questions)) {
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
          LOG_ERRORV("String in reply from Python function '%s' is null", function_name);
        }

        // 6. assign next_questions
        // 7. assign question_count
        struct question *qn = session_get_question((char *)uid, ses);
        if (!qn) {
          is_error = 1;
          LOG_ERRORV("Error adding question '%s' to list of next questions, question does not exist.", uid);
        }

        if (add_next_question(action, qn, nq, ses)) {
          is_error = 1;
          LOG_ERRORV("Error adding question '%s' to list of next questions", uid);
        }

      } else {
        is_error = 1;
        LOG_ERRORV("result.next_questions[%d] item is not a string in response from Python function '%s'", i, function_name);
      }

    } // endfor

    LOG_INFO("call python next question(s) finished.");

  } while (0);

  Py_XDECREF(result);
  if (is_error) {
    retVal = -99;
  }

  return retVal;
}

/*
 * Fetch analysis json string via Python script
 * #288, the parent unit is responsible for freeing *output pointer
 * Note! output needs to be freed outside of this function
 */
int get_analysis_python(struct session *ses, const char **output) {
  int retVal = 0;
  PyObject *result = NULL;

  do {
    if (!ses) {
      LOG_ERROR("session is null");
    }
    if (!ses->survey_id) {
      LOG_ERROR("survey id is NULL");
    }
    if (!ses->session_id) {
      LOG_ERROR("session id is NULL");
    }
    // int python
    // #TODO #459 move to bootstrap, check py_module
    if (py_init()) {
      LOG_ERROR("Failed to initialise python.\n");
    }
    if (!py_module) {
      LOG_ERROR( "Python module 'nextquestion' not loaded. Does it have an error?");
    }

    // select from avaliable hook functions
    char function_name[1024];
    PyObject *function_reference = py_get_hook_function("analyse", ses->survey_id, function_name, 1024);
    if (!function_reference) {
      LOG_ERROR("Failed to get hook function for 'analyse'");
    }

    // Okay, we have the function object, so build the argument list and call it.
    result = py_invoke_hook_function(function_reference, function_name, ses, ACTION_SESSION_ANALYSIS, 0);

    if (!result) {
      py_log_error(NULL);
      LOG_ERRORV("Python function '%s' did not return anything. Check Python error message above", function_name);
    }

    // PyObject_Print(result,stderr,0);
    if (PyUnicode_Check(result)) {

      // Get value and put it as single response
      // #288, replaced PyUnicode_AsUTF8 with PyUnicode_AsUTF8AndSize
      Py_ssize_t size;
      const char *return_string = PyUnicode_AsUTF8AndSize(result, &size);

      if (!return_string) {
        LOG_ERRORV("String in reply from Python function '%s' is null", function_name);
      }

      // #288, allocate dedicated memory, managed by backend and write string
      *output = strndup(return_string, (size_t)size);
      // TODO should issues occur then consider hard-setting a \0 token at the last position of *output
    } else {
      LOG_ERRORV("Return value from Python function '%s' is not a string.", function_name);
    }

    LOG_INFO("call python next question(s) finished.");
  } while (0);

  Py_XDECREF(result);
  return retVal;
}
