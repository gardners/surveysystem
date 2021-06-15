#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errorlog.h"
#include "question_types.h"
#include "survey.h"
#include "serialisers.h"
#include "utils.h"
#include "py_module.h"

/**
 * flags whether a survey answer has been answered, excluding internal system answers
 *  - answer not deleted
 *  - answer not of QTYPE_META
 *  - answer not uid not pefixed with '@'
 */
int is_given_answer(struct answer *a) {
  if (!a) {
    LOG_WARNV("ERROR: is_given_answer() received NULL answer", 0);
    return 0;
  }

  if (a->flags & ANSWER_DELETED) {
    return 0;
  }

  if (a->type == QTYPE_META) {
    return 0;
  }

  if (a->uid[0] == '@') {
    return 0;
  }

  return 1;
}

/**
 * flags whether a survey answer is an internal system answer
 *  - answer is of QTYPE_META
 *  - answer uid is pefixed with '@'
 */
int is_system_answer(struct answer *a) {
  if (!a) {
    LOG_WARNV("ERROR: is_system_answer() received NULL answer", 0);
    return 1;
  }

  if (a->type == QTYPE_META) {
    return 1;
  }

  if (a->uid[0] == '@') {
    return 1;
  }

  return 0;
}

int dump_next_questions(FILE *f, struct nextquestions *nq) {
  int retVal = 0;
  int i;
  do {
    if (!f) {
      LOG_ERROR("dump_next_questions(): invalid file pointer.");
    }

    fprintf(f, "{\n");
    if (!nq) {
      fprintf(f, "nextquestions { <NULL> }\n");
      break;
    }

    fprintf(
      f,
      "nextquestions {\n"
      "  status: %d\n"
      "  message: %s\n"
      "  question_count: %d\n"
      "  progress: [%d, %d]\n"
      "  questions: [\n",
      nq->status,
      nq->message,
      nq->question_count,
      // #13 add suport for progress indicator
      nq->progress[0],
      nq->progress[1]
    );

    for (i = 0; i < nq->question_count; i++) {
      fprintf(f, "    %s%s\n", nq->next_questions[i]->uid, (i < nq->question_count - 1) ? ",": "");
    }

    fprintf(f , "  ]\n}\n");
  } while (0);

  return retVal;
}

// #332 free nextquestions data struct
void free_next_questions(struct nextquestions *nq) {
  if (!nq) {
    return;
  }
  freez(nq->message);
  for (int i = 0; i < nq->question_count; i++) {
    free_question(nq->next_questions[i]);
  }
  nq->question_count = 0;
  // #13 add suport for progress indicator
  nq->progress[0] = 0;
  nq->progress[1] = 0;
  free(nq);
  return;
}

/**
 * Adds a question to a netxtquestion struct and updates session
 * #462 delete existing answers
 * #213 handle default values for deleted answers
 */
int add_next_question(enum actions action, struct question *qn, struct nextquestions *nq, struct session *ses) {
  int retVal = 0;
  struct question *copy = NULL;

  do {
    if (!ses) {
      LOG_ERROR("struct session is NULL");
    }
    if (!nq) {
      LOG_ERROR("nextquestions is NULL");
    }
    if (!qn) {
      LOG_ERROR("question is NULL");
    }

    // #373 separate allocated space for questions
    struct answer *exists = session_get_answer(qn->uid, ses);
    if (exists) {

      if (action == ACTION_SESSION_DELETEANSWER) {
        // #462 if SESSION_DELETEANSWER: delete answer for next question and all following answers
        if (session_delete_answer(ses, qn->uid) < 0) {
          LOG_ERRORV("add_next_question(): Deleting existing answer for next question '%s' failed", qn->uid);
        }
      }

      // #213 default value if answer exists and is deleted
      // # 237 previously deleted sha answers: don't supply default value based on previous answer
      char default_value[8192] = { 0 };

      if (exists->flags & ANSWER_DELETED) {
        if (qn->type != QTYPE_SHA1_HASH) {
          if (answer_get_value_raw(exists, default_value, 8192)) {
            LOG_ERRORV("add_next_question(): Failed to fetch default value from previously deleted answer to next question '%s'", qn->uid);
          }
        }
      }

      copy = copy_question(qn, default_value);
      if (!copy) {
        LOG_ERRORV("add_next_question(): Copying next question '%s' in list of next questions failed", qn->uid);
      }

    } else {
      copy = copy_question(qn, NULL);
      if (!copy) {
        LOG_ERRORV("add_next_question(): Copying next question '%s' in list of next questions failed", qn->uid);
      }
    }
    nq->next_questions[nq->question_count] = copy;
    nq->question_count++;

  } while (0);

  return retVal;
}

/**
 * Generic next question selector, which selects the first question lacking an answer.
 * #332 next_questions data struct
 * #445 add action, affected_answers_count args (to be used in later development)
 */
int get_next_questions_generic(struct session *ses, struct nextquestions *nq, enum actions action, int affected_answers_count) {
  int retVal = 0;

  do {
    if (!ses) {
      LOG_ERROR("struct session is NULL");
    }
    if (!ses->survey_id) {
      LOG_ERROR("surveyname is NULL");
    }
    if (!ses->session_id) {
      LOG_ERROR("session_uuid is NULL");
    }
    if (!nq) {
      LOG_ERROR("nextquestions is NULL");
    }
    if (nq->question_count) {
      LOG_ERROR("nextquestions->question_count is > 0");
    }

    LOG_INFO("Calling get_next_questions_generic()");

    int index = 0;
    struct answer *ans = session_get_last_given_answer(ses);
    if (ans) {
      index = session_get_question_index(ans->uid, ses);
      // last question
      if (index >= ses->question_count - 1) {
        nq->progress[0] = ses->question_count;
        nq->progress[1] = ses->question_count;
        break;
      }
      // next question
      index++;
    }

    if (add_next_question(action, ses->questions[index], nq, ses)) {
      LOG_ERRORV("Error adding question '%s' to list of next questions", ses->questions[index]->uid);
    }

    // #13 add suport for progress indicator, in generic (linear) mode we just copy the session counters
    nq->progress[0] = index;
    nq->progress[1] = ses->question_count;

  } while (0);

  return retVal;
}

/**
 * Get next questions
 * dispatcher function (generic or python)
 * #332 nextquestions data struct
 * #379 refactor
 */
struct nextquestions *get_next_questions(struct session *s, enum actions action, int affected_answers_count) {
  // Call the function to get the next question(s) to ask.
  // First see if we have a python function to do the job.
  // If not, then return the list of all not-yet-answered questions
  int retVal = 0;
  int fail = 0;
  struct nextquestions *nq = NULL;

  do {
    if (!s) {
      LOG_ERROR("session structure is NULL");
    }

    nq = calloc(sizeof(struct nextquestions), 1);
    if (!nq) {
      LOG_ERROR("init_next_questions() failed");
    }

    if (s->nextquestions_flag & NEXTQUESTIONS_FLAG_PYTHON) {

      LOG_INFO("NEXTQUESTIONS_FLAG_PYTHON set, calling get_next_question_python()");
      fail = get_next_question_python(s, nq, action, affected_answers_count);

      if (fail) {
        LOG_ERRORV("get_next_question_python() failed with return code %d", fail);
      }

    } else if (s->nextquestions_flag & NEXTQUESTIONS_FLAG_GENERIC) {

      // PGS: Disabled generic implementation of nextquestion, since if you have a python version and it can't be loaded
      // for some reason we should NOT fall back, because it may expose questions and IP in a survey that should not be revealed.
      LOG_INFO("NEXTQUESTIONS_FLAG_GENERIC set, calling get_next_questions_generic()");
      fail = get_next_questions_generic(s, nq, action, affected_answers_count);

      if (fail) {
        LOG_ERRORV("get_next_questions_generic() failed with return code %d", fail);
      }

    } else {
      LOG_ERRORV("Could not identify nextquestion mode, session contains unknown next_questions_flag. %d", s->nextquestions_flag);
    }

    // #379 update state (finished)
    if (nq->question_count == 0) {
      // #408, nextquestion queries can pass now after a session was closed, prevent regression
      if (s->state < SESSION_FINISHED) {
        s->state = SESSION_FINISHED;
      }
      LOG_INFO("Set session state to SESSION_FINISHED");
    }

    // #379 update state (re-open finished session)
    if (nq->question_count > 0) {
      if (s->state == SESSION_FINISHED) {
        s->state = SESSION_OPEN;
        LOG_INFO("Set session state from SESSION_FINISHED to SESSION_OPEN");
      }
    }

  } while (0);

  if (retVal) {
    free_next_questions(nq);
    LOG_WARNV("get_next_questions() failed.", 0);
    return NULL;
  }

  return nq;
}

/*
 * #300 prepare analysis string response
 * TODO (not yet implemeneted in main.c)
 * **output needs to be freed outside of this function
 */
int get_analysis_generic(struct session *s, const char **output) {
  int retVal = 0;

  do {
    *output = strdup("\"NOT IMPLEMENTED\""); // write something (valid json)
  } while (0);

  return retVal;
}

/**
 * Get analysis
 * dispatcher function (generic or python)
 * #379 refactor
 */
int get_analysis(struct session *s, const char **output) {
  // Call the function to get the next question(s) to ask.
  // First see if we have a python function to do the job.
  // If not, then return the list of all not-yet-answered questions
  int retVal = 0;
  int fail = 0;
  do {
    if (!s) {
      LOG_ERROR("session structure is NULL");
    }

    if (s->nextquestions_flag & NEXTQUESTIONS_FLAG_PYTHON) {

      LOG_INFO("NEXTQUESTIONS_FLAG_PYTHON set, calling get_analysis_python()");
      fail = get_analysis_python(s, output);

      if (fail) {
        LOG_ERRORV("get_analysis_python() failed with return code %d", fail);
      }

    } else if (s->nextquestions_flag & NEXTQUESTIONS_FLAG_GENERIC) {

      // PGS: Disabled generic implementation of nextquestion, since if you have a python version and it can't be loaded
      // for some reason we should NOT fall back, because it may expose questions and IP in a survey that should not be revealed.
      LOG_INFO("NEXTQUESTIONS_FLAG_GENERIC set, calling get_analysis_generic())");
      fail = get_analysis_generic(s, output);

      if (fail) {
        LOG_ERRORV("get_analysis_generic() failed with return code %d", fail);
      }

    } else {
      LOG_ERRORV("Could not identify nextquestion mode, Unknown next_questions_flag. %d", s->nextquestions_flag);
    }

    // #379 update state (if not closed already) and save session
    if (s->state < SESSION_CLOSED) {
      s->state = SESSION_CLOSED;
      if (save_session(s)) {
        LOG_ERROR("save_session( on SESSION_CLOSED failed");
      }
    }

  } while (0);

  if (retVal) {
    LOG_WARNV("get_analysis() failed.", 0)
  }

  return retVal;
}
