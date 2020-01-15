
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "survey.h"
#include "errorlog.h"
#include "question_types.h"

int generate_path(char *path_in,char *path_out,int max_len)
{
  int retVal=0;

  do {
    if (!path_in) LOG_ERROR("path_in() is NULL");
    if (!path_out) LOG_ERROR("path_out() is NULL");
    if (max_len<128) LOG_ERROR("max_len passed to generate_path() is too small");
    char *survey_home=getenv("SURVEY_HOME");
    if (!survey_home) LOG_ERROR("SURVEY_HOME environment variable not set");

    int l=snprintf(path_out,max_len,"%s/%s",survey_home,path_in);
    if (l<1||l>=max_len) LOG_ERROR("snprintf() failed");
  } while(0);
  return retVal;
}

/**
 * Generates path to python controller based on ENV
 * If env "SURVEY_PYTHONDIR" is set, use that, otherwise use "SURVEY_HOME/python"
 */
int generate_python_path(char *path_out, int max_len)
{
  int retVal = 0;

  do {
    if (!path_out) LOG_ERROR("path_out() is NULL");
    if (max_len < 128) LOG_ERROR("max_len passed to generate_path() is too small");
    
    char *survey_pythondir = getenv("SURVEY_PYTHONDIR");
    // if no SURVEY_PYTHONDIR defined, use python folder within SURVEY_HOME
    if (!survey_pythondir) {
      LOG_INFOV("#### getenv no HOME_PY", survey_pythondir, path_out);
      char *survey_home = getenv("SURVEY_HOME");
      if (!survey_home) LOG_ERROR("SURVEY_HOME environment variable not set");
      
      int l = snprintf(path_out, max_len, "%s/%s", survey_home, "python");
      if (l < 1 || l >= max_len) LOG_ERROR("snprintf() failed");
    } else {
      LOG_INFOV("#### getenv  HAS HOME_PY", survey_pythondir, path_out);
      int m = snprintf(path_out, max_len, "%s", survey_pythondir);
      if (m < 1 || m >= max_len) LOG_ERROR("snprintf() failed");
    }
    LOG_INFOV("#### getenv(\"SURVEY_PYTHONDIR\"): \"%s\", path_out: \"%s\"", survey_pythondir, path_out);
  } while(0);
  return retVal;
}

