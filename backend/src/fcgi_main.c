
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
  { kvalid_stringne, "if-match" }
};

typedef void (*disp)(struct kreq *);

static void fcgi_index(struct kreq *);
static void fcgi_page_session(struct kreq *);
static void fcgi_page_answers(struct kreq *);
static void fcgi_addanswer(struct kreq *);
static void fcgi_updateanswer(struct kreq *);
static void fcgi_nextquestion(struct kreq *);
static void fcgi_delanswer(struct kreq *);
static void fcgi_delprevanswer(struct kreq *);
static void fcgi_delsession(struct kreq *);
static void fcgi_accesstest(struct kreq *);
static void fcgi_fastcgitest(struct kreq *);
static void fcgi_analyse(struct kreq *);

static enum khttp sanitise_page_request(const struct kreq *req);

static const disp disps[PAGE__MAX] = {
    fcgi_index,
    fcgi_page_session,
    fcgi_page_answers,
    fcgi_addanswer,
    fcgi_updateanswer,
    fcgi_nextquestion,
    fcgi_delanswer,
    fcgi_delprevanswer,
    fcgi_delsession,
    fcgi_accesstest,
    fcgi_fastcgitest,
    fcgi_analyse
};

static const char *const pages[PAGE__MAX] = {
    "index",
    "session",
    "answers",
    "addanswer",
    "updateanswer",
    "nextquestion",
    "delanswer",
    "delprevanswer",
    "delsession",
    "accesstest",
    "fastcgitest",
    "analyse"
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

      // Clear our internal error log
      clear_errors();

      // Parse request
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
      enum khttp valid = sanitise_page_request(&req);
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
    }

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
static enum khttp sanitise_page_request(const struct kreq *req) {

    if (!req) {
        return KHTTP_500;
    }
    // man khttp_parse(3): The page index found by looking up pagename in the pages array. If pagename is not found in pages, pagesz is used; if pagename is empty, defpage is used.
    if(req->page >= PAGE__MAX) {
        return KHTTP_404;
    }

    // TODO, add mime type checks
    return KHTTP_200;
}

static void fcgi_index(struct kreq *req) {
  int retVal = 0;
  do {
    LOG_INFOV("Entering page handler: '%s' '%s'", kmethods[req->method], req->fullpath);

    // #398 add root page
    http_json_error(req, KHTTP_405, "Not implemented");

    LOG_INFO("Leaving page handler");
  } while (0);

  (void)retVal;
  return;
}

/**
 * Parse request payload and add  desrerialised answers to session
 * #260
 * PARSE_ANSWER_SERIALISED,
 * PARSE_ANSWER_VALUES,
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

int fcgi_session_delete_answers(struct kreq *req, struct session *ses, int *affected_count) {
  int retVal = 0;

  do {
    BREAK_IF(req == NULL, SS_ERROR_ARG, "req");
    BREAK_IF(ses == NULL, SS_ERROR_ARG, "ses");

    *affected_count = 0;
    int res;

    // validate If-Match (consistency sha)
    char *sha1 = fcgi_request_get_consistency_hash(req);
    if (!sha1) {
      BREAK_CODE(SS_INVALID_CONSISTENCY_HASH, "missing if-match header/param");
    }

    if (sha1_validate_string_hashlike(sha1)) {
      BREAK_CODE(SS_INVALID_CONSISTENCY_HASH, "invalid if-match header/param");
    }

    // determine parser
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
 * #260, #461
 * POST /answers: add answer(s)
 * HEAD /answers prevalidate adding answer(s)
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
    // ACTION__NONE indicates undefined action
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
        action = ACTION_NONE;
    }

    LOG_INFOV("action: '%s'", session_action_names[action]);

    if (action == ACTION_NONE) {
      BREAK_CODE(SS_INVALID_METHOD, NULL);
    }

    ses = fcgi_request_load_and_verify_session(req, action, &res);
    if (!ses) {
      BREAK_CODE(res, "failed to load session");
    }

    // dispatcher:
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

    // #332 next_questions data struct
    nq = get_next_questions(ses, action, affected_count);
    if (!nq) {
      BREAK_CODE(SS_SYSTEM_GET_NEXTQUESTIONS, "failed to get next questions'");
    }

    // #494  HEAD request: do not create and exit
    if (req->method == KMETHOD_HEAD) {
      http_open(req, KHTTP_200, KMIME_APP_JSON, ses->consistency_hash);
      khttp_puts(req, NULL);
      LOG_INFO("Leaving page handler.");
      break;
    }

    // joerg: break if session could not be updated
    res = save_session(ses);
    if (res) {
      BREAK_CODE(res, "save_session failed");
    }

    // All ok, so tell the caller the next question to be answered
    // #373, separate response handler for nextquestions
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
 * #260, #461
 * POST /answers: add answer(s)
 * HEAD /answers prevalidate adding answer(s)
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
    // ACTION__NONE indicates undefined action
    switch (req->method) {
      case KMETHOD_HEAD:
      case KMETHOD_GET:
      case KMETHOD_POST:
        action = ACTION_SESSION_NEW;
      break;

      default:
        action = ACTION_NONE;
    }

    LOG_INFOV("action: '%s'", session_action_names[action]);

    if (action == ACTION_NONE) {
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

    // session_id
    switch (req->method) {
      // create new session id
      case KMETHOD_GET:
        if (create_session_id(session_id, 40)) {
          BREAK_CODE(SS_ERROR, "Unable to create session id");
        }
      break;

      // fetch param session_id
      case KMETHOD_POST:
        param = fcgi_request_get_field_value(KEY_SESSION_ID, req);
        if (validate_session_id(param)) {
          BREAK_CODEV(SS_INVALID_SESSION_ID, "session: '%s'", (param) ? param : "(null)");
        }
        strncpy(session_id, param, 40);
      break;

      // (HEAD) nothing
      default:
      break;
    }

    if (retVal) {
      break;
    }

    // #494  HEAD request: do not create and exit
    if (req->method == KMETHOD_HEAD) {
      http_open(req, KHTTP_200, KMIME_APP_JSON, ses->consistency_hash);
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


static void fcgi_addanswer(struct kreq *req) {
  int retVal = 0;

  struct session *ses = NULL;
  struct answer *ans = NULL;
  struct nextquestions *nq = NULL;
  enum actions action = ACTION_SESSION_ADDANSWER;

  do {

    LOG_INFOV("Entering page handler: '%s' '%s'", kmethods[req->method], req->fullpath);

    ses = fcgi_request_load_session(req);
    if (!ses) {
      http_json_error(req, KHTTP_400, "Could not load specified session. Does it exist?");
      BREAK_ERROR("Could not load session");
    }

    /*
     FILE *fp = open_log("session.log");
     dump_session(fp, ses);
    */

    // validate request against session meta (#363)
    enum khttp status = fcgi_request_validate_meta_session(req, ses);
    if (status != KHTTP_200) {
      http_json_error(req, status, "Invalid idendity provider, check app configuration");
      BREAK_ERRORV("validate_session_meta_kreq() returned status %d != (%d)", KHTTP_200, status);
    }

    // validate requested action against session current state (#379)
    char reason[1024];
    if (validate_session_action(action, ses, reason, 1024)) {
      http_json_error(req, KHTTP_400, reason);
      BREAK_ERROR("Session action validation failed");
    }

    ans = fcgi_request_load_answer(req);
    if (!ans) {
      http_json_error(req, KHTTP_400, "Could not load answer");
      BREAK_ERROR("Could not load answer");
    }

    if (validate_session_add_answer(ses, ans)) {
      http_json_error(req, KHTTP_400, "Answer is invalid");
      BREAK_ERROR("Answer validation failed");
    }

    // #445 count affected answers
    int affected_count = session_add_answer(ses, ans);
    if (affected_count < 0) {
      http_json_error(req, KHTTP_400, "Invalid answer, could not add to session.");
      BREAK_ERROR("session_add_answer() failed.");
    }

    // #332 next_questions data struct
    nq = get_next_questions(ses, action, affected_count);
    if (!nq) {
      http_json_error(req, KHTTP_500, "Unable to get next questions.");
      BREAK_ERROR("get_next_questions() failed");
    }

    // joerg: break if session could not be updated
    if (save_session(ses)) {
      http_json_error(req, KHTTP_500, "Unable to update session.");
      BREAK_ERROR("save_session failed()");
    }

    // All ok, so tell the caller the next question to be answered
    // #373, separate response handler for nextquestions
    if (fcgi_response_nextquestion(req, ses, nq)) {
      http_json_error(req, KHTTP_500, "Could not load next questions for specified session.");
      BREAK_ERROR("Could not load next questions for specified session");
    }

    LOG_INFO("Leaving page handler.");

  } while (0);

  // destruct
  free_session(ses);
  free_answer(ans);
  free_next_questions(nq);

  (void)retVal;
  return;
}

static void fcgi_updateanswer(struct kreq *req) {
  int retVal = 0;

  struct session *ses = NULL;
  struct answer *ans = NULL;
  struct nextquestions *nq = NULL;
  enum actions action = ACTION_SESSION_ADDANSWER;

  do {

    LOG_INFOV("Entering page handler: '%s' '%s'", kmethods[req->method], req->fullpath);

    ses = fcgi_request_load_session(req);
    if (!ses) {
      http_json_error(req, KHTTP_400, "Could not load specified session. Does it exist?");
      BREAK_ERROR("Could not load session");
    }

    // validate request against session meta (#363)
    enum khttp status = fcgi_request_validate_meta_session(req, ses);
    if (status != KHTTP_200) {
      http_json_error(req, status, "Invalid idendity provider, check app configuration");
      BREAK_ERRORV("validate_session_meta_kreq() returned status %d != (%d)", KHTTP_200, status);
    }

    // validate requested action against session current state (#379)
    char reason[1024];
    if (validate_session_action(action, ses, reason, 1024)) {
      http_json_error(req, KHTTP_400, reason);
      BREAK_ERROR("Session action validation failed");
    }

    ans = fcgi_request_load_answer(req);
    if (!ans) {
      http_json_error(req, KHTTP_400, "Could not load answer");
      BREAK_ERROR("Could not load answer");
    }

    if (validate_session_add_answer(ses, ans)) {
      http_json_error(req, KHTTP_400, "Answer is invalid");
      BREAK_ERROR("Answer validation failed");
    }

    // #445 count affected answers
    int deleted = session_delete_answer(ses, ans->uid);
    if (deleted < 0) {
      // TODO could be both 400 or 500 (storage, serialization, not in session)
      http_json_error(req, KHTTP_400, "Answer does not match existing session records.");
      BREAK_ERROR("session_delete_answer() failed");
    }

    // #445 count affected answers
    int affected_count = session_add_answer(ses, ans);
    if (affected_count < 0) {
      http_json_error(req, KHTTP_400, "Invalid answer, could not add to session.");
      BREAK_ERROR("session_add_answer() failed.");
    }

    // #332 next_questions data struct
    nq = get_next_questions(ses, action, affected_count);
    if (!nq) {
      http_json_error(req, KHTTP_500, "Unable to get next questions.");
      BREAK_ERROR("get_next_questions() failed");
    }

    if (save_session(ses)) {
      http_json_error(req, KHTTP_500, "Unable to update session.");
      BREAK_ERROR("save_session() failed");
    }

    // All ok, so tell the caller the next question to be answered
    // #373, separate response handler for nextquestions
    if (fcgi_response_nextquestion(req, ses, nq)) {
      http_json_error(req, KHTTP_500, "Could not load next questions for specified session.");
      BREAK_ERROR("Could not load next questions for specified session.");
    }

    LOG_INFO("Leaving page handler.");

  } while (0);

  // destruct
  free_session(ses);
  free_answer(ans);
  free_next_questions(nq);

  (void)retVal;
  return;
}

static void fcgi_delanswer(struct kreq *req) {
  int retVal = 0;

  struct session *ses = NULL;
  struct nextquestions *nq = NULL;
  enum actions action = ACTION_SESSION_DELETEANSWER;

  do {

    LOG_INFOV("Entering page handler: '%s' '%s'", kmethods[req->method], req->fullpath);

    char *question_id = fcgi_request_get_field_value(KEY_QUESTION_ID, req);

    ses = fcgi_request_load_session(req);
    if (!ses) {
      http_json_error(req, KHTTP_400, "Could not load specified session. Does it exist?");
      BREAK_ERROR("Could not load session");
    }

    if (validate_session_delete_answer(ses, question_id)) {
      http_json_error(req, KHTTP_400, "Could load answer. Does it exist?");
      BREAK_ERRORV("Could not load answer '%s'", question_id);
    }

    // validate request against session meta (#363)
    enum khttp status = fcgi_request_validate_meta_session(req, ses);
    if (status != KHTTP_200) {
      http_json_error(req, status, "Invalid idendity provider, check app configuration");
      BREAK_ERRORV("validate_session_meta_kreq() returned status %d != (%d)", KHTTP_200, status);
    }

    // validate requested action against session current state (#379)
    char reason[1024];
    if (validate_session_action(action, ses, reason, 1024)) {
      http_json_error(req, KHTTP_400, reason);
      BREAK_ERROR("Session action validation failed");
    }

    // We have a question -- so delete all answers to the given question

    // #445 count affected answers
    int affected_count = session_delete_answer(ses, question_id);
    if (affected_count < 0) {
      // TODO could be both 400 or 500 (storage, serialization, not in session)
      http_json_error(req, KHTTP_400, "Answer does not match existing session records.");
      BREAK_ERROR("session_delete_answer() failed");
    }

    // #332 next_questions data struct
    nq = get_next_questions(ses, action, affected_count);
    if (!nq) {
      http_json_error(req, KHTTP_500, "Unable to get next questions.");
      BREAK_ERROR("get_next_questions() failed");
    }

    if (save_session(ses)) {
      http_json_error(req, KHTTP_500, "Unable to update session.");
      BREAK_ERROR("save_session() failed");
    }

    // All ok, so tell the caller the next question to be answered
    // #373, separate response handler for nextquestions
    if (fcgi_response_nextquestion(req, ses, nq)) {
      http_json_error(req, KHTTP_500, "Could not load next questions for specified session.");
      BREAK_ERROR("Could not load next questions for specified session");
    }
    LOG_INFO("Leaving page handler.");

  } while (0);

  // destruct
  free_session(ses);
  free_next_questions(nq);

  (void)retVal;
  return;
}

static void fcgi_delprevanswer(struct kreq *req) {
  int retVal = 0;

  struct session *ses = NULL;
  struct nextquestions *nq = NULL;
  enum actions action = ACTION_SESSION_DELETEANSWER;

  do {

    LOG_INFOV("Entering page handler: '%s' '%s'", kmethods[req->method], req->fullpath);

    struct khead *header = req->reqmap[KREQU_IF_MATCH];

    if (!header) {
      http_json_error(req, KHTTP_400, "request header 'If-Match' is missing.");
      BREAK_ERROR("request header 'If-Match' is missing.");
    }

    // (weak) valiation if header->val is hash like string
    if (sha1_validate_string_hashlike(header->val)) {
      http_json_error(req, KHTTP_400, "request header 'If-Match' is invalid.");
      BREAK_ERROR("request header 'If-Match' is invalid.");
    }

    ses = fcgi_request_load_session(req);
    if (!ses) {
      http_json_error(req, KHTTP_400, "Could not load specified session. Does it exist?");
      BREAK_ERROR("Could not load session");
    }

    // validate request against session meta (#363)
    enum khttp status = fcgi_request_validate_meta_session(req, ses);
    if (status != KHTTP_200) {
      http_json_error(req, status, "Invalid idendity provider, check app configuration");
      BREAK_ERRORV("validate_session_meta_kreq() returned status %d != (%d)", KHTTP_200, status);
    }

    // validate requested action against session current state (#379)
    //  - get last answer (not a system answer and not deleted)
    //  - define validation scope: if a last answer was found (to be flagged as deleted) use DELTEANSWER. Otherwise use the lower NEXTQUESTIONS
    struct answer *last = session_get_last_given_answer(ses);
    int validate_action = (last) ? action : ACTION_SESSION_NEXTQUESTIONS;

    char reason[1024];
    if (validate_session_action(validate_action, ses, reason, 1024)) {
      http_json_error(req, KHTTP_400, reason);
      BREAK_ERROR("Session action validation failed");
    }

    if (strncmp(header->val, ses->consistency_hash, HASHSTRING_LENGTH)) {
      http_json_error(req, KHTTP_412, "Request header 'If-Match' does not match");
      BREAK_ERROR("checksum does not match session consistency hash!");
    }

    LOG_INFOV("checksum match passed for session '%s'", ses->session_id);

    // #445 count affected answers
    int affected_count = -1;

    if (last) {
      LOG_INFOV("deleting last given answer '%s'", last->uid);
      affected_count = session_delete_answer(ses, last->uid);
    } else {
      LOG_INFO("no last given answer in session");
      affected_count = 0;
    }

    if (affected_count < 0) {
      // TODO could be both 400 or 500 (storage, serialization, not in session)
      http_json_error(req, KHTTP_400, "Answer does not match existing session records.");
      BREAK_ERROR("session_delete_answer() failed");
    }

    // #332 next_questions data struct
    nq = get_next_questions(ses, action, affected_count);
    if (!nq) {
      http_json_error(req, KHTTP_500, "Unable to get next questions.");
      BREAK_ERROR("get_next_questions() failed");
    }

    if (save_session(ses)) {
      http_json_error(req, KHTTP_500, "Unable to update session.");
      BREAK_ERROR("save_session() failed");
    }

    // All ok, so tell the caller the next question to be answered
    // #373, separate response handler for nextquestions
    if (fcgi_response_nextquestion(req, ses, nq)) {
      http_json_error(req, KHTTP_500, "Could not load next questions for specified session.");
      BREAK_ERROR("Could not load next questions for specified session");
    }

    LOG_INFO("Leaving page handler.");

  } while (0);

  // destruct
  free_session(ses);
  free_next_questions(nq);

  (void)retVal;
  return;
}

static void fcgi_delsession(struct kreq *req) {
  int retVal = 0;

  struct session *ses = NULL;
  enum actions action = ACTION_SESSION_DELETE;

  do {

    LOG_INFOV("Entering page handler: '%s' '%s'", kmethods[req->method], req->fullpath);

    ses = fcgi_request_load_session(req);
    if (!ses) {
      http_json_error(req, KHTTP_400, "Could not load specified session. Does it exist?");
      BREAK_ERROR("Could not load session");
    }

    // validate request against session meta (#363)
    enum khttp status = fcgi_request_validate_meta_session(req, ses);
    if (status != KHTTP_200) {
      http_json_error(req, status, "Invalid idendity provider, check app configuration");
      BREAK_ERRORV("validate_session_meta_kreq() returned status %d != (%d)", KHTTP_200, status);
    }

    // validate requested action against session current state (#379)
    char reason[1024];
    if (validate_session_action(action, ses, reason, 1024)) {
      http_json_error(req, KHTTP_400, reason);
      BREAK_ERROR("Session action validation failed");
    }

    if (delete_session(ses->session_id)) {
      http_json_error(req, KHTTP_400, "Could not delete session. Does it exist?");
      BREAK_ERROR("delete_session() failed");
    }

    // reply
    http_open(req, KHTTP_200, KMIME_TEXT_PLAIN, NULL);
    enum kcgi_err er = khttp_puts(req, "Session deleted");
    if (er != KCGI_OK) {
      BREAK_ERROR("khttp_puts() failed");
    }

    LOG_INFO("Leaving page handler");

  } while (0);

  // destruct
  free_session(ses);

  (void)retVal;
  return;
}

static void fcgi_nextquestion(struct kreq *req) {
  //  enum kcgi_err    er;
  int retVal = 0;

  struct session *ses = NULL;
  struct nextquestions *nq = NULL;
  enum actions action = ACTION_SESSION_NEXTQUESTIONS;

  do {

    LOG_INFOV("Entering page handler: '%s' '%s'", kmethods[req->method], req->fullpath);

    ses = fcgi_request_load_session(req);
    if (!ses) {
      http_json_error(req, KHTTP_400, "Could not load specified session. Does it exist?");
      BREAK_ERROR("Could not load session");
    }

    // validate request against session meta (#363)
    enum khttp status = fcgi_request_validate_meta_session(req, ses);
    if (status != KHTTP_200) {
      http_json_error(req, status, "Invalid idendity provider, check app configuration");
      BREAK_ERRORV("validate_session_meta_kreq() returned status %d != (%d)", KHTTP_200, status);
    }

    // validate requested action against session current state (#379)
    char reason[1024];
    if (validate_session_action(action, ses, reason, 1024)) {
      http_json_error(req, KHTTP_400, reason);
      BREAK_ERROR("Session action validation failed");
    }

    // #332 next_questions data struct
    nq = get_next_questions(ses, action, 0);
    if (!nq) {
      http_json_error(req, KHTTP_500, "Unable to get next questions.");
      BREAK_ERROR("get_next_questions() failed");
    }

    if (save_session(ses)) {
      http_json_error(req, KHTTP_500, "Unable to update session.");
      BREAK_ERROR("save_session() failed");
    }

    // All ok, so tell the caller the next question to be answered
    // #373, separate response handler for nextquestions
    if (fcgi_response_nextquestion(req, ses, nq)) {
      http_json_error(req, KHTTP_500, "Could not load next questions for specified session.");
      BREAK_ERROR("Could not load next questions for specified session.");
    }

    LOG_INFO("Leaving page handler.");
  } while (0);

  // destruct
  free_session(ses);
  free_next_questions(nq);

  (void)retVal;
  return;
}

#define TEST_READ(X)                                                           \
  snprintf(failmsg, 16384, "Could not generate path ${SURVEY_HOME}/%s", X);    \
  if (generate_path(X, test_path, 8192)) {                                     \
    http_json_error(req, KHTTP_500, failmsg);                                      \
    break;                                                                     \
  }                                                                            \
  snprintf(failmsg, 16384,                                                     \
           "Could not open for reading path ${SURVEY_HOME}/%s", X);            \
  f = fopen(test_path, "r");                                                   \
  if (!f) {                                                                    \
    http_json_error(req, KHTTP_500, failmsg);                                      \
    break;                                                                     \
  }                                                                            \
  fclose(f);

#define TEST_WRITE(X)                                                          \
  snprintf(failmsg, 16384, "Could not generate path ${SURVEY_HOME}/%s", X);    \
  if (generate_path(X, test_path, 8192)) {                                     \
    http_json_error(req, KHTTP_500, failmsg);                                      \
    break;                                                                     \
  }                                                                            \
  f = fopen(test_path, "w");                                                   \
  if (!f) {                                                                    \
    snprintf(failmsg, 16384,                                                   \
             "Could not open for writing path %s, errno=%d (%s)", test_path,   \
             errno, strerror(errno));                                          \
    http_json_error(req, KHTTP_500, failmsg);                                      \
    break;                                                                     \
  }                                                                            \
  fclose(f);

#define TEST_MKDIR(X)                                                          \
  snprintf(failmsg, 16384, "Could not generate path ${SURVEY_HOME}/%s", X);    \
  if (generate_path(X, test_path, 8192)) {                                     \
    http_json_error(req, KHTTP_500, failmsg);                                      \
    break;                                                                     \
  }                                                                            \
  if (mkdir(test_path, 0777)) {                                                \
    snprintf(failmsg, 16384, "Could not mkdir path %s, errno=%d (%s)",         \
             test_path, errno, strerror(errno));                               \
    http_json_error(req, KHTTP_500, failmsg);                                      \
    break;                                                                     \
  }

#define TEST_REMOVE(X)                                                         \
  snprintf(failmsg, 16384, "Could not generate path ${SURVEY_HOME}/%s", X);    \
  if (generate_path(X, test_path, 8192)) {                                     \
    http_json_error(req, KHTTP_500, failmsg);                                      \
    break;                                                                     \
  }                                                                            \
  if (remove(test_path)) {                                                     \
    if (errno != ENOENT) {                                                     \
      snprintf(failmsg, 16384,                                                 \
               "Could not remove file for writing path %s, errno=%d (%s)",     \
               test_path, errno, strerror(errno));                             \
      http_json_error(req, KHTTP_500, failmsg);                                    \
      break;                                                                   \
    }                                                                          \
  }

static void fcgi_accesstest(struct kreq *req) {
  // Try to access paths, and report status.

  do {

    LOG_INFOV("Entering page handler: '%s' '%s'", kmethods[req->method], req->fullpath);

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

    http_json_error(req, KHTTP_200, "All okay.");

    LOG_INFO("Leaving page handler.");

  } while (0);

  return;
}

static void fcgi_fastcgitest(struct kreq *req) {
  do {

    LOG_INFOV("Entering page handler: '%s' '%s'", kmethods[req->method], req->fullpath);

    http_json_error(req, KHTTP_200, "All okay.");

    LOG_INFO("Leaving page handler.");

  } while (0);
  return;
}

/**
 * fetch analysis json
 * #300 TODO deal with planned get_analysysis_generic() **output
 */
static void fcgi_analyse(struct kreq *req) {
  enum kcgi_err er;
  int retVal = 0;

  struct session *ses = NULL;
  const char *analysis = NULL;
  enum actions action = ACTION_SESSION_ANALYSIS;

  do {

    LOG_INFOV("Entering page handler: '%s' '%s'", kmethods[req->method], req->fullpath);

    ses = fcgi_request_load_session(req);
    if (!ses) {
      http_json_error(req, KHTTP_400, "Could not load specified session. Does it exist?");
      BREAK_ERROR("Could not load session");
    }

    // validate request against session meta (#363)
    enum khttp status = fcgi_request_validate_meta_session(req, ses);
    if (status != KHTTP_200) {
      http_json_error(req, status, "Invalid idendity provider, check app configuration");
      BREAK_ERRORV("validate_session_meta_kreq() returned status %d != (%d)", KHTTP_200, status);
    }

    // validate requested action against session current state (#379)
    char reason[1024];
    if (validate_session_action(action, ses, reason, 1024)) {
      http_json_error(req, KHTTP_400, reason);
      BREAK_ERROR("Session action validation failed");
    }

    // String is allocated into heap to since we have no control over the lifetime of the Python string.
    // You need to free it
    if (get_analysis(ses, &analysis)) {
      http_json_error(req, KHTTP_500, "Could not retrieve analysis.");
      BREAK_ERROR("get_analysis() failed");
    }

    if (!analysis) {
      http_json_error(req, KHTTP_500, "Could not retrieve analysis (NULL).");
      BREAK_ERROR("get_analysis() returned NULL result");
    }

    if (!analysis[0]) {
      http_json_error(req, KHTTP_500, "Could not retrieve analysis (empty result).");
      BREAK_ERROR("get_analysis() returned empty result");
    }

    // store analysis with session
    if (session_add_datafile(ses->session_id, "analysis.json", analysis)) {
      LOG_WARNV("Could not add analysis.json for session.", 0);
      // do not break here
    }

    // reply, #268 add consistency hash
    http_open(req, KHTTP_200, KMIME_APP_JSON, ses->consistency_hash);

    er = khttp_puts(req, analysis);
    if (er != KCGI_OK) {
      BREAK_ERROR("khttp_puts() failed");
    }

    LOG_INFO("Leaving page handler");

  } while (0);

  // destruct
  free_session(ses);
  freez((char *)analysis);

  (void)retVal;

  return;
}
