#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "errorlog.h"
#include "serialisers.h"
#include "survey.h"
#include "sha1.h"
#include "utils.h"
#include "validators.h"

void usage(void) {
  fprintf(
      stderr,
      "usage: surveycli newsession <survey name> -- create a new session\n"
      "       surveycli addanswer <sessionid> <serialised answer> -- add a serialised answer to an existing session\n"
      "       surveycli addanswervalue <sessionid> <question_id> <value> -- add an answer value to an existing session\n"
      "       surveycli nextquestion <sessionid> -- get the next question that  be asked\n"
      "       surveycli delanswer <sessionid> <question id> -- delete an answer (and all following) from an existing session\n"
      "       surveycli delprevanswer <sessionid> <checksum> -- delete previous answer from an existing session\n"
      "       surveycli delsession <sessionid> -- delete an existing session\n"
      "       surveycli analyse <sessionid> -- get the analysis of a finished session\n"
      "       surveycli progress <sessionid> -- get the progress count of an existing session\n"
      "       surveycli getchecksum <sessionid> -- get consistency hash of an existing session\n");
};

void init(int argc, char **argv) {

  char *env;
  char test[1024];
  int log_tested = 0;

  env = getenv("SURVEY_HOME");
  if (!env) {
    setenv("SURVEY_HOME", ".", 1);
    LOG_INFO("env 'SURVEY_HOME' not found set to '.'");
  }

  env = getenv("SURVEY_PYTHONDIR") ;
  if (!env) {
    setenv("SURVEY_PYTHONDIR", "./python", 1);
    LOG_INFO("env 'SURVEY_PYTHONDIR' not found set to './python'");
  }

  env = getenv("SS_LOG_FILE");
  if (env) {
    LOG_INFOV("env 'SS_LOG_FILE' found, custom log path is '%s'", env);

    // test custom log
    FILE *fp = fopen(env, "a");
    if (!fp) {
      fprintf(stderr, "Cannot access custom log  '%s' for append. \n", env);
      exit(-1);
    }
    fclose(fp);
    log_tested = 1;
  }

  env = getenv("SURVEY_HOME");

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

 // test log dir
  if (log_tested) {
    return;
  }

  env = getenv("SURVEY_HOME");
  snprintf(test, 1024 ,"%s/logs", (env) ? env: "");
  if (access(test, W_OK)) {
    fprintf(stderr, "Cannot access log dir '%s' (W_OK). Does it exist?\n", test);
    exit(-1);
  }

}

void print_nextquestion(struct nextquestions *nq) {
    if (!nq ) {
      return;
    }
    // end of session
    if (!nq->question_count) {
      return;
    }

    // return adjacent next question only
    char out [MAX_LINE];
    if (serialise_question(nq->next_questions[0], out, MAX_LINE)) {
      LOG_WARNV("serialising next question '%s' failed", nq->next_questions[0]->uid);
    }

    printf("%s\n", out);

    return;
}

int do_newsession(char *survey_id) {
  int retVal = 0;

  struct session *ses = NULL;
  struct nextquestions *nq = NULL;
  enum actions action = ACTION_SESSION_NEW;
  int err;

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
      BREAK_ERROR("Create session id failed.");
    }

    // #268 create_session() now returns returns a session struct
    ses = create_session(survey_id, session_id, &meta, &err);
    if (!ses) {
      fprintf(stderr, "Failed to create session, error: '%s'\n", get_error(err, 0, "[ERROR] unknown"));
      BREAK_ERROR("Create session failed.");
    }

    // # 461 store nextquestions after creating session
    nq = get_next_questions(ses, action, 0);
    if (!nq) {
      BREAK_ERROR("get_next_questions() failed");
    }

    if (save_session(ses)) {
      BREAK_ERROR("save_session() failed");
    }

    printf("%s\n", session_id);
    LOG_INFO("Leaving newsession handler.");
  } while(0);

  // destruct
  free_session(ses);

  return retVal;
}

// #422, #425
int do_progress(char *session_id) {
  int retVal = 0;

  struct session *ses = NULL;
  enum actions action = ACTION_SESSION_NEXTQUESTIONS;

  do {
    LOG_INFO("Entering progress handler.");
    int err;

    ses = load_session(session_id, &err);
    if (!ses) {
      fprintf(stderr, "Could not load specified session, error: '%s'\n", get_error(err, 0, "[ERROR] unknown"));
      BREAK_ERROR("Could not load session");
    }

    char reason[1024];
    if (validate_session_action(action, ses, reason, 1024)) {
      fprintf(stderr, "%s\n", reason);
      BREAK_ERROR("Session action validation failed");
    }

    printf("%d/%d\n", ses->given_answer_count + 1, ses->question_count);
    LOG_INFO("Leaving progress handler.");
  } while(0);

  // destruct
  free_session(ses);

  return retVal;
}

int do_nextquestions(char *session_id) {
  int retVal = 0;

  struct session *ses = NULL;
  struct nextquestions *nq = NULL;
  enum actions action = ACTION_SESSION_NEXTQUESTIONS;

  do {
    LOG_INFO("Entering nextquestion handler.");
    int err;

    ses = load_session(session_id, &err);
    if (!ses) {
      fprintf(stderr, "Could not load specified session, error: '%s'\n", get_error(err, 0, "[ERROR] unknown"));
      BREAK_ERROR("Could not load session");
    }

    char reason[1024];
    if (validate_session_action(action, ses, reason, 1024)) {
      fprintf(stderr, "%s\n", reason);
      BREAK_ERROR("Session action validation failed");
    }

    nq = get_next_questions(ses, action, 0);
    if (!nq) {
      fprintf(stderr, "Could not load next questions.\n");
      BREAK_ERROR("get_next_questions() failed");
    }

    if (save_session(ses)) {
      fprintf(stderr, "Unable to update session.\n");
      BREAK_ERROR("save_session() failed");
    }

    print_nextquestion(nq);
    LOG_INFO("Leaving nextquestion handler.");

  } while (0);

  // destruct
  free_session(ses);
  free_next_questions(nq);

  return retVal;
}

int do_addanswer(char *session_id, char *serialised_answer) {
  int retVal = 0;

  struct session *ses = NULL;
  struct answer *ans = NULL;
  struct nextquestions *nq = NULL;
  enum actions action = ACTION_SESSION_ADDANSWER;

  do {
    LOG_INFO("Entering addanswer handler.");
    int err;

    ses = load_session(session_id, &err);
    if (!ses) {
      fprintf(stderr, "Could not load specified session, error: '%s'\n", get_error(err, 0, "[ERROR] unknown"));
      BREAK_ERROR("Could not load session");
    }

    char reason[1024];
    if (validate_session_action(action, ses, reason, 1024)) {
      fprintf(stderr, "%s\n", reason);
      BREAK_ERROR("Session action validation failed");
    }

    // Deserialise answer
    ans = calloc(sizeof(struct answer), 1);
    if (!ans) {
      fprintf(stderr, "calloc() of answer structure failed.\n");
      BREAK_ERROR("calloc() of answer structure failed.");
    }

    if (deserialise_answer(serialised_answer, ANSWER_SCOPE_PUBLIC, ans)) {
      fprintf(stderr, "answer format is invalid.\n");
      BREAK_ERROR("deserialise_answer() failed.");
    }

    // #482 validate against ses->next_questions
    int match = strcmp(
      (ans->uid) ?  ans->uid : "<null answers>",
      (ses->next_questions) ? ses->next_questions : "<null next_questions>"
    );

    if (match != 0) {
      BREAK_ERRORV("Answers don't match next_questions: '%.50s' != '%.50s'", ans->uid, ses->next_questions);
    }

    if (validate_session_add_answer(ses, ans)) {
      fprintf(stderr, "Answer is invalid\n");
      BREAK_ERROR("Answer validation failed");
    }

    // #445 count affected answers
    int affected_count = session_add_answer(ses, ans);
    if (affected_count < 0) {
      fprintf(stderr, "could not add answer to session.\n");
      BREAK_ERROR("session_add_answer() failed.");
    }

    nq = get_next_questions(ses, action, affected_count);
    if (!nq) {
      fprintf(stderr, "Could not load next questions.\n");
      BREAK_ERROR("get_next_questions() failed");
    }

    if (save_session(ses)) {
      fprintf(stderr, "Unable to update session.\n");
      BREAK_ERROR("save_session() failed");
    }

    print_nextquestion(nq);
    LOG_INFO("Leaving addanswer handler.");

  } while (0);

  // destruct
  free_session(ses);
  free_answer(ans);
  free_next_questions(nq);

  return retVal;
}

// #425, 422
int do_addanswervalue(char *session_id, char *uid, char *value) {

  int retVal = 0;

  struct session *ses = NULL;
  struct answer *ans = NULL;
  struct nextquestions *nq = NULL;
  enum actions action = ACTION_SESSION_ADDANSWER;

  do {
    LOG_INFO("Entering addanswervalue handler.");
    int err;

    ses = load_session(session_id, &err);
    if (!ses) {
      fprintf(stderr, "Could not load specified session, error: '%s'\n", get_error(err, 0, "[ERROR] unknown"));
      BREAK_ERROR("Could not load session");
    }

    char reason[1024];
    if (validate_session_action(action, ses, reason, 1024)) {
      fprintf(stderr, "%s\n", reason);
      BREAK_ERROR("Session action validation failed");
    }

    // load question from session, we need the type
    struct question *qn = session_get_question(uid, ses);
    if (!qn) {
      BREAK_ERRORV("could not find question '%s' in survey", uid);
    }

    // Deserialise answer
    ans = calloc(sizeof(struct answer), 1);
    if (!ans) {
      fprintf(stderr, "calloc() of answer structure failed.\n");
      BREAK_ERROR("calloc() of answer structure failed.");
    }
    ans->uid = strdup(uid);
    ans->type = qn->type;

    // #482 validate against ses->next_questions
    int match = strcmp(
      (ans->uid) ?  ans->uid : "<null answers>",
      (ses->next_questions) ? ses->next_questions : "<null next_questions>"
    );

    if (match != 0) {
      BREAK_ERRORV("Answers don't match next_questions: '%.50s' != '%.50s'", ans->uid, ses->next_questions);
    }

    if (answer_set_value_raw(ans, value)) {
      fprintf(stderr, "answer format is invalid.\n");
      BREAK_ERROR("answer_set_value_raw() failed.");
    }

    // #445 count affected answers
    int affected_count = session_add_answer(ses, ans);
    if (affected_count < 0) {
      fprintf(stderr, "could not add answer to session.\n");
      BREAK_ERROR("session_add_answer() failed.");
    }

    nq = get_next_questions(ses, action, affected_count);
    if (!nq) {
      fprintf(stderr, "Could not load next questions.\n");
      BREAK_ERROR("get_next_questions() failed");
    }

    if (save_session(ses)) {
      fprintf(stderr, "Unable to update session.\n");
      BREAK_ERROR("save_session() failed");
    }

    print_nextquestion(nq);
    LOG_INFO("Leaving addanswervalue handler.");

  } while (0);

  // destruct
  free_session(ses);
  free_answer(ans);
  free_next_questions(nq);

  return retVal;
}

int do_delanswer(char *session_id, char *question_id) {
  int retVal = 0;

  struct session *ses = NULL;
  struct nextquestions *nq = NULL;
  enum actions action = ACTION_SESSION_DELETEANSWER;

  do {
    LOG_INFO("Entering delanswer handler.");
    int err;

    ses = load_session(session_id, &err);
    if (!ses) {
      fprintf(stderr, "Could not load specified session, error: '%s'\n", get_error(err, 0, "[ERROR] unknown"));
      BREAK_ERROR("Could not load session");
    }

    char reason[1024];
    if (validate_session_action(action, ses, reason, 1024)) {
      fprintf(stderr, "%s\n", reason);
      BREAK_ERROR("Session action validation failed");
    }

    if (validate_session_delete_answer(ses, question_id)) {
      fprintf(stderr, "validating answer for deletion failed\n");
      BREAK_ERROR("validate_session_delete_answer() failed");
    }

    // #445 count affected answers
    int affected_count = session_delete_answer(ses, question_id);
    if (affected_count < 0) {
      fprintf(stderr, "Answer does not match existing session records.\n");
      BREAK_ERROR("session_delete_answer() failed");
    }

    nq = get_next_questions(ses, action, affected_count);
    if (!nq) {
      fprintf(stderr, "Could not load next questions.\n");
      BREAK_ERROR("get_next_questions() failed");
    }

    if (save_session(ses)) {
      fprintf(stderr, "Unable to update session.\n");
      BREAK_ERROR("save_session() failed");
    }

    print_nextquestion(nq);
    LOG_INFO("Leaving delanswer handler.");

  } while (0);

  // destruct
  free_session(ses);
  free_next_questions(nq);

  return retVal;
}

int do_delprevanswer(char *session_id, char *checksum) {
  int retVal = 0;

  struct session *ses = NULL;
  struct nextquestions *nq = NULL;
  enum actions action = ACTION_SESSION_DELETEANSWER;

  do {
    LOG_INFO("Entering delprevanswer handler.");
    int err;

    // (weak) valiation if header->val is hash like string
    if (sha1_validate_string_hashlike(checksum)) {
      fprintf(stderr, "arg 'checksum' is invalid!");
      BREAK_ERROR("carg 'checksum' is invalid!");
    }

    ses = load_session(session_id, &err);
    if (!ses) {
      fprintf(stderr, "Could not load specified session, error: '%s'\n", get_error(err, 0, "[ERROR] unknown"));
      BREAK_ERROR("Could not load session");
    }

    // get last answer (not a system answer and not deleted)
    // define validation scope
    // if no answer to delete was found demote validation scope to just nextquestions
    struct answer *last = session_get_last_given_answer(ses);
    int validate_action = (last) ? ACTION_SESSION_DELETEANSWER : ACTION_SESSION_NEXTQUESTIONS;

    char reason[1024];
    if (validate_session_action(validate_action, ses, reason, 1024)) {
      fprintf(stderr, "%s\n", reason);
      BREAK_ERROR("Session action validation failed");
    }

    if (strncmp(checksum, ses->consistency_hash, HASHSTRING_LENGTH )) {
      fprintf(stderr, "checksum does not match session consistency hash!");
      BREAK_ERROR("checksum does not match session consistency hash!");
    }
    LOG_INFOV("checksum match passed for session '%s'", ses->session_id);

    // #445 count affected answers
    int affected_count = -1;

    if (last) {
      if (validate_session_delete_answer(ses, last->uid)) {
        fprintf(stderr, "validating answer for deletion failed\n");
        BREAK_ERROR("validate_session_delete_answer() failed");
      }

      LOG_INFOV("deleting last given answer '%s'", last->uid);
      affected_count = session_delete_answer(ses, last->uid);
    } else {
      LOG_INFO("no last given answer in session");
      affected_count = 0;
    }

    if (affected_count < 0) {
      fprintf(stderr, "delete last answer failed.\n");
      BREAK_ERROR("session_delete_answer() failed");
    }

    // fetch nextquestions, even if no last answer was found
    nq = get_next_questions(ses, action, affected_count);
    if (!nq) {
      fprintf(stderr, "Could not load next questions.\n");
      BREAK_ERROR("get_next_questions() failed");
    }

    if (save_session(ses)) {
      fprintf(stderr, "Unable to update session.\n");
      BREAK_ERROR("save_session() failed");
    }

    print_nextquestion(nq);
    LOG_INFO("Leaving delprevanswer handler.");

  } while (0);

  // destruct
  free_session(ses);
  free_next_questions(nq);

  return retVal;
}

int do_getchecksum(char *session_id) {
  int retVal = 0;

  struct session *ses = NULL;
  enum actions action = ACTION_SESSION_NEXTQUESTIONS;

  do {
    LOG_INFO("Entering getchecksum handler.");
    int err;

    ses = load_session(session_id, &err);
    if (!ses) {
      fprintf(stderr, "Could not load specified session, error: '%s'\n", get_error(err, 0, "[ERROR] unknown"));
      BREAK_ERROR("Could not load session");
    }

    char reason[1024];
    if (validate_session_action(action, ses, reason, 1024)) {
      fprintf(stderr, "%s\n", reason);
      BREAK_ERROR("Session action validation failed");
    }

    if (!ses->consistency_hash || !strlen(ses->consistency_hash)) {
      fprintf(stderr, "session does not contain a consistency hash! (legacy session?)");
      BREAK_ERROR("consistency_hash is null or empty(legacy session?)");
    }

    printf("%s\n", ses->consistency_hash);
    LOG_INFO("Leaving getchecksum handler.");

  } while (0);

  // destruct
  free_session(ses);

  return retVal;
}

int do_delsession(char *session_id) {
  int retVal = 0;

  struct session *ses = NULL;
  // enum actions action = ACTION_SESSION_DELETE;

  do {
    LOG_INFO("Entering delsession handler.");
    int err;

    ses = load_session(session_id, &err);
    if (!ses) {
      fprintf(stderr, "Could not load specified session, error: '%s'\n", get_error(err, 0, "[ERROR] unknown"));
      BREAK_ERROR("Could not load session");
    }

    /* below is intentinally disabled for cli, but left as a reminder to develop an admin role stategy */
    // char reason[1024];
    // if (validate_session_action(action, ses, reason, 1024)) {
    //   fprintf(stderr, "%s\n", reason);
    //   BREAK_ERROR("Session action validation failed");
    // }

    if (delete_session(session_id)) {
      fprintf(stderr, "Could not delete session. Does it exist?\n");
      BREAK_ERROR("delete_session() failed");
    }

    printf("session deleted \n");
    LOG_INFO("Leaving delsession handler.");

  } while (0);

  // destruct
  free_session(ses);

  return retVal;
}

int do_analyse(char *session_id) {
  int retVal = 0;

  struct session *ses = NULL;
  const char *analysis = NULL;
  enum actions action = ACTION_SESSION_ANALYSIS;

  do {
    LOG_INFO("Entering analyse handler.");
    int err;

    ses = load_session(session_id, &err);
    if (!ses) {
      fprintf(stderr, "Could not load specified session, error: '%s'\n", get_error(err, 0, "[ERROR] unknown"));
      BREAK_ERROR("Could not load session");
    }

    char reason[1024];
    if (validate_session_action(action, ses, reason, 1024)) {
      fprintf(stderr, "%s\n", reason);
      BREAK_ERROR("Session action validation failed");
    }

    if (get_analysis(ses, &analysis)) {
      fprintf(stderr, "Could not retrieve analysis.\n");
      BREAK_ERROR("get_analysis() failed");
    }

    if (!analysis) {
      fprintf(stderr, "Could not retrieve analysis (NULL).\n");
      BREAK_ERROR("get_analysis() returned NULL result");
    }

    if (!analysis[0]) {
      fprintf(stderr, "Could not retrieve analysis (empty result).\n");
      BREAK_ERROR("get_analysis() returned empty result");
    }

    // store analysis with session
    char fname[256];
    snprintf(fname, 256, "%s.analysis.json", session_id);
    if (session_add_datafile(ses->session_id, fname, analysis)) {
      LOG_WARNV("Could not add analysis.json for session.", 0);
      // do not break here
    }

    printf("%s\n", (char *)analysis);
    LOG_INFO("Leaving analyse handler.");

  } while (0);

  // destruct
  free_session(ses);
  freez((char *)analysis);

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
        BREAK_ERROR("Failed to create new session");
      }
   } else if (!strcmp(argv[1], "progress")) {

      // #422, add command for session progress count
      if (argc != 3) {
        usage();
        retVal = -1;
        break;
      }

      if (do_progress(argv[2])) {
        fprintf(stderr, "Failed to count survey progress.\n");
        BREAK_ERROR("Failed to count survey progress");
      }

   } else if (!strcmp(argv[1], "nextquestion")) {

      if (argc != 3) {
        usage();
        retVal = -1;
        break;
      }

      if (do_nextquestions(argv[2])) {
        fprintf(stderr, "Failed to get next questions.\n");
        BREAK_ERROR("Failed to get next questions");
      }

    } else if (!strcmp(argv[1], "delanswer")) {

      if (argc != 4) {
        usage();
        retVal = -1;
        break;
      }

      if (do_delanswer(argv[2], argv[3])) {
        fprintf(stderr, "Failed to delete answer.\n");
        BREAK_ERROR("Failed to delete answer");
      }

    } else if (!strcmp(argv[1], "addanswer")) {

      if (argc != 4) {
        usage();
        retVal = -1;
        break;
      }

      if (do_addanswer(argv[2], argv[3])) {
        fprintf(stderr, "Failed to add answer.\n");
        BREAK_ERROR("Failed to add answer");
      }

    } else if (!strcmp(argv[1], "addanswervalue")) {

      // #422 add command for simplified answer value submission
      if (argc != 5) {
        usage();
        retVal = -1;
        break;
      }

      if (do_addanswervalue(argv[2], argv[3], argv[4])) {
        fprintf(stderr, "Failed to add answer value.\n");
        BREAK_ERROR("Failed to add answer value");
      }

    } else if (!strcmp(argv[1], "delprevanswer")) {

      // #426, add cli support for consistency hash
      if (argc != 4) {
        usage();
        retVal = -1;
        break;
      }

      if (do_delprevanswer(argv[2], argv[3])) {
        fprintf(stderr, "Failed to delete last answer.\n");
        BREAK_ERROR("Failed to delete last answer");
      }

    } else if (!strcmp(argv[1], "getchecksum")) {

      // #426, add cli support for consistency hash
      if (argc != 3) {
        usage();
        retVal = -1;
        break;
      }

      if (do_getchecksum(argv[2])) {
        fprintf(stderr, "Failed to fetch consistency checksum.\n");
        BREAK_ERROR("Failed fetch analysis");
      }

    } else if (!strcmp(argv[1], "analyse")) {

      if (argc != 3) {
        usage();
        retVal = -1;
        break;
      }

      if (do_analyse(argv[2])) {
        fprintf(stderr, "Failed to fetch analysis.\n");
        BREAK_ERROR("Failed fetch analysis");
      }

    } else if (!strcmp(argv[1], "delsession")) {

      if (argc != 3) {
        usage();
        retVal = -1;
        break;
      }

      if (do_delsession(argv[2])) {
        fprintf(stderr, "Failed to delete session.\n");
        BREAK_ERROR("Failed to delete session");
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
