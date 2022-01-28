
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "kcgi.h"
#include "kcgijson.h"

#include "errorlog.h"
#include "question_types.h"
#include "serialisers.h"
#include "survey.h"
#include "validators.h"
#include "utils.h"
#include "sha1.h"

#include "fcgi.h"

static const struct kvalid keys[KEY__MAX] = {
  { kvalid_stringne, "surveyid" },
  { kvalid_stringne, "sessionid" },
  { kvalid_stringne, "questionid" },
  { kvalid_stringne, "answer" },
  { kvalid_stringne, "if-match" },
  { kvalid_stringne, "extended" },
};

typedef void (*disp)(struct kreq *);

static void fcgi_page_index(struct kreq *);
static void fcgi_page_session(struct kreq *);
static void fcgi_page_questions(struct kreq *);
static void fcgi_page_answers(struct kreq *);
static void fcgi_page_analysis(struct kreq *);
static void fcgi_page_check(struct kreq *);

static enum khttp fcgi_sanitise_page_request(const struct kreq *req);

static const disp disps[PAGE__MAX] = {
    fcgi_page_index,

    fcgi_page_session,
    fcgi_page_questions,
    fcgi_page_answers,
    fcgi_page_analysis,

    fcgi_page_check,
};

static const char *const pages[PAGE__MAX] = {
    "index",

    "session",
    "questions",
    "answers",
    "analysis",

    "status",
};

void usage(void) {
  fprintf(stderr, "usage: surveyfcgi -- Start fast CGI service\n");
};

int main(int argc, char **argv) {
  int retVal = 0;

  do {
    if (argc > 1) {
      usage();
      retVal = -1;
      break;
    }

    if (!getenv("SURVEY_HOME")) {
      fprintf(stderr, "SURVEY_HOME environment variable must be set to data "
                      "directory for survey system.\n");
      usage();
      retVal = -1;
      break;
    }

    struct kreq req;
    struct kfcgi *fcgi = NULL;
    enum kcgi_err er;

    if (KCGI_OK != khttp_fcgi_init(&fcgi, keys,
      KEY__MAX, // CGI variable parse definitions
      pages, PAGE__MAX, // Pages for parsing
      PAGE_INDEX)) {
        BREAK_ERROR("khttp_fcgi_init() failed.");
    }

    if (!fcgi) {
      BREAK_ERROR("fcgi==NULL after call to khttp_fcgi_init()");
    }

    // For each request
    for (;;) {

      // clear internal error log
      clear_errors();

      // parse request
      fprintf(stderr, "Calling fcgi_parse()\n");
      er = khttp_fcgi_parse(fcgi, &req);

      fprintf(stderr, "Returned from fcgi_parse()\n");

      if (KCGI_EXIT == er) {

        LOG_WARNV("khttp_fcgi_parse: terminate, becausee er == KCGI_EXIT", 1);
        fprintf(stderr, "khttp_fcgi_parse: terminate, becausee er == KCGI_EXIT");
        break;

      } else if (KCGI_OK != er) {

        LOG_WARNV("khttp_fcgi_parse: error: %d\n", er);
        fprintf(stderr, "khttp_fcgi_parse: error: %d\n", er);
        break;

      }

      // #437 add 404 page handler and request prevalidation
      enum khttp valid = fcgi_sanitise_page_request(&req);
      if (valid != KHTTP_200) {

        er = http_open(&req, valid, req.mime, NULL);
        if (KCGI_HUP == er) {
          continue;
        }

      } else {

        if (KMETHOD_OPTIONS == req.method) {

            khttp_head(&req, kresps[KRESP_ALLOW], "OPTIONS HEAD GET POST");
            er = http_open(&req, KHTTP_200, req.mime, NULL);
            if (KCGI_HUP == er) {
              continue;
            }

        } else {
            // Call page dispatcher
            (*disps[req.page])(&req);
            // Make sure no sessions are locked when done.
            release_my_session_locks();
        }

      }

      // Close off request
      khttp_free(&req);
    } // end for

    CHECKPOINT();
    khttp_fcgi_free(fcgi);
    CHECKPOINT();

  } while (0);

  if (retVal) {
    fprintf(stderr, "Survey FASTCGI service failed:\n");
    dump_errors(stderr);
  }
  return retVal;
}

/**
 * Prevalidates incoming request (registred path, mime type etc)
 * #414, #437
 */
static enum khttp fcgi_sanitise_page_request(const struct kreq *req) {

    if (!req) {
        return KHTTP_500;
    }

    // man khttp_parse(3): The page index found by looking up pagename in the pages array.
    //   If pagename is not found in pages, pagesz is used; if pagename is empty, defpage is used.
    if(req->page >= PAGE__MAX) {
        return KHTTP_404;
    }

    // TODO, add mime type checks
    return KHTTP_200;
}

/**
 * Parse request payload and add desrerialised answer(s) to session
 * #260
 */
int fcgi_session_add_answers(struct kreq *req, struct session *ses, int *affected_count) {
  int retVal = 0;
  struct answer_list *list = NULL;

  do {
    BREAK_IF(req == NULL, SS_ERROR_ARG, "req");
    BREAK_IF(ses == NULL, SS_ERROR_ARG, "ses");

    *affected_count =0;
    int res;

    // parse answers

    list = fcgi_request_parse_answers_serialised(req, ses, &res);
    if (res) {
      BREAK_CODE(res, "error parsing answers (serialised)");
    }

    if (!list) {
      list = fcgi_request_parse_answers_values(req, ses, &res);
      if (res) {
        BREAK_CODE(res, "error parsing answers (values)");
      }
    }

    if (!list) {
      BREAK_CODE(SS_MISSING_ANSWER, "Could find find answers in request");
    }

    // add answers

    for (int i = 0; i < list->len; i++) {
      res = validate_session_add_answer(ses, list->answers[i]);
      BREAK_IF(res != SS_OK, res, NULL);

      // #445 count affected answers
      *affected_count += session_add_answer(ses, list->answers[i]);
      if (*affected_count < 0) {
        BREAK_CODEV(SS_INVALID_ANSWER, "Failed to add answer", list->answers[i]->uid);
      }
    }

    if (retVal) {
      break;
    }

  } while (0);

  free_answer_list(list);
  return retVal;
}

/**
 * Parse request payload and adelete answer(s) from session
 * #260
 */
int fcgi_session_delete_answers(struct kreq *req, struct session *ses, int *affected_count) {
  int retVal = 0;

  do {
    BREAK_IF(req == NULL, SS_ERROR_ARG, "req");
    BREAK_IF(ses == NULL, SS_ERROR_ARG, "ses");

    *affected_count = 0;
    int res;

    // validate consiststency checksum

    char *sha1 = fcgi_request_get_consistency_hash(req);
    if (!sha1) {
      BREAK_CODE(SS_INVALID_CONSISTENCY_HASH, "missing if-match header/param");
    }

    if (sha1_validate_string_hashlike(sha1)) {
      BREAK_CODE(SS_INVALID_CONSISTENCY_HASH, "invalid if-match header/param");
    }

    if (strncmp(sha1, ses->consistency_hash, HASHSTRING_LENGTH)) {
      BREAK_CODE(SS_INVALID_CONSISTENCY_HASH, "if-match header/param does not match session consistency hash");
    }

    // parse answers

    int del_prev = 0;
    char *uid = fcgi_request_get_field_value(KEY_QUESTION_ID, req);

    if (!uid) {
      del_prev = 1;
      struct answer *last = session_get_last_given_answer(ses);
      uid = (last) ? last->uid : NULL;

      if(!uid) {
        BREAK_CODE(SS_OK, "(pass) deletion previous answer on an empty or fully deleted session");
      }
    }

    // delete answers

    LOG_INFOV("preparing to delete answers: mode: '%s', uid: '%s'", (del_prev) ? "previous" : "until", uid);

    res = validate_session_delete_answer(ses, uid);
    BREAK_IF(res != SS_OK, res, NULL);

    *affected_count = session_delete_answer(ses, uid);
    if (affected_count < 0) {
      // TODO could be both 400 or 500 (storage, serialization, not in session)
      BREAK_CODEV(SS_INVALID_ANSWER, "Failed to add answer", uid);
    }

  } while (0);

  return retVal;
}

/**
 * page handler /index
 * #260
 */
static void fcgi_page_index(struct kreq *req) {
  int retVal = 0;

  enum actions action;

  do {
    LOG_INFOV("Entering page handler: '%s' '%s'", kmethods[req->method], req->fullpath);
    BREAK_IF(req == NULL, SS_ERROR_ARG, "req");

    // validate allowed methods #260, #461

    switch (req->method) {
      case KMETHOD_HEAD:
      case KMETHOD_GET:
        action = ACTION_NONE;
      break;

      default:
        action = ACTION_MAX;
    }

    LOG_INFOV("action: '%s'", session_action_names[action]);

    if (action < 0) {
      BREAK_CODE(SS_INVALID_METHOD, NULL);
    }

    // #398 add root page, for now return 405 (not implemented)
    if (http_open(req, KHTTP_204, KMIME_TEXT_PLAIN, NULL)) {
      BREAK_ERROR("http_open(): unable to initialise http response");
    }
    khttp_puts(req, NULL);

    LOG_INFO("Leaving page handler");
  } while (0);

  // error response
  if (retVal) {
    fcgi_error_response(req, retVal);
  }

  (void)retVal;
  return;
}

/**
 * page handler /session (create, delete: not implemented)
 * #260
 */
static void fcgi_page_session(struct kreq *req) {
  int retVal = 0;

  struct session_meta *meta = NULL;
  struct session *ses = NULL;

  enum actions action;
  char session_id[40];

  char *survey_id = NULL;
  char *param = NULL;
  int res;

  do {
    LOG_INFOV("Entering page handler: '%s' '%s'", kmethods[req->method], req->fullpath);

    BREAK_IF(req == NULL, SS_ERROR_ARG, "req");

    // validate allowed methods #260, #461

    switch (req->method) {
      case KMETHOD_HEAD:
      case KMETHOD_GET:
      case KMETHOD_POST:
        action = ACTION_SESSION_NEW;
      break;

      default:
        action = ACTION_MAX;
    }

    LOG_INFOV("action: '%s'", session_action_names[action]);

    if (action == ACTION_MAX) {
      BREAK_CODE(SS_INVALID_METHOD, NULL);
    }

    // survey id

    survey_id = fcgi_request_get_field_value(KEY_SURVEY_ID, req);
    if (validate_survey_id(survey_id)) {
      BREAK_CODEV(SS_INVALID_SURVEY_ID, "survey: '%s'", (survey_id) ? survey_id : "(null)");
    }

    // request meta(#363)

    meta = fcgi_request_parse_meta(req);
    if (!meta) {
      BREAK_CODEV(SS_SYSTEM_LOAD_SESSION_META, "session: '%s'", session_id);
    }
    res = fcgi_request_validate_session_idendity(req, meta);
    if (res) {
      BREAK_CODEV(res, "session: '%s'", session_id);
    }

    switch (req->method) {
      // fetch param session_id
      case KMETHOD_POST:
      case KMETHOD_HEAD:
        param = fcgi_request_get_field_value(KEY_SESSION_ID, req);
      break;

      // nothing, create session id
      default:
      break;
    }

    if (param) {
        if (validate_session_id(param)) {
          BREAK_CODEV(SS_INVALID_SESSION_ID, "session: '%s'", (param) ? param : "(null)");
        }
        strncpy(session_id, param, 40);
    } else {
        if (create_session_id(session_id, 40)) {
          BREAK_CODE(SS_ERROR, "Unable to create session id");
        }
    }

    int res = session_exists(session_id);
    if (res != SS_NOSUCH_SESSION) {
      BREAK_CODE(res, NULL);
    }

    // #494  HEAD request: do not create and exit
    if (req->method == KMETHOD_HEAD) {
      if (http_open(req, KHTTP_200, KMIME_APP_JSON, NULL)) {
        BREAK_ERROR("http_open(): unable to initialise http response");
      }
      khttp_puts(req, NULL);
      LOG_INFO("Leaving page handler.");
      break;
    }

    // create session (also adds and stores next_questions)

    ses = create_session(survey_id, session_id, meta, &res);
    if (!ses) {
      BREAK_CODE(res, "failed to create session");
    }

    // response

    if (http_open(req, KHTTP_200, KMIME_TEXT_PLAIN, ses->consistency_hash)) {
      BREAK_ERROR("http_open(): unable to initialise http response");
    }

    enum kcgi_err er = khttp_puts(req, session_id);
    if (er != KCGI_OK) {
      BREAK_ERROR("khttp_puts() failed");
    }

    LOG_INFO("Leaving page handler.");
  } while (0);

  // destruct
  free_session_meta(meta);
  free_session(ses);

  if (retVal) {
    fcgi_error_response(req, retVal);
  }

  // error response
  (void)retVal;
  return;
}

/**
 * page handler /questions (get next questions)
 * #260
 */
static void fcgi_page_questions(struct kreq *req) {
  int retVal = 0;

  struct session *ses = NULL;
  struct nextquestions *nq = NULL;
  enum actions action;
  int res;

  do {
    LOG_INFOV("Entering page handler: '%s' '%s'", kmethods[req->method], req->fullpath);

    BREAK_IF(req == NULL, SS_ERROR_ARG, "req");

    // validate allowed methods #260, #461

    switch (req->method) {
      case KMETHOD_HEAD:
      case KMETHOD_GET:
        action = ACTION_SESSION_NEXTQUESTIONS;
      break;

      default:
        action = ACTION_MAX;
    }

    LOG_INFOV("action: '%s'", session_action_names[action]);

    if (action == ACTION_MAX) {
      BREAK_CODE(SS_INVALID_METHOD, NULL);
    }

    // get session

    ses = fcgi_request_load_and_verify_session(req, action, &res);
    if (!ses) {
      BREAK_CODE(res, "failed to load session");
    }

    // next_questions #332

    nq = get_next_questions(ses, action, 0);
    if (!nq) {
      BREAK_CODE(SS_SYSTEM_GET_NEXTQUESTIONS, "failed to get next questions'");
    }

    // #494  HEAD request: do not create and exit
    if (req->method == KMETHOD_HEAD) {
      if (http_open(req, KHTTP_200, KMIME_APP_JSON, ses->consistency_hash)) {
        BREAK_ERROR("http_open(): unable to initialise http response");
      }
      khttp_puts(req, NULL);
      LOG_INFO("Leaving page handler.");
      break;
    }

    // save session (ses->next_questions)

    res = save_session(ses);
    if (res) {
      BREAK_CODE(res, "save_session failed");
    }

    // response

    res = fcgi_response_nextquestion(req, ses, nq);
    if (res) {
      BREAK_CODE(res, "Could write page response (next questions)");
    }

    LOG_INFO("Leaving page handler.");
  } while (0);

  // destruct
  free_session(ses);
  free_next_questions(nq);

  if (retVal) {
    fcgi_error_response(req, retVal);
  }

  // error response
  (void)retVal;
  return;
}

/**
 * page handler /answers (add, delete)
 * #260
 */
static void fcgi_page_answers(struct kreq *req) {
  int retVal = 0;

  struct session *ses = NULL;
  struct answer *ans = NULL;
  struct nextquestions *nq = NULL;

  enum actions action;
  int affected_count = 0;
  int res;

  do {
    LOG_INFOV("Entering page handler: '%s' '%s'", kmethods[req->method], req->fullpath);
    BREAK_IF(req == NULL, SS_ERROR_ARG, "req");

    // validate allowed methods #260, #461

    switch (req->method) {
      case KMETHOD_HEAD:
      case KMETHOD_PUT:
      case KMETHOD_POST:
        action = ACTION_SESSION_ADDANSWER;
      break;

      case KMETHOD_DELETE:
        action = ACTION_SESSION_DELETEANSWER;
      break;

      default:
        action = ACTION_MAX;
    }

    LOG_INFOV("action: '%s'", session_action_names[action]);

    if (action == ACTION_MAX) {
      BREAK_CODE(SS_INVALID_METHOD, NULL);
    }

    // get session

    ses = fcgi_request_load_and_verify_session(req, action, &res);
    if (!ses) {
      BREAK_CODE(res, "failed to load session");
    }

    // dispatch action:

    //    POST   /answers?sessionid=<session_id>&answer=q1:a1:0:0:0:0:0:0:0
    //    POST   /answers?sessionid=<session_id> -d "q1:a1:0:0:0:0:0:0:0\nq2:a2:0:0:0:0:0:0:0"
    //    POST   /answers?sessionid=<session_id>&q1=a1
    //    POST   /answers?sessionid=<session_id>&q1=a1&q2=a1
    //    DELETE /answers?sessionid=<session_id>&uid=q1&sha=<consistency_sha>
    //    DELETE /answers?sessionid=<session_id>&sha=<consistency_sha> (previous anwers)

    switch (action) {
      case ACTION_SESSION_ADDANSWER:
        res = fcgi_session_add_answers(req, ses, &affected_count);
      break;

      case ACTION_SESSION_DELETEANSWER:
        res = fcgi_session_delete_answers(req, ses, &affected_count);
      break;

      default:
        res = SS_INVALID_ACTION;
    }

    if (res) {
      BREAK_CODEV(res, "failed to handle action '%s'", session_action_names[action]);
    }

    // next_questions

    nq = get_next_questions(ses, action, affected_count);
    if (!nq) {
      BREAK_CODE(SS_SYSTEM_GET_NEXTQUESTIONS, "failed to get next questions'");
    }

    // #494  HEAD request: do not create and exit
    if (req->method == KMETHOD_HEAD) {
      if (http_open(req, KHTTP_200, KMIME_APP_JSON, ses->consistency_hash)) {
        BREAK_ERROR("http_open(): unable to initialise http response");
      }
      khttp_puts(req, NULL);
      LOG_INFO("Leaving page handler.");
      break;
    }

    // save session

    res = save_session(ses);
    if (res) {
      BREAK_CODE(res, "save_session failed");
    }

    // response

    res = fcgi_response_nextquestion(req, ses, nq);
    if (res) {
      BREAK_CODE(res, "Could write page response (next questions)");
    }

    LOG_INFO("Leaving page handler.");
  } while (0);

  // destruct
  free_session(ses);
  free_answer(ans);
  free_next_questions(nq);

  // error response
  if (retVal) {
    fcgi_error_response(req, retVal);
  }

  (void)retVal;
  return;
}

/**
 * page handler /analysis (get)
 * #260
 */
static void fcgi_page_analysis(struct kreq *req) {
  enum kcgi_err er;
  int retVal = 0;

  struct session *ses = NULL;
  const char *analysis = NULL;

  enum actions action;
  int res;

  do {
    LOG_INFOV("Entering page handler: '%s' '%s'", kmethods[req->method], req->fullpath);
    BREAK_IF(req == NULL, SS_ERROR_ARG, "req");

    // validate allowed methods #260, #461

    switch (req->method) {
      case KMETHOD_HEAD:
      case KMETHOD_GET:
        action = ACTION_SESSION_ANALYSIS;
      break;

      default:
        action = ACTION_MAX;
    }

    LOG_INFOV("action: '%s'", session_action_names[action]);

    if (action == ACTION_MAX) {
      BREAK_CODE(SS_INVALID_METHOD, NULL);
    }

    // get session

    ses = fcgi_request_load_and_verify_session(req, action, &res);
    if (!ses) {
      BREAK_CODE(res, "failed to load session");
    }

    // get analysis

    // string is allocated into heap to since we have no control over the lifetime of the Python string.
    //    You need to free it
    res = get_analysis(ses, &analysis);
    if (res) {
      BREAK_CODE(SS_SYSTEM_GET_ANALYSIS, "get_analysis() failed");
    }

    if (!analysis) {
      BREAK_CODE(SS_SYSTEM_GET_ANALYSIS, "get_analysis() failed (null pointer)");
    }

    if (!analysis[0]) {
      BREAK_CODE(SS_SYSTEM_GET_ANALYSIS, "get_analysis() failed (empty)");
    }

    // save analysis

    res = session_add_datafile(ses->session_id, "analysis.json", analysis);
    if (res) {
      // do not break on error here, serve analysis to client in any case
      LOG_CODE(res, "Could not add analysis.json for session.");
    }

    // response

    if (http_open(req, KHTTP_200, KMIME_APP_JSON, ses->consistency_hash)) {
      BREAK_ERROR("http_open(): unable to initialise http response");
    }

    er = khttp_puts(req, analysis);
    if (er != KCGI_OK) {
      BREAK_ERROR("khttp_puts() failed");
    }

    LOG_INFO("Leaving page handler");
  } while (0);

  // destruct
  free_session(ses);
  freez((char *)analysis);

  if (retVal) {
    fcgi_error_response(req, retVal);
  }

  (void)retVal;
  return;
}

#define TEST_READ(X)                                                           \
  snprintf(failmsg, 16384, "Could not generate path ${SURVEY_HOME}/%s", X);    \
  if (generate_path(X, test_path, 8192)) {                                     \
    http_json_error(req, KHTTP_500, failmsg);                                  \
    break;                                                                     \
  }                                                                            \
  snprintf(failmsg, 16384,                                                     \
           "Could not open for reading path ${SURVEY_HOME}/%s", X);            \
  f = fopen(test_path, "r");                                                   \
  if (!f) {                                                                    \
    http_json_error(req, KHTTP_500, failmsg);                                  \
    break;                                                                     \
  }                                                                            \
  fclose(f);

#define TEST_WRITE(X)                                                          \
  snprintf(failmsg, 16384, "Could not generate path ${SURVEY_HOME}/%s", X);    \
  if (generate_path(X, test_path, 8192)) {                                     \
    http_json_error(req, KHTTP_500, failmsg);                                  \
    break;                                                                     \
  }                                                                            \
  f = fopen(test_path, "w");                                                   \
  if (!f) {                                                                    \
    snprintf(failmsg, 16384,                                                   \
             "Could not open for writing path %s, errno=%d (%s)", test_path,   \
             errno, strerror(errno));                                          \
    http_json_error(req, KHTTP_500, failmsg);                                  \
    break;                                                                     \
  }                                                                            \
  fclose(f);

#define TEST_MKDIR(X)                                                          \
  snprintf(failmsg, 16384, "Could not generate path ${SURVEY_HOME}/%s", X);    \
  if (generate_path(X, test_path, 8192)) {                                     \
    http_json_error(req, KHTTP_500, failmsg);                                  \
    break;                                                                     \
  }                                                                            \
  if (mkdir(test_path, 0777)) {                                                \
    snprintf(failmsg, 16384, "Could not mkdir path %s, errno=%d (%s)",         \
             test_path, errno, strerror(errno));                               \
    http_json_error(req, KHTTP_500, failmsg);                                  \
    break;                                                                     \
  }

#define TEST_REMOVE(X)                                                         \
  snprintf(failmsg, 16384, "Could not generate path ${SURVEY_HOME}/%s", X);    \
  if (generate_path(X, test_path, 8192)) {                                     \
    http_json_error(req, KHTTP_500, failmsg);                                  \
    break;                                                                     \
  }                                                                            \
  if (remove(test_path)) {                                                     \
    if (errno != ENOENT) {                                                     \
      snprintf(failmsg, 16384,                                                 \
               "Could not remove file for writing path %s, errno=%d (%s)",     \
               test_path, errno, strerror(errno));                             \
      http_json_error(req, KHTTP_500, failmsg);                                \
      break;                                                                   \
    }                                                                          \
  }

static void fcgi_page_check(struct kreq *req) {
  int retVal = 0;

  enum actions action;
  char *extended = NULL;

  do {
    LOG_INFOV("Entering page handler: '%s' '%s'", kmethods[req->method], req->fullpath);
    BREAK_IF(req == NULL, SS_ERROR_ARG, "req");

    // validate allowed methods #260, #461

    switch (req->method) {
      case KMETHOD_HEAD:
      case KMETHOD_GET:
        action = ACTION_NONE;
      break;

      default:
        action = ACTION_MAX;
    }

    LOG_INFOV("action: '%s'", session_action_names[action]);

    if (action == ACTION_MAX) {
      BREAK_CODE(SS_INVALID_METHOD, NULL);
    }

    if (req->method != KMETHOD_HEAD) {
      extended = fcgi_request_get_field_value(KEY_CHECK_EXTENDED, req); // check only pure existence, value doesn't matter
    }

    if (!extended) {
      if (http_open(req, KHTTP_200, KMIME_TEXT_PLAIN, NULL)) {
        BREAK_ERROR("http_open(): unable to initialise http response");
      }
      khttp_puts(req, NULL);
      break;
    }

    LOG_INFOV("proceeding with extended test (param KEY_CHECK_EXTENDED = '%s', extended", extended);

    char test_path[8192];
    char failmsg[16384];
    FILE *f = NULL;

    TEST_READ("");
    TEST_READ("surveys");

    // #84 cleanup previous tests
    TEST_REMOVE("surveys/testfile");
    TEST_REMOVE("surveys/testdir/testfile");
    TEST_REMOVE("surveys/testdir");
    TEST_REMOVE("sessions/testfile");
    TEST_REMOVE("sessions/testdir/testfile");
    TEST_REMOVE("sessions/testdir");

    // Then try to actually create the various files to test file system permissions and access
    TEST_WRITE("locks/testfile");
    TEST_WRITE("surveys/testfile");
    TEST_MKDIR("surveys/testdir");
    TEST_READ("surveys/testdir");
    TEST_WRITE("surveys/testdir/testfile");
    TEST_READ("sessions");
    TEST_WRITE("sessions/testfile");
    TEST_MKDIR("sessions/testdir");
    TEST_READ("sessions/testdir");
    TEST_WRITE("sessions/testdir/testfile");
    TEST_READ("logs");

    if (http_open(req, KHTTP_204, KMIME_TEXT_PLAIN, NULL)) {
      BREAK_ERROR("http_open(): unable to initialise http response");
    }
    khttp_puts(req, NULL);
    LOG_INFO("Leaving page handler.");

  } while (0);

  if (retVal) {
    fcgi_error_response(req, retVal);
  }

  (void)retVal;
  return;
}
