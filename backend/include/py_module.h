#ifndef __PY_H__
#define __PY_H__

#include <Python.h>
#include "survey.h"

#define PY_ERR_PRINT() do { if (PyErr_Occurred()) { PyErr_PrintEx(1); PyErr_Clear(); } } while(0)

int py_init();
int py_destroy();

void py_object_log(char *label, PyObject *obj, FILE *fp);
void py_log_error(FILE *fp);

int call_python_analysis(struct session *s, const char **output);
int call_python_nextquestion(struct session *s, struct nextquestions *nq, enum actions action, int affected_answers_count);
#endif
