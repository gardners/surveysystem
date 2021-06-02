#ifndef __VALIDATORS_H__
#define __VALIDATORS_H__

#include "survey.h"

#define MISMATCH_IS_NOT_AN_ERROR 0
#define MISMATCH_IS_AN_ERROR 1

int validate_survey_id(char *survey_id);
int validate_session_id(char *session_id);

int validate_session_delete_answer(char *question_id, struct session *ses);
#endif
