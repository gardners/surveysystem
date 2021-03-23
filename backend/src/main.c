#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "errorlog.h"
#include "serialisers.h"
#include "survey.h"
#include "sha1.h"

void usage(void) {
  fprintf(
      stderr,
      "usage: surveycli newsession <survey name> -- create a new session\n"
      "       surveycli addanswer <sessionid> <serialised answer> -- add an answer to an existing session\n"
      "       surveycli nextquestion <sessionid> -- get the next question that  be asked\n"
      "       surveycli delanswer <sessionid> <question id> -- delete an answer from an existing session\n"
      "       surveycli delprevanswer <sessionid> <question id> -- delete an answer from an existing session\n"
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
    char out [65536];
    if (serialise_question(nq->next_questions[0], out, 65536)) {
      LOG_WARNV("serialising next question '%s' failed", nq->next_questions[0]->uid);
    }

    printf("%s\n", out);

    return;
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

    // #268 create_session() now returns returns a session struct
    struct session *ses = create_session(survey_id, session_id, &meta);
    if (!ses) {
      fprintf(stderr, "failed to create session.\n");
      LOG_ERROR("Create session failed.");
    }
    free_session(ses);

    printf("%s\n", session_id);
    LOG_INFO("Leaving newsession handler.");
  } while(0);

  return retVal;
}

// #422, #425
int do_progress(char *session_id) {
  int retVal = 0;

  struct session *ses = NULL;

  do {
    LOG_INFO("Entering countquestions handler.");

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

    printf("%d/%d\n", ses->given_answer_count + 1, ses->question_count);
    LOG_INFO("Leaving progress handler.");
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

    print_nextquestion(nq);

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

    print_nextquestion(nq);

    free_session(ses);
    free_next_questions(nq);
    LOG_INFO("Leaving addanswer handler.");

  } while (0);

  return retVal;
}

// #425, 422
int do_addanswervalue(char *session_id, char *uid, char *value) {

  int retVal = 0;

  struct session *ses = NULL;
  struct answer *ans = NULL;
  struct nextquestions *nq = NULL;

  do {
    LOG_INFO("Entering addanswervalue handler.");

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

    // load question from session, we need the type
    struct question *qn = session_get_question(uid, ses);
    if (!qn) {
      free_session(ses);
      ses = NULL;
      LOG_ERRORV("could not find question '%s' in survey", uid);
    }

    // Deserialise answer
    ans = calloc(sizeof(struct answer), 1);
    ans->uid = strdup(uid);
    ans->type = qn->type;
    if (!ans) {
      fprintf(stderr, "calloc() of answer structure failed.\n");
      LOG_ERROR("calloc() of answer structure failed.");
    }

    if (answer_set_value_raw(ans, value)) {
      free_answer(ans);
      fprintf(stderr, "answer format is invalid.\n");
      LOG_ERROR("answer_set_value_raw() failed.");
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

    print_nextquestion(nq);

    free_session(ses);
    free_next_questions(nq);
    LOG_INFO("Leaving addanswervalue handler.");

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

    print_nextquestion(nq);

    free_session(ses);
    free_next_questions(nq);
    LOG_INFO("Leaving delanswer handler.");

  } while (0);

  return retVal;
}

int do_delprevanswer(char *session_id, char *checksum) {
  int retVal = 0;

  struct session *ses = NULL;
  struct nextquestions *nq = NULL;

  do {
    LOG_INFO("Entering delprevanswer handler.");


    // (weak) valiation if header->val is hash like string
    if (sha1_validate_string_hashlike(checksum)) {
      fprintf(stderr, "arg 'checksum' is invalid!");
      LOG_ERROR("carg 'checksum' is invalid!");
    }

    ses = load_session(session_id);
    if (!ses) {
      fprintf(stderr, "Could not load specified session. Does it exist?");
      LOG_ERROR("Could not load session");
    }

    // get last answer (not a system answer and not deleted)
    // define validation scope
    // if no answer to delete was found demote validation scope to just nextquestions
    struct answer *last = session_get_last_given_answer(ses);
    int action = (last) ? ACTION_SESSION_DELETEANSWER : ACTION_SESSION_NEXTQUESTIONS;

    char reason[1024];
    if (validate_session_action(action, ses, reason, 1024)) {
      free_session(ses);
      ses = NULL;
      fprintf(stderr, "%s\n", reason);
      LOG_ERROR("Session action validation failed");
    }

    if (strncmp(checksum, ses->consistency_hash, HASHSTRING_LENGTH )) {
      free_session(ses);
      fprintf(stderr, "checksum does not match session consistency hash!");
      LOG_ERROR("checksum does not match session consistency hash!");
    }
    LOG_INFOV("checksum match passed for session '%s'", ses->session_id);

    if (last) {
      if (session_delete_answer(ses, last, 0) < 0) {
        free_session(ses);
        ses = NULL;
        fprintf(stderr, "delete last answer failed.\n");
        LOG_ERROR("session_delete_answer() failed");
      }
      LOG_INFOV("deleted question '%s' in session '%s'", last->uid, ses->session_id);
    } else {
      LOG_WARNV("no last given answer in session '%s'", ses->session_id);
    }

    // fetch nextquestions, even if no last answer was found
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

    print_nextquestion(nq);

    free_session(ses);
    free_next_questions(nq);
    LOG_INFO("Leaving delprevanswer handler.");

  } while (0);

  return retVal;
}

int do_getchecksum(char *session_id) {
  int retVal = 0;

  struct session *ses = NULL;

  do {
    LOG_INFO("Entering getchecksum handler.");

    ses = load_session(session_id);
    if (!ses) {
      fprintf(stderr, "Could not load specified session. Does it exist?");
      LOG_ERROR("Could not load session");
    }

    char reason[1024];
    if (validate_session_action(ACTION_SESSION_NEXTQUESTIONS, ses, reason, 1024)) {
      free_session(ses);
      ses = NULL;
      fprintf(stderr, "%s\n", reason);
      LOG_ERROR("Session action validation failed");
    }

    if (!ses->consistency_hash || !strlen(ses->consistency_hash)) {
      free_session(ses);
      fprintf(stderr, "session does not contain a consistency hash! (legacy session?)");
      LOG_ERROR("consistency_hash is null or empty(legacy session?)");
    }

    printf("%s\n", ses->consistency_hash);

    free_session(ses);
    LOG_INFO("Leaving getchecksum handler.");

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
   } else if (!strcmp(argv[1], "progress")) {

      // #422, add command for session progress count
      if (argc != 3) {
        usage();
        retVal = -1;
        break;
      }

      if (do_progress(argv[2])) {
        fprintf(stderr, "Failed to count survey progress.\n");
        LOG_ERROR("Failed to count survey progress");
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

    } else if (!strcmp(argv[1], "addanswervalue")) {

      // #422 add command for simplified answer value submission
      if (argc != 5) {
        usage();
        retVal = -1;
        break;
      }

      if (do_addanswervalue(argv[2], argv[3], argv[4])) {
        fprintf(stderr, "Failed to add answer value.\n");
        LOG_ERROR("Failed to add answer value");
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

    } else if (!strcmp(argv[1], "delprevanswer")) {

      // #426, add cli support for consistency hash
      if (argc != 4) {
        usage();
        retVal = -1;
        break;
      }

      if (do_delprevanswer(argv[2], argv[3])) {
        fprintf(stderr, "Failed to delete last answer.\n");
        LOG_ERROR("Failed to delete last answer");
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
        LOG_ERROR("Failed fetch analysis");
      }

    } else if (!strcmp(argv[1], "analyse")) {

      if (argc != 3) {
        usage();
        retVal = -1;
        break;
      }

      if (do_analyse(argv[2])) {
        fprintf(stderr, "Failed to fetch analysis.\n");
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
