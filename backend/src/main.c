
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "errorlog.h"
#include "serialisers.h"
#include "survey.h"

void usage(void) {
  fprintf(
      stderr,
      "usage: survey newsession <survey name> -- create a new session\n"
      "       survey addanswer <sessionid> <serialised answer> -- add an "
      "answer to an existing session\n"
      "       survey updateanswer <sessionid> <serialised answer> -- add (or "
      "replace an existing) answer to an existing session\n"
      "       survey nextquestion <sessionid> -- get the next question that "
      "should be asked\n"
      "       survey delanswer <sessionid> <question id> -- delete an answer "
      "from an existing session\n"
      "       survey delsession <sessionid> -- delete an existing session\n");
};

int main(int argc, char **argv) {
  int retVal = 0;

  do {
    if (argc < 2) {
      usage();
      retVal = -1;
      break;
    }

    if (!strcmp(argv[1], "newsession")) {

      if (argc != 3) {
        usage();
        retVal = -1;
        break;
      }

      char session_id[1024];

      // #363
      struct session_meta meta = {
        .user = NULL,
        .group = NULL,
        .authority = NULL,
        .provider = IDENDITY_CLI,
      };

      if (create_session(argv[2], session_id, &meta)) {
        LOG_ERROR("create_session() failed");
      }

      free_session_meta(&meta);

      if (!retVal) {
        printf("%s\n", session_id);
      } else {
        fprintf(stderr, "Failed to create new session.\n");
      }

    } else if (!strcmp(argv[1], "addanswer")) {

      if (argc != 4) {
        usage();
        retVal = -1;
        break;
      }

      char *session_id = argv[2];
      char *serialised_answer = argv[3];

      struct session *s = load_session(session_id);
      if (!s)
        LOG_ERRORV("load_session('%s') failed", session_id);
      struct answer a;
      bzero(&a, sizeof(struct answer));
      if (deserialise_answer(serialised_answer, ANSWER_FIELDS_PUBLIC, &a))
        LOG_ERRORV("deserialise_answer('%s') failed", serialised_answer);
      if (!s)
        LOG_ERRORV("load_session('%s') failed", session_id);
      if (session_add_answer(s, &a))
        LOG_ERRORV("session_add_answer('%s','%s') failed", session_id,
                   serialised_answer);
      if (save_session(s))
        LOG_ERRORV("save_session('%s') failed", session_id);

    } else if (!strcmp(argv[1], "updateanswer")) {

      if (argc != 4) {
        usage();
        retVal = -1;
        break;
      }

      char *session_id = argv[2];
      char *serialised_answer = argv[3];

      struct session *s = load_session(session_id);
      if (!s)
        LOG_ERRORV("load_session('%s') failed", session_id);

      // validate requested action against session current state (#379)
      char reason[1024];
      if (validate_session_action(ACTION_SESSION_ADDANSWER, s, reason, 1024)) {
        LOG_ERROR(reason);
      }

      struct answer a;
      bzero(&a, sizeof(struct answer));
      if (deserialise_answer(serialised_answer, ANSWER_FIELDS_PUBLIC, &a))
        LOG_ERRORV("deserialise_answer('%s') failed", serialised_answer);
      if (!s)
        LOG_ERRORV("load_session('%s') failed", session_id);
      if (session_delete_answers_by_question_uid(s, a.uid, 0) < 0)
        LOG_ERRORV("session_delete_answers_by_question_uid('%s','%s') failed",
                   session_id, serialised_answer);
      if (session_add_answer(s, &a))
        LOG_ERRORV("session_add_answer('%s','%s') failed", session_id,
                   serialised_answer);
      if (save_session(s))
        LOG_ERRORV("save_session('%s') failed", session_id);

    } else if (!strcmp(argv[1], "nextquestion")) {

      if (argc != 3) {
        usage();
        retVal = -1;
        break;
      }

      char *session_id = argv[2];
      struct session *s = load_session(session_id);

      // validate requested action against session current state (#379)
      char reason[1024];
      if (validate_session_action(ACTION_SESSION_NEXTQUESTIONS, s, reason, 1024)) {
        LOG_ERROR(reason);
      }

      // #332 nextquestions data struct
      struct nextquestions *nq = get_next_questions(s);
      if (!nq) {
        free_next_questions(nq);
        LOG_ERRORV("get_next_questions('%s') failed", session_id);
      }

      printf("%d\n", nq->question_count);
      for (int i = 0; nq->question_count; i++) {
        printf("%s\n", nq->next_questions[i]->uid);
      }

      free_next_questions(nq);

    } else if (!strcmp(argv[1], "delanswer")) {

      if (argc != 4) {
        usage();
        retVal = -1;
        break;
      }

      char *session_id = argv[2];
      char *serialised_answer = argv[3];

      struct session *s = load_session(session_id);
      if (!s)
        LOG_ERRORV("load_session('%s') failed", session_id);

      // validate requested action against session current state (#379)
      char reason[1024];
      if (validate_session_action(ACTION_SESSION_DELETEANSWER, s, reason, 1024)) {
        LOG_ERROR(reason);
      }

      if (strstr(serialised_answer, ":")) {
        struct answer a;
        bzero(&a, sizeof(struct answer));
        if (deserialise_answer(serialised_answer, ANSWER_FIELDS_PUBLIC, &a))
          LOG_ERRORV("deserialise_answer('%s') failed", serialised_answer);
        if (session_delete_answer(s, &a, 0))
          LOG_ERRORV("session_delete_answer('%s','%s') failed", session_id,
                     serialised_answer);
      } else {
        if (session_delete_answers_by_question_uid(s, serialised_answer, 0))
          LOG_ERRORV("session_delete_answers_by_question_uid('%s','%s') failed",
                     session_id, serialised_answer);
      }

      if (save_session(s))
        LOG_ERRORV("save_session('%s') failed", session_id);

    } else if (!strcmp(argv[1], "delsession")) {

      if (argc != 3) {
        usage();
        retVal = -1;
        break;
      }

      char *session_id = argv[2];

      struct session *s = load_session(session_id);
      if (!s)
        LOG_ERRORV("load_session('%s') failed", session_id);
      free_session(s);

      // validate requested action against session current state (#379)
      char reason[1024];
      if (validate_session_action(ACTION_SESSION_DELETE, s, reason, 1024)) {
        LOG_ERROR(reason);
      }

      if (delete_session(session_id))
        LOG_ERRORV("delete_session('%s') failed", session_id);

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
