#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "errorlog.h"
#include "serialisers.h"
#include "survey.h"

void usage(void) {
  fprintf(
      stderr,
      "usage: survey newsession <survey name> -- create a new session\n"
      "       survey addanswer <sessionid> <serialised answer> -- add an "
      "answer to an existing session\n"
      "       survey nextquestion <sessionid> -- get the next question that "
      "should be asked\n"
      "       survey delanswer <sessionid> <question id> -- delete an answer "
      "from an existing session\n"
      "       survey delsession <sessionid> -- delete an existing session\n");
};

void init(int argc, char **argv) {
   if (!getenv("SURVEY_HOME") ) {
      setenv("SURVEY_HOME", ".", 1);
      LOG_INFO("env 'SURVEY_HOME' not found set to '.'");
    }

    if (!getenv("SURVEY_PYTHONDIR") ) {
      setenv("SURVEY_PYTHONDIR", "./python", 1);
      LOG_INFO("env 'SURVEY_PYTHONDIR' not found set to './python'");
    }

    char test[1024];
    char *env;


    // test log dir
    env = getenv("SURVEY_HOME");
    snprintf(test, 1024 ,"%s/logs", (env) ? env: "");
    if (access(test, W_OK)) {
      fprintf(stderr, "Cannot access log dir '%s' (W_OK). Does it exist?\n", test);
      exit(-1);
    }

    // test sessions dir
    snprintf(test, 1024 ,"%s/sessions", (env) ? env: "");
    if (access(test, W_OK)) {
      fprintf(stderr, "Cannot access session dir '%s' (W_OK). Does it exist?\n", test);
      exit(-1);
    }

    // test surveys dir
    snprintf(test, 1024 ,"%s/surveys", (env) ? env: "");
    if (access(test, W_OK)) {
      fprintf(stderr, "Cannot access survey dir '%s' (W_OK). Does it exist?\n", test);
      exit(-1);
    }

    // test python dir
    env = getenv("SURVEY_PYTHONDIR");
    snprintf(test, 1024 ,"%s", (env) ? env: "");
    if (access(test, R_OK)) {
      fprintf(stderr, "Cannot access python dir '%s' (R_OK). Does it exist?\n", test);
      exit(-1);
    }
}

int do_newsession(char *survey_id) {
  int retVal = 0;

  do {
    LOG_INFO("Entering newsession handler.");
    struct session_meta meta = {
      .user = NULL,
      .group = NULL,
      .authority = NULL,
      .provider = IDENDITY_CLI,
    };

    // #239, create new session id (separated out)
    char session_id[256];
    if(create_session_id(session_id, 256)) {
      LOG_ERROR("Create session id failed.");
    }

    if (create_session(survey_id, session_id, &meta)) {
      fprintf(stderr, "failed to create session.\n");
      LOG_ERROR("Create session failed.");
    }

    printf("%s\n", session_id);
    LOG_INFO("Leaving newsession handler.");
  } while(0);

  return retVal;
}

int do_nextquestions(char *session_id) {
  int retVal = 0;

  struct session *ses = NULL;
  struct nextquestions *nq = NULL;

  do {
    LOG_INFO("Entering nextquestion handler.");

    ses = load_session(session_id);
    if (!ses) {
      fprintf(stderr, "Could not load specified session. Does it exist?\n");
      LOG_ERROR("Could not load session");
    }

    char reason[1024];
    if (validate_session_action(ACTION_SESSION_NEXTQUESTIONS, ses, reason, 1024)) {
      free_session(ses);
      ses = NULL;
      fprintf(stderr, "%s\n", reason);
      LOG_ERROR("Session action validation failed");
    }

    nq = get_next_questions(ses);
    if (!nq) {
      free_session(ses);
      ses = NULL;
      fprintf(stderr, "Could not load next questions.\n");
      LOG_ERROR("get_next_questions() failed");
    }

    if (save_session(ses)) {
      free_session(ses);
      ses = NULL;
      free_next_questions(nq);
      nq = NULL;
      fprintf(stderr, "Unable to update session.\n");
      LOG_ERROR("save_session() failed");
    }

    // return adjacent next question only
    printf("%s\n", (nq->question_count) ? nq->next_questions[0]->uid : "");

    free_session(ses);
    free_next_questions(nq);
    LOG_INFO("Leaving nextquestion handler.");
  } while (0);

  return retVal;
}

int do_addanswer(char *session_id, char *serialised_answer) {
  int retVal = 0;

  struct session *ses = NULL;
  struct answer *ans = NULL;
  struct nextquestions *nq = NULL;

  do {
    LOG_INFO("Entering addanswer handler.");

    ses = load_session(session_id);
    if (!ses) {
      fprintf(stderr, "Could not load specified session. Does it exist?");
      LOG_ERROR("Could not load session");
    }

    char reason[1024];
    if (validate_session_action(ACTION_SESSION_ADDANSWER, ses, reason, 1024)) {
      free_session(ses);
      ses = NULL;
      fprintf(stderr, "%s\n", reason);
      LOG_ERROR("Session action validation failed");
    }

    // Deserialise answer
    ans = calloc(sizeof(struct answer), 1);
    if (!ans) {
      fprintf(stderr, "calloc() of answer structure failed.\n");
      LOG_ERROR("calloc() of answer structure failed.");
    }

    if (deserialise_answer(serialised_answer, ANSWER_SCOPE_PUBLIC, ans)) {
      free_answer(ans);
      fprintf(stderr, "answer format is invalid.\n");
      LOG_ERROR("deserialise_answer() failed.");
    }

    if (session_add_answer(ses, ans)) {
      free_session(ses);
      ses = NULL;
      free_answer(ans);
      ans = NULL;
      fprintf(stderr, "could not add answer to session.\n");
      LOG_ERROR("session_add_answer() failed.");
    }

    free_answer(ans);
    ans = NULL;

    nq = get_next_questions(ses);
    if (!nq) {
      free_session(ses);
      ses = NULL;
      fprintf(stderr, "Could not load next questions.\n");
      LOG_ERROR("get_next_questions() failed");
    }

    if (save_session(ses)) {
      free_session(ses);
      ses = NULL;
      free_next_questions(nq);
      nq = NULL;
      fprintf(stderr, "Unable to update session.\n");
      LOG_ERROR("save_session() failed");
    }

    // return adjacent next question only
    printf("%s\n", (nq->question_count) ? nq->next_questions[0]->uid : "");

    free_session(ses);
    free_next_questions(nq);
    LOG_INFO("Leaving addanswer handler.");

  } while (0);

  return retVal;
}

int do_delanswer(char *session_id, char *question_id) {
  int retVal = 0;

  struct session *ses = NULL;
  struct nextquestions *nq = NULL;

  do {
    LOG_INFO("Entering delanswer handler.");

    ses = load_session(session_id);
    if (!ses) {
      fprintf(stderr, "Could not load specified session. Does it exist?");
      LOG_ERROR("Could not load session");
    }

    char reason[1024];
    if (validate_session_action(ACTION_SESSION_DELETEANSWER, ses, reason, 1024)) {
      free_session(ses);
      ses = NULL;
      fprintf(stderr, "%s\n", reason);
      LOG_ERROR("Session action validation failed");
    }

    if (session_delete_answers_by_question_uid(ses, question_id, 1) < 0) {
      free_session(ses);
      ses = NULL;
      fprintf(stderr, "Answer does not match existing session records.\n");
      LOG_ERROR("session_delete_answers_by_question_uid() failed");
    }

    nq = get_next_questions(ses);
    if (!nq) {
      free_session(ses);
      ses = NULL;
      fprintf(stderr, "Could not load next questions.\n");
      LOG_ERROR("get_next_questions() failed");
    }

    if (save_session(ses)) {
      free_session(ses);
      ses = NULL;
      free_next_questions(nq);
      nq = NULL;
      fprintf(stderr, "Unable to update session.\n");
      LOG_ERROR("save_session() failed");
    }

    // return adjacent next question only
    printf("%s\n", (nq->question_count) ? nq->next_questions[0]->uid : "");

    free_session(ses);
    free_next_questions(nq);
    LOG_INFO("Leaving delanswer handler.");

  } while (0);

  return retVal;
}

int do_delsession(char *session_id) {
  int retVal = 0;
  struct session *ses = NULL;

  do {
    LOG_INFO("Entering delsession handler.");

    ses = load_session(session_id);
    if (!ses) {
      fprintf(stderr, "Could not load specified session. Does it exist?");
      LOG_ERROR("Could not load session");
    }

    /* below is intentinally disabled for cli, but left as a reminder to develop an admin role stategy */
    // char reason[1024];
    // if (validate_session_action(ACTION_SESSION_DELETE, ses, reason, 1024)) {
    //   free_session(ses);
    //   ses = NULL;
    //   fprintf(stderr, "%s\n", reason);
    //   LOG_ERROR("Session action validation failed");
    // }

    if (delete_session(session_id)) {
      free_session(ses);
      ses = NULL;
      fprintf(stderr, "Could not delete session. Does it exist?\n");
      LOG_ERROR("delete_session() failed");
    }

    free_session(ses);
    LOG_INFO("Leaving delsession handler.");

  } while (0);

  return retVal;
}

int do_analyse(char *session_id) {
  int retVal = 0;
  struct session *ses = NULL;

  do {
    LOG_INFO("Entering analyse handler.");

    ses = load_session(session_id);
    if (!ses) {
      fprintf(stderr, "Could not load specified session. Does it exist?");
      LOG_ERROR("Could not load session");
    }

    char reason[1024];
    if (validate_session_action(ACTION_SESSION_ANALYSIS, ses, reason, 1024)) {
      free_session(ses);
      ses = NULL;
      fprintf(stderr, "%s\n", reason);
      LOG_ERROR("Session action validation failed");
    }

    const char *analysis = NULL;
    if (get_analysis(ses, &analysis)) {
      free_session(ses);
      ses = NULL;
      fprintf(stderr, "Could not retrieve analysis.\n");
      LOG_ERROR("get_analysis() failed");
    }

    if (!analysis) {
      free_session(ses);
      ses = NULL;
      fprintf(stderr, "Could not retrieve analysis (NULL).\n");
      LOG_ERROR("get_analysis() returned NULL result");
    }

    if (!analysis[0]) {
      free_session(ses);
      ses = NULL;
      fprintf(stderr, "Could not retrieve analysis (empty result).\n");
      LOG_ERROR("get_analysis() returned empty result");
    }

    // store analysis with session
    if (session_add_datafile(ses->session_id, "analysis.json", analysis)) {
      LOG_WARNV("Could not add analysis.json for session.", 0);
      // do not break here
    }

    if (analysis) {
      printf("%s\n", analysis);
      free((char *)analysis);
    }

    free_session(ses);
    LOG_INFO("Leaving analyse handler.");

  } while (0);

  return retVal;
}

int main(int argc, char **argv) {
  int retVal = 0;

  do {
    if (argc < 2) {
      usage();
      retVal = -1;
      break;
    }

    init(argc, argv);

    if (!strcmp(argv[1], "newsession")) {

      if (argc != 3) {
        usage();
        retVal = -1;
        break;
      }

      if (do_newsession(argv[2])) {
        fprintf(stderr, "Failed to create new session.\n");
        LOG_ERROR("Failed to create new session");
      }

   } else if (!strcmp(argv[1], "nextquestion")) {

      if (argc != 3) {
        usage();
        retVal = -1;
        break;
      }

      if (do_nextquestions(argv[2])) {
        fprintf(stderr, "Failed to get next questions.\n");
        LOG_ERROR("Failed to get next questions");
      }

    } else if (!strcmp(argv[1], "delanswer")) {

      if (argc != 4) {
        usage();
        retVal = -1;
        break;
      }

      if (do_delanswer(argv[2], argv[3])) {
        fprintf(stderr, "Failed to add answer.\n");
        LOG_ERROR("Failed to add answer");
      }

    } else if (!strcmp(argv[1], "addanswer")) {

      if (argc != 4) {
        usage();
        retVal = -1;
        break;
      }

      if (do_addanswer(argv[2], argv[3])) {
        fprintf(stderr, "Failed to add answer.\n");
        LOG_ERROR("Failed to add answer");
      }

    } else if (!strcmp(argv[1], "delanswer")) {

      if (argc != 4) {
        usage();
        retVal = -1;
        break;
      }

      if (do_delanswer(argv[2], argv[3])) {
        fprintf(stderr, "Failed to add answer.\n");
        LOG_ERROR("Failed to add answer");
      }

    } else if (!strcmp(argv[1], "analyse")) {

      if (argc != 3) {
        usage();
        retVal = -1;
        break;
      }

      if (do_analyse(argv[2])) {
        fprintf(stderr, "Failed fetch analysis.\n");
        LOG_ERROR("Failed fetch analysis");
      }

    } else if (!strcmp(argv[1], "delsession")) {

      if (argc != 3) {
        usage();
        retVal = -1;
        break;
      }

      if (do_delsession(argv[2])) {
        fprintf(stderr, "Failed to delete session.\n");
        LOG_ERROR("Failed to delete session");
      }

    } else {
      usage();
      retVal = -1;
      break;
    }
  } while (0);

  if (retVal) {
    fprintf(stderr, "Command failed:\n");
    dump_errors(stderr);
  }
  return retVal;
}
