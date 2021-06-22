#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "errorlog.h"
#include "survey.h"

/**
 * Make sure that a survey ID contains no disallowed characters.
 * We allow undderscore and space as well as dash and period, to allow some greater
 * freedom when specifying the symbolic name of a survey (form).  We naturally also
 * allow all upper and lower case latin characters, rather than just hexadecimal
 * characters.
 */
int validate_survey_id(char *survey_id) {
  int retVal = 0;
  do {
    if (!survey_id) {
      LOG_ERROR("survey_id is NULL");
    }
    if (!survey_id[0]) {
      LOG_ERROR("survey_id is empty string");
    }

    for (int i = 0; survey_id[i]; i++) {

      switch (survey_id[i]) {
      case ' ':
      case '.':
      case '-':
      case '_':
        break;

      default:
        if (!isalnum(survey_id[i])) {
          LOG_ERRORV("Illegal character 0x%02x in survey_id '%s'. Must be 0-9, a-z, space, period, comma, underscore", survey_id[i], survey_id);
        }
        break;
      } // endswitch

    } // endfor
  } while (0);
  return retVal;
}

/**
 * Verify that a session ID does not contain any illegal characters.
 * We allow only hex and the dash character.
 * The main objective is to disallow colons and slashes, to
 * prevent subverting the CSV file format or the formation of file names
 * and paths.
 */
int validate_session_id(char *session_id) {
  int retVal = 0;
  do {
    if (!session_id) {
      LOG_ERROR("session_id is NULL");
    }
    LOG_WARNV("Validating session id '%s'", session_id);
    if (strlen(session_id) != 36) {
      LOG_ERRORV("session_id '%s' must be exactly 36 characters long", session_id);
    }
    if (session_id[0] == '-') {
      LOG_ERRORV("session_id '%s' may not begin with a dash", session_id);
    }

    for (int i = 0; session_id[i]; i++) {

      switch (session_id[i]) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
      case '-':
        // Acceptable characters
        break;

      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F':
        LOG_ERRORV("session_id '%s' must be lower case", session_id);
        break;

      default:
        LOG_ERRORV( "Illegal character 0x%02x in session_id '%s'. Must be a valid UUID", session_id[i], session_id);
        break;
      } // endswitch

      if (retVal) {
        break;
      }

    } // endfor
  } while (0);
  return retVal;
}

/**
 * Verify that an answer exists in a given session and can be deleted
 */
int validate_session_delete_answer(char *question_id, struct session *ses) {
  int retVal = 0;
  do {

    if (!question_id) {
      LOG_ERROR("question_id is NULL");
    }

    if (!ses) {
      LOG_ERROR("session is NULL");
    }

    struct answer *ans = session_get_answer(question_id, ses);
    if (!ans) {
      LOG_ERRORV("Could not load answer '%s'", question_id);
    }

    if (!is_given_answer(ans)) {
      LOG_ERRORV("Answer '%s' is not a given answer", question_id);
    }

  } while (0);
  return retVal;
}

/**
 * Verify that an answer exists in a given session and can be deleted
 */
int validate_session_add_answer(struct session *ses, struct answer *ans, char *reason, size_t sz) {
  int retVal = 0;
  do {

    if (!ans) {
      LOG_ERROR("answer is NULL");
    }

    // validate exists
    struct question *qn = session_get_question(ans->uid, ses);
    if (!qn) {
      LOG_ERRORV("No question defined for answer '%s'", ans->uid);
    }

    switch(qn->type) {
      case QTYPE_UUID:
        if(validate_session_id(ans->text)) {
          LOG_ERRORV("not a valid uuid: '%s'\n", ans->text);
        }
      break;
    }

  } while (0);
  return retVal;
}
