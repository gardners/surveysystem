#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "errorlog.h"

/*
  Various functions for freeing data structures.
  freez() is the same as free(), but just checks to make sure that it hasn't been
  passed a null pointer.  This makes the latter functions a little simpler.
 */
void freez(void *p) {
  if (p) {
    free(p);
  }
  return;
}

/*
  Remove any trailing line feed or carriage returns from the input string.
 */
void trim_crlf(char *line) {
  if (!line) {
    return;
  }
  int len = strlen(line);

  while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
    line[--len] = 0;
  }
  return;
}

struct tm *format_time_ISO8601(time_t t, char *buf, size_t len) {
  struct tm *lt = localtime(&t);
  if (!lt) {
    return lt;
  }
  strftime(buf, len, "%FT%T%z", lt);
  // 2019-07-24T11:49:20+09:30
  return lt;
}


/**
 * non-destructive line parsing from a string
 * The returned char *line needs to be deallocated by the callee
 *
 * #461 deserialise a sequence of answers
 */
char *parse_line(const char *body, char separator, char **saveptr) {
  int len;
  char *str = (body) ? (char*)body : *saveptr;
  char *line;

  if(!str) {
    return NULL;
  }

  char *sep = strchr(str, separator);
  if (sep == NULL) {
      sep = strchr(str, '\0');
  }

  len = sep - str;
  if (!len) {
      return NULL;
  }

  line = malloc(len + 1);
  if (!line) {
    LOG_CODE(SS_ERROR_MEM, "Error allocating memory for parsing line");
    return NULL;
  }
  memmove (line, str, len);
  line[len] = '\0';

  if (*sep == '\0') {
    *saveptr = NULL; // prevent overflow on eventual next call
    return line;
  }

  *saveptr = sep + 1;
  return line;
}
