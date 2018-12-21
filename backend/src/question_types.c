#include "question_types.h"
char *question_type_names[1+NUM_QUESTION_TYPES+1]={
  "start of list",
  "INT",
  "FIXEDPOINT",
  "CHECKBOX", // added in python controller, TODO naming this is misleading
  "MULTICHOICE",
  "MULTISELECT", // TODO this field seems equivalent to MULTICHOICE, remove?
  "LATLON",
  "DATETIME",
  "TIMERANGE",
  "UPLOAD",
  "TEXT",
  "UUID",
  "end of list"};
