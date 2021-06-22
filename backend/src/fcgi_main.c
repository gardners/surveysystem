
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
  { kvalid_stringne, "answer" }
};

typedef void (*disp)(struct kreq *);

static void fcgi_index(struct kreq *);
static void fcgi_newsession(struct kreq *);
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
    fcgi_newsession,
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
    "newsession",
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
      LOG_ERROR("khttp_fcgi_init() failed.");
    }

    if (!fcgi) {
      LOG_ERROR("fcgi==NULL after call to khttp_fcgi_init()");
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

        http_open(&req, valid, req.mime, NULL);

      } else {

        if (KMETHOD_OPTIONS == req.method) {
            khttp_head(&req, kresps[KRESP_ALLOW], "OPTIONS HEAD GET POST");
            http_open(&req, KHTTP_200, req.mime, NULL);
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

static void fcgi_newsession(struct kreq *req) {
  int retVal = 0;

  struct session_meta *meta = NULL;
  struct session *ses = NULL;

  do {

    LOG_INFOV("Entering page handler: '%s' '%s'", kmethods[req->method], req->fullpath);

    char *survey_id = fcgi_request_get_field_value(KEY_SURVEYID, req);
    char *session_id = fcgi_request_get_field_value(KEY_SESSIONID, req); // POST only!
    char created_sessid[40] = { '\0' };

    if (validate_survey_id(survey_id)) {
      http_json_error(req, KHTTP_400, "Surveyid is invalid.");
      LOG_ERROR("Invalid survey id");
    }

    // #363 parse session meta
    meta = fcgi_request_parse_meta(req);
    if (!meta) {
      http_json_error(req, KHTTP_500, "Session could not be created (parse request meta).");
      LOG_ERROR("fcgi_request_parse_meta() failed");
    }

    // #363 validate session meta
    enum khttp status = fcgi_request_validate_meta_kreq(req, meta);
    if (status >= KHTTP_400) {
      http_json_error(req, status, "Invalid idendity provider check app configuration");
      LOG_ERRORV("fcgi_request_validate_meta_kreq() returned status %d >= KHTTP_400 (%d)", KHTTP_400, status);
    }

    if (KMETHOD_POST == req->method) {

      if (meta->provider != IDENDITY_HTTP_TRUSTED) {
        // only allowed for trusted middleware, so return 400
        http_json_error(req, KHTTP_400, "POST session_id only allowed in a managed system.");
        LOG_ERRORV("POST /newsession with invalid provider %d", meta->provider);
      }

      if (validate_session_id(session_id)) {
        // valid uid or return 400
        http_json_error(req, KHTTP_400, "Invalid session_id");
        LOG_ERROR("Invalid session_id");
      }

      char sess_path[1024];
      if(generate_session_path(session_id, session_id, sess_path, 1024)) {
        http_json_error(req, KHTTP_500, "Failed to create session path");
        LOG_ERROR("Failed to create session path");
      }

      if(!access(sess_path, F_OK)) {
        // valid uid or return 400
        http_json_error(req, KHTTP_400, "Session exists already");
        LOG_ERRORV("Session '%s' exists already", session_id);
      }

    } else {

      // #239, create new session id (separated out)
      if(create_session_id(created_sessid, 40)) {
        http_json_error(req, KHTTP_500, "Session could not be created (create session id).");
        LOG_ERROR("create session id failed");
      }
      session_id = created_sessid;

    } // if POST

    // #363 save session meta into session
    // #268 get ses->consistency_sha
    ses = create_session(survey_id, session_id, meta);
    if (!ses) {
      http_json_error(req, KHTTP_500, "Session could not be created (create session).");
      LOG_ERROR("create_session() failed");
    }

    // reply
    http_open(req, KHTTP_200, KMIME_TEXT_PLAIN, ses->consistency_hash);
    enum kcgi_err er = khttp_puts(req, session_id);
    if (er != KCGI_OK) {
      LOG_ERROR("khttp_puts() failed");
    }

    LOG_INFO("Leaving page handler");

  } while (0);

  // destruct
  free_session_meta(meta);
  free_session(ses);

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
      LOG_ERROR("Could not load session");
    }

    /*
     FILE *fp = open_log("session.log");
     dump_session(fp, ses);
    */

    // validate request against session meta (#363)
    enum khttp status = fcgi_request_validate_meta_session(req, ses);
    if (status != KHTTP_200) {
      http_json_error(req, status, "Invalid idendity provider, check app configuration");
      LOG_ERRORV("validate_session_meta_kreq() returned status %d != (%d)", KHTTP_200, status);
    }

    // validate requested action against session current state (#379)
    char reason[1024];
    if (validate_session_action(action, ses, reason, 1024)) {
      http_json_error(req, KHTTP_400, reason);
      LOG_ERROR("Session action validation failed");
    }

    ans = fcgi_request_load_answer(req);
    if (!ans) {
      http_json_error(req, KHTTP_400, "Could not load answer");
      LOG_ERROR("Could not load answer");
    }

    if (validate_session_add_answer(ses, ans, reason, 1024)) {
      http_json_error(req, KHTTP_400, reason);
      LOG_ERROR("Session action validation failed");
    }

    // #445 count affected answers
    int affected_count = session_add_answer(ses, ans);
    if (affected_count < 0) {
      http_json_error(req, KHTTP_400, "Invalid answer, could not add to session.");
      LOG_ERROR("session_add_answer() failed.");
    }

    // #332 next_questions data struct
    nq = get_next_questions(ses, action, affected_count);
    if (!nq) {
      http_json_error(req, KHTTP_500, "Unable to get next questions.");
      LOG_ERROR("get_next_questions() failed");
    }

    // joerg: break if session could not be updated
    if (save_session(ses)) {
      http_json_error(req, KHTTP_500, "Unable to update session.");
      LOG_ERROR("save_session failed()");
    }

    // All ok, so tell the caller the next question to be answered
    // #373, separate response handler for nextquestions
    if (fcgi_response_nextquestion(req, ses, nq)) {
      http_json_error(req, KHTTP_500, "Could not load next questions for specified session.");
      LOG_ERROR("Could not load next questions for specified session");
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
      LOG_ERROR("Could not load session");
    }

    // validate request against session meta (#363)
    enum khttp status = fcgi_request_validate_meta_session(req, ses);
    if (status != KHTTP_200) {
      http_json_error(req, status, "Invalid idendity provider, check app configuration");
      LOG_ERRORV("validate_session_meta_kreq() returned status %d != (%d)", KHTTP_200, status);
    }

    // validate requested action against session current state (#379)
    char reason[1024];
    if (validate_session_action(action, ses, reason, 1024)) {
      http_json_error(req, KHTTP_400, reason);
      LOG_ERROR("Session action validation failed");
    }

    ans = fcgi_request_load_answer(req);
    if (!ans) {
      http_json_error(req, KHTTP_400, "Could not load answer");
      LOG_ERROR("Could not load answer");
    }

    if (validate_session_add_answer(ses, ans, reason, 1024)) {
      http_json_error(req, KHTTP_400, reason);
      LOG_ERROR("Session action validation failed");
    }

    // #445 count affected answers
    int deleted = session_delete_answer(ses, ans->uid);
    if (deleted < 0) {
      // TODO could be both 400 or 500 (storage, serialization, not in session)
      http_json_error(req, KHTTP_400, "Answer does not match existing session records.");
      LOG_ERROR("session_delete_answer() failed");
    }

    // #445 count affected answers
    int affected_count = session_add_answer(ses, ans);
    if (affected_count < 0) {
      http_json_error(req, KHTTP_400, "Invalid answer, could not add to session.");
      LOG_ERROR("session_add_answer() failed.");
    }

    // #332 next_questions data struct
    nq = get_next_questions(ses, action, affected_count);
    if (!nq) {
      http_json_error(req, KHTTP_500, "Unable to get next questions.");
      LOG_ERROR("get_next_questions() failed");
    }

    if (save_session(ses)) {
      http_json_error(req, KHTTP_500, "Unable to update session.");
      LOG_ERROR("save_session() failed");
    }

    // All ok, so tell the caller the next question to be answered
    // #373, separate response handler for nextquestions
    if (fcgi_response_nextquestion(req, ses, nq)) {
      http_json_error(req, KHTTP_500, "Could not load next questions for specified session.");
      LOG_ERROR("Could not load next questions for specified session.");
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

    char *question_id = fcgi_request_get_field_value(KEY_QUESTIONID, req);

    ses = fcgi_request_load_session(req);
    if (!ses) {
      http_json_error(req, KHTTP_400, "Could not load specified session. Does it exist?");
      LOG_ERROR("Could not load session");
    }

    if (validate_session_delete_answer(question_id, ses)) {
      http_json_error(req, KHTTP_400, "Could load answer. Does it exist?");
      LOG_ERRORV("Could not load answer '%s'", question_id);
    }

    // validate request against session meta (#363)
    enum khttp status = fcgi_request_validate_meta_session(req, ses);
    if (status != KHTTP_200) {
      http_json_error(req, status, "Invalid idendity provider, check app configuration");
      LOG_ERRORV("validate_session_meta_kreq() returned status %d != (%d)", KHTTP_200, status);
    }

    // validate requested action against session current state (#379)
    char reason[1024];
    if (validate_session_action(action, ses, reason, 1024)) {
      http_json_error(req, KHTTP_400, reason);
      LOG_ERROR("Session action validation failed");
    }

    // We have a question -- so delete all answers to the given question

    // #445 count affected answers
    int affected_count = session_delete_answer(ses, question_id);
    if (affected_count < 0) {
      // TODO could be both 400 or 500 (storage, serialization, not in session)
      http_json_error(req, KHTTP_400, "Answer does not match existing session records.");
      LOG_ERROR("session_delete_answer() failed");
    }

    // #332 next_questions data struct
    nq = get_next_questions(ses, action, affected_count);
    if (!nq) {
      http_json_error(req, KHTTP_500, "Unable to get next questions.");
      LOG_ERROR("get_next_questions() failed");
    }

    if (save_session(ses)) {
      http_json_error(req, KHTTP_500, "Unable to update session.");
      LOG_ERROR("save_session() failed");
    }

    // All ok, so tell the caller the next question to be answered
    // #373, separate response handler for nextquestions
    if (fcgi_response_nextquestion(req, ses, nq)) {
      http_json_error(req, KHTTP_500, "Could not load next questions for specified session.");
      LOG_ERROR("Could not load next questions for specified session");
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
      LOG_ERROR("request header 'If-Match' is missing.");
    }

    // (weak) valiation if header->val is hash like string
    if (sha1_validate_string_hashlike(header->val)) {
      http_json_error(req, KHTTP_400, "request header 'If-Match' is invalid.");
      LOG_ERROR("request header 'If-Match' is invalid.");
    }

    ses = fcgi_request_load_session(req);
    if (!ses) {
      http_json_error(req, KHTTP_400, "Could not load specified session. Does it exist?");
      LOG_ERROR("Could not load session");
    }

    // validate request against session meta (#363)
    enum khttp status = fcgi_request_validate_meta_session(req, ses);
    if (status != KHTTP_200) {
      http_json_error(req, status, "Invalid idendity provider, check app configuration");
      LOG_ERRORV("validate_session_meta_kreq() returned status %d != (%d)", KHTTP_200, status);
    }

    // validate requested action against session current state (#379)
    //  - get last answer (not a system answer and not deleted)
    //  - define validation scope: if a last answer was found (to be flagged as deleted) use DELTEANSWER. Otherwise use the lower NEXTQUESTIONS
    struct answer *last = session_get_last_given_answer(ses);
    int validate_action = (last) ? action : ACTION_SESSION_NEXTQUESTIONS;

    char reason[1024];
    if (validate_session_action(validate_action, ses, reason, 1024)) {
      http_json_error(req, KHTTP_400, reason);
      LOG_ERROR("Session action validation failed");
    }

    if (strncmp(header->val, ses->consistency_hash, HASHSTRING_LENGTH)) {
      http_json_error(req, KHTTP_412, "Request header 'If-Match' does not match");
      LOG_ERROR("checksum does not match session consistency hash!");
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
      LOG_ERROR("session_delete_answer() failed");
    }

    // #332 next_questions data struct
    nq = get_next_questions(ses, action, affected_count);
    if (!nq) {
      http_json_error(req, KHTTP_500, "Unable to get next questions.");
      LOG_ERROR("get_next_questions() failed");
    }

    if (save_session(ses)) {
      http_json_error(req, KHTTP_500, "Unable to update session.");
      LOG_ERROR("save_session() failed");
    }

    // All ok, so tell the caller the next question to be answered
    // #373, separate response handler for nextquestions
    if (fcgi_response_nextquestion(req, ses, nq)) {
      http_json_error(req, KHTTP_500, "Could not load next questions for specified session.");
      LOG_ERROR("Could not load next questions for specified session");
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
      LOG_ERROR("Could not load session");
    }

    // validate request against session meta (#363)
    enum khttp status = fcgi_request_validate_meta_session(req, ses);
    if (status != KHTTP_200) {
      http_json_error(req, status, "Invalid idendity provider, check app configuration");
      LOG_ERRORV("validate_session_meta_kreq() returned status %d != (%d)", KHTTP_200, status);
    }

    // validate requested action against session current state (#379)
    char reason[1024];
    if (validate_session_action(action, ses, reason, 1024)) {
      http_json_error(req, KHTTP_400, reason);
      LOG_ERROR("Session action validation failed");
    }

    if (delete_session(ses->session_id)) {
      http_json_error(req, KHTTP_400, "Could not delete session. Does it exist?");
      LOG_ERROR("delete_session() failed");
    }

    // reply
    http_open(req, KHTTP_200, KMIME_TEXT_PLAIN, NULL);
    enum kcgi_err er = khttp_puts(req, "Session deleted");
    if (er != KCGI_OK) {
      LOG_ERROR("khttp_puts() failed");
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
      LOG_ERROR("Could not load session");
    }

    // validate request against session meta (#363)
    enum khttp status = fcgi_request_validate_meta_session(req, ses);
    if (status != KHTTP_200) {
      http_json_error(req, status, "Invalid idendity provider, check app configuration");
      LOG_ERRORV("validate_session_meta_kreq() returned status %d != (%d)", KHTTP_200, status);
    }

    // validate requested action against session current state (#379)
    char reason[1024];
    if (validate_session_action(action, ses, reason, 1024)) {
      http_json_error(req, KHTTP_400, reason);
      LOG_ERROR("Session action validation failed");
    }

    // #332 next_questions data struct
    nq = get_next_questions(ses, action, 0);
    if (!nq) {
      http_json_error(req, KHTTP_500, "Unable to get next questions.");
      LOG_ERROR("get_next_questions() failed");
    }

    if (save_session(ses)) {
      http_json_error(req, KHTTP_500, "Unable to update session.");
      LOG_ERROR("save_session() failed");
    }

    // All ok, so tell the caller the next question to be answered
    // #373, separate response handler for nextquestions
    if (fcgi_response_nextquestion(req, ses, nq)) {
      http_json_error(req, KHTTP_500, "Could not load next questions for specified session.");
      LOG_ERROR("Could not load next questions for specified session.");
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
      LOG_ERROR("Could not load session");
      break;
    }

    // validate request against session meta (#363)
    enum khttp status = fcgi_request_validate_meta_session(req, ses);
    if (status != KHTTP_200) {
      http_json_error(req, status, "Invalid idendity provider, check app configuration");
      LOG_ERRORV("validate_session_meta_kreq() returned status %d != (%d)", KHTTP_200, status);
    }

    // validate requested action against session current state (#379)
    char reason[1024];
    if (validate_session_action(action, ses, reason, 1024)) {
      http_json_error(req, KHTTP_400, reason);
      LOG_ERROR("Session action validation failed");
    }

    // String is allocated into heap to since we have no control over the lifetime of the Python string.
    // You need to free it
    if (get_analysis(ses, &analysis)) {
      http_json_error(req, KHTTP_500, "Could not retrieve analysis.");
      LOG_ERROR("get_analysis() failed");
    }

    if (!analysis) {
      http_json_error(req, KHTTP_500, "Could not retrieve analysis (NULL).");
      LOG_ERROR("get_analysis() returned NULL result");
    }

    if (!analysis[0]) {
      http_json_error(req, KHTTP_500, "Could not retrieve analysis (empty result).");
      LOG_ERROR("get_analysis() returned empty result");
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
      LOG_ERROR("khttp_puts() failed");
    }

    LOG_INFO("Leaving page handler");

  } while (0);

  // destruct
  free_session(ses);
  freez((char *)analysis);

  (void)retVal;

  return;
}
