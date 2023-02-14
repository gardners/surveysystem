
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>


#include "errorlog.h"
#include "question_types.h"
#include "survey.h"

int generate_path(char *path_in, char *path_out, int max_len) {
  int retVal = 0;

  do {
    if (!path_in) {
      BREAK_ERROR("path_in() is NULL");
    }
    if (!path_out) {
      BREAK_ERROR("path_out() is NULL");
    }
    if (max_len < 128) {
      BREAK_ERROR("max_len passed to generate_path() is too small");
    }
    char *survey_home = getenv("SURVEY_HOME");
    if (!survey_home) {
      BREAK_ERROR("SURVEY_HOME environment variable not set");
    }

    int l = snprintf(path_out, max_len, "%s/%s", survey_home, path_in);
    if (l < 1 || l >= max_len) {
      BREAK_ERROR("snprintf() failed");
    }
  } while (0);
  return retVal;
}

int generate_session_path(char *session_id, char *filename, char *path_out, int max_len) {
  int retVal = 0;
  char prefix[5];
  int r;

  do {
    if (!session_id) {
      BREAK_ERROR("session_id is NULL");
    }
    if (!path_out) {
      BREAK_ERROR("path_out() is NULL");
    }
    if (max_len < 128) {
      BREAK_ERROR("max_len is too small");
    }

    char *survey_home = getenv("SURVEY_HOME");
    if (!survey_home) {
      BREAK_ERROR("SURVEY_HOME environment variable not set");
    }

    for (int i = 0; i < 4; i++) {
      prefix[i] = session_id[i];
    }
    prefix[4] = 0;

    if (filename == NULL) {
      // dir path
      r = snprintf(path_out, max_len, "%s/sessions/%s", survey_home, prefix);
    } else {
      // file path
      r = snprintf(path_out, max_len, "%s/sessions/%s/%s", survey_home, prefix, filename);
    }

    if (r < 1 || r >= max_len) {
      BREAK_ERROR("snprintf() failed");
    }

  } while (0);
  return retVal;
}

int generate_survey_path(char *survey_id, char *filename, char *path_out, int max_len) {
  int retVal = 0;
  int r;

  do {
    if (!survey_id) {
      BREAK_ERROR("survey_id is NULL");
    }
    if (!path_out) {
      BREAK_ERROR("path_out() is NULL");
    }
    if (max_len < 128) {
      BREAK_ERROR("max_len is too small");
    }

    char *survey_home = getenv("SURVEY_HOME");
    if (!survey_home) {
      BREAK_ERROR("SURVEY_HOME environment variable not set");
    }

    if (filename == NULL) {
      // dir path
      r = snprintf(path_out, max_len, "%s/surveys/%s", survey_home, survey_id);
    } else {
      // file path
      r = snprintf(path_out, max_len, "%s/surveys/%s/%s", survey_home, survey_id, filename);
    }

    if (r < 1 || r >= max_len) {
      BREAK_ERROR("snprintf() failed");
    }

  } while (0);
  return retVal;
}

/**
 * Generates path to python controller based on ENV
 * If env "SURVEY_PYTHONDIR" is set, use that, otherwise use "SURVEY_HOME/python"
 */
int generate_python_path(char *path_out, int max_len) {
  int retVal = 0;

  do {
    if (!path_out) {
      BREAK_ERROR("path_out() is NULL");
    }
    if (max_len < 128) {
      BREAK_ERROR("max_len is too small");
    }

    char *survey_pythondir = getenv("SURVEY_PYTHONDIR");
    // if no SURVEY_PYTHONDIR defined, use python folder within SURVEY_HOME
    if (!survey_pythondir) {

      LOG_INFO("ENV 'SURVEY_PYTHONDIR' not defined, try using "
               "'<SURVEY_HOME>/python' instead.");
      char *survey_home = getenv("SURVEY_HOME");
      if (!survey_home) {
        BREAK_ERROR("SURVEY_HOME environment variable not set");
      }

      int l = snprintf(path_out, max_len, "%s/%s", survey_home, "python");
      if (l < 1 || l >= max_len) {
        BREAK_ERROR("snprintf() failed");
      }

    } else {

      LOG_INFOV("ENV 'SURVEY_PYTHONDIR' found: '%s'", survey_pythondir);
      int m = snprintf(path_out, max_len, "%s", survey_pythondir);
      if (m < 1 || m >= max_len) {
        BREAK_ERROR("snprintf() failed");
      }
    }

  } while (0);
  return retVal;
}

