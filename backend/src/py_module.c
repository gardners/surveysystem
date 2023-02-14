#include <stdio.h>
#include <stdlib.h>
#include <Python.h>

#include "errorlog.h"
#include "py_module.h"

// py module instance (kept persistent for performance resons)
PyObject *py_module = NULL;
PyObject *py_globals = NULL;
PyObject *py_func_traceback = NULL;

int force_restart = 0;// #361 force re-initalisation. use this only for tests!
char py_module_path[1024] = "";

// forward declaration
static int py_module_init();

/**
 * logs a PyObject, using either the logger or into a custom steam
 */
void py_object_log(char *label, PyObject *obj, FILE *fp) {
    PyObject* repr = PyObject_Repr(obj);
    PyObject* str = PyUnicode_AsEncodedString(repr, "utf-8", "~E~");
    const char *bytes = PyBytes_AS_STRING(str);

    if (fp) {
        fprintf(fp, "'%s':\n'%s'\n", label, bytes);
    } else {
        LOG_INFOV("'%s':\n'%s'", label, bytes);
    }

    Py_XDECREF(repr);
    Py_XDECREF(str);
}

/**
 * Logs and clear a python error
 *
 * Fetch a printable traceback string via an optional python callback.
 * There seems (Python 3.8) to be no convenient way to serialise a traceback instance this via the api.
 */
void py_log_error(FILE *fp) {

  if(!PyErr_Occurred()) {
    if (fp) {
        fprintf(fp, "Python error loggging requested, but PyErr_Occurred() returned NULL\n");
    } else {
        LOG_INFO("Python error loggging requested, but PyErr_Occurred() returned NULL");
    }
    return;
  }

  PyObject *type = NULL;
  PyObject *value = NULL;
  PyObject *traceback = NULL;

  PyErr_Fetch(&type, &value, &traceback);
  PyErr_NormalizeException(&type, &value, &traceback);
  if (traceback) {
    PyException_SetTraceback(value, traceback);
  }
  //traceback.format_exc()
  py_object_log("type:", type, fp);
  py_object_log("value:", value, fp);

  if (py_func_traceback) {
    PyObject *tb_args = NULL;
    PyObject *tb_str = NULL;

    tb_args = PyTuple_New(3);
    PyTuple_SetItem(tb_args, 0, type);
    PyTuple_SetItem(tb_args, 1, value);
    PyTuple_SetItem(tb_args, 2, traceback);

    tb_str = PyObject_Call(py_func_traceback, tb_args, NULL);
    py_object_log("traceback:", tb_str, fp);

    Py_XDECREF(tb_args);
    Py_XDECREF(tb_str);
  }

  Py_XDECREF(type);
  Py_XDECREF(value);
  Py_XDECREF(traceback);

  PyErr_Clear();
}

/**
 * Dereference Python modules and unload Python
 */
int py_destroy() {
  int retVal = 0;

  do {
    Py_XDECREF(py_module);
    Py_XDECREF(py_globals);
    Py_XDECREF(py_func_traceback);

    py_module = NULL;
    py_globals = NULL;
    py_func_traceback = NULL;

    if (Py_FinalizeEx()) {
        BREAK_ERROR("Py_FinalizeEx() FAILED. Memory has been leaked!");
    }
  } while(0);

  return retVal;
}

/**
 * Initilaise python and load custom modules
 */
int py_init() {
    // #361 force re-initalisation. use this only for tests!
    char *force_init = getenv("SURVEY_FORCE_PYINIT");
    if (force_init) {
        force_restart = atoi(force_init);
        if (force_restart) {
            LOG_INFO(" => env 'SURVEY_FORCE_PYINIT' was set to forcibly restart python");
        }
    }
    return py_module_init();
}


static int py_module_init() {
  int retVal =0;

  PyObject *syspath = NULL;
  PyObject *fromList = NULL;
  PyObject *check = NULL;

  do {

    if (force_restart) {
        LOG_INFO("=> restarting python");
        py_destroy();
    }

    if (py_module) {
      break;
    }

    LOG_INFO(" => starting python init");
    wchar_t *program = Py_DecodeLocale("nextquestion", NULL);
    // program need?
    if (!program) {
      BREAK_ERROR("cannot decode program name to wchar_t");
    }
    Py_SetProgramName(program);
    PyMem_RawFree(program);

    Py_Initialize();

    // main
    py_module = PyImport_AddModule("__main__");
    if (!py_module) {
      BREAK_ERROR("cannot add main module");
    }

    py_globals = PyModule_GetDict(py_module);

    if (!strlen(py_module_path)) {
      if (generate_python_path(py_module_path, 1024)) {
        BREAK_ERROR("Failed to generate python search path");
      }
    }

    char source[2048];
    snprintf(source, 2048,
      "import sys\n"
      "sys.path.append('%s')\n"
      "\n"
      "import traceback\n"
      "import nextquestion as nq\n"
      "\n"
      "def get_traceback(exc_type, exc_value, exc_tb):\n"
      "    lines = traceback.format_exception(exc_type, exc_value, exc_tb)\n"
      "    output = '\\n'.join(lines)\n"
      "    return output\n",
      py_module_path);

    check = PyRun_String(
      source,
      Py_file_input,
      py_globals,
      py_globals
    );

    if(!check) {
        py_log_error(NULL);
        BREAK_ERROR("initialising 'nextquestions' failed");
    }

    py_func_traceback = PyObject_GetAttrString(py_module, "get_traceback");
    if(!py_func_traceback) {
        py_log_error(NULL);
        BREAK_ERROR("retrieving function get_traceback() from 'nextquestions' failed");
    }
  } while(0);

  Py_XDECREF(syspath);
  Py_XDECREF(fromList);
  Py_XDECREF(check);

  if (retVal) {
    py_destroy();
  }

  return retVal;
}

