
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
    if (!path_in) LOG_ERROR("path_in() is NULL","");
    if (!path_out) LOG_ERROR("path_out() is NULL","");
    if (max_len<128) LOG_ERROR("max_len passed to generate_path() is too small","");
    char *survey_home=getenv("SURVEY_HOME");
    if (!survey_home) LOG_ERROR("SURVEY_HOME environment variable not set","");

    int l=snprintf(path_out,max_len,"%s/%s",survey_home,path_in);
    if (l<1||l>=max_len) LOG_ERROR("snprintf() failed","");
  } while(0);
}

