
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
#include "utils.h"

#include "fcgirequest.h"

#define CHECKPOINT()                                                           \
  {                                                                            \
    fprintf(stderr, "%s:%d:%s():pid=%d: Checkpoint\n", __FILE__, __LINE__,     \
            __FUNCTION__, getpid());                                           \
  }

int kvalid_surveyid(struct kpair *kp) {

  // XXX - This process runs in the sandboxed child environment, from where
  // it is not possible to emit any log output.  Very annoying, but we have
  // to live with it.
  LOG_MUTE();

  int retVal = 0;
  do {
    if (!kp)
      LOG_ERROR("kp is NULL");

    // Only use our validation here, not one of the pre-defined ones
    kp->type = KPAIR__MAX;
    LOG_WARNV("Validating surveyid", 1);

    // Is okay
    kp->parsed.s = kp->val;
  } while (0);

  if (retVal) {
    retVal = 0;
  } else {
    retVal = 1;
  }

  LOG_UNMUTE();
  return retVal;
}

int kvalid_sessionid(struct kpair *kp) {
  // XXX - This process runs in the sandboxed child environment, from where
  // it is not possible to emit any log output.  Very annoying, but we have
  // to live with it.
  LOG_MUTE();

  int retVal = 0;
  do {
    if (!kp)
      LOG_ERROR("kp is NULL");

    // Only use our validation here, not one of the pre-defined ones
    kp->type = KPAIR__MAX;

    kp->parsed.s = kp->val;
    LOG_WARNV("Validating sessionid", 1);
    if (validate_session_id(kp->val)) {
      LOG_ERROR("validate_session_id failed");
    }
  } while (0);

  if (retVal) {
    retVal = 0;
  } else {
    retVal = 1;
  }

  LOG_UNMUTE();
  return retVal;
}

int kvalid_questionid(struct kpair *kp) {
  // XXX - This process runs in the sandboxed child environment, from where
  // it is not possible to emit any log output.  Very annoying, but we have
  // to live with it.
  LOG_MUTE();

  int retVal = 0;
  do {
    if (!kp)
      LOG_ERROR("kp is NULL");
    // Only use our validation here, not one of the pre-defined ones
    kp->type = KPAIR__MAX;

    LOG_WARNV("Validating questionid", 1);

    kp->parsed.s = kp->val;
    // Is okay
    if (kvalid_string(kp)) {
      retVal = 0;
    } else {
      LOG_ERROR("questionid is not a valid string");
    }
  } while (0);

  if (retVal) {
    retVal = 0;
  } else {
    retVal = 1;
  }

  LOG_UNMUTE();
  return retVal;
}

int kvalid_answer(struct kpair *kp) {

  // XXX - This process runs in the sandboxed child environment, from where
  // it is not possible to emit any log output.  Very annoying, but we have
  // to live with it.
  LOG_MUTE();

  int retVal = 0;

  do {

    LOG_WARNV("Validating answer", 1);

    // Only use our validation here, not one of the pre-defined ones
    kp->type = KPAIR__MAX;

    kp->parsed.s = kp->val;

    struct answer *a = calloc(sizeof(struct answer), 1);
    if (!a) {
      LOG_ERROR("Could not calloc() answer structure.");
    }

    // TODO XXX Remember deserialised answer and keep it in memory to save parsing twice? alternatively just basic validation by counting delimiters

    if (deserialise_answer(kp->val, ANSWER_FIELDS_PUBLIC, a)) {
      free_answer(a);
      LOG_ERROR("deserialise_answer() failed");
    } else {
      free_answer(a);
      // Success, so nothing to do
    }
  } while (0);

  LOG_WARNV("retVal=%d", retVal);

  if (retVal) {
    retVal = 0;
  } else {
    retVal = 1;
  }

  LOG_UNMUTE();
  return retVal;
}

enum key { KEY_SURVEYID, KEY_SESSIONID, KEY_QUESTIONID, KEY_ANSWER, KEY__MAX };

static const struct kvalid keys[KEY__MAX] = {
  { kvalid_surveyid, "surveyid" },
  { kvalid_sessionid, "sessionid" },
  { kvalid_questionid, "questionid" },
  { kvalid_answer, "answer" }
};

enum page {
  PAGE_NEWSESSION,
  PAGE_ADDANSWER,
  PAGE_UPDATEANSWER,
  PAGE_NEXTQUESTION,
  PAGE_DELANSWER,
  PAGE_DELANSWERANDFOLLOWING,
  PAGE_DELSESSION,
  PAGE_ACCESTEST,
  PAGE_FCGITEST,
  PAGE_ANALYSE,
  PAGE__MAX
};

typedef void (*disp)(struct kreq *);

static void fcgi_newsession(struct kreq *);
static void fcgi_addanswer(struct kreq *);
static void fcgi_updateanswer(struct kreq *);
static void fcgi_nextquestion(struct kreq *);
static void fcgi_delanswer(struct kreq *);
static void fcgi_delanswerandfollowing(struct kreq *);
static void fcgi_delsession(struct kreq *);
static void fcgi_accesstest(struct kreq *);
static void fcgi_fastcgitest(struct kreq *);
static void fcgi_analyse(struct kreq *);

static const disp disps[PAGE__MAX] = {
    fcgi_newsession,   fcgi_addanswer,  fcgi_updateanswer,
    fcgi_nextquestion, fcgi_delanswer,  fcgi_delanswerandfollowing,
    fcgi_delsession,   fcgi_accesstest, fcgi_fastcgitest,
    fcgi_analyse};

static const char *const pages[PAGE__MAX] = {
    "newsession",
    "addanswer",
    "updateanswer",
    "nextquestion",
    "delanswer",
    "delanswerandfollowing",
    "delsession",
    "accesstest",
    "fastcgitest",
    "analyse"
};

int response_nextquestion(struct kreq *req, struct session *ses, struct nextquestions *nq);
struct session *request_load_session(struct kreq *req);
struct answer *request_load_answer(struct kreq *req);

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
                                   PAGE_NEWSESSION)) {
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
        fprintf(stderr,
                "khttp_fcgi_parse: terminate, becausee er == KCGI_EXIT");
        break;
      } else if (KCGI_OK != er) {
        LOG_WARNV("khttp_fcgi_parse: error: %d\n", er);
        fprintf(stderr, "khttp_fcgi_parse: error: %d\n", er);
        break;
      }

      // Fail if we can't find the page, or the mime type is wrong
      LOG_WARNV("Considering whether to throw a 404 (req.page=%d, MAX=%d; "
                "req.mime=%d, KMIME=%d)",
                (int)req.page, (int)PAGE__MAX, (int)KMIME_TEXT_HTML,
                (int)req.mime);

      if (PAGE__MAX == req.page || KMIME_TEXT_HTML != req.mime) {

        LOG_WARNV("Throwing a 404 error", 1);
        er = khttp_head(&req, kresps[KRESP_STATUS], "%s", khttps[KHTTP_404]);

        if (KCGI_HUP == er) {
          fprintf(stderr, "khttp_head: interrupt\n");
          continue;
        } else if (KCGI_OK != er) {
          fprintf(stderr, "khttp_head: error: %d\n", er);
          break;
        }

      } else {
        // Call page dispatcher
        (*disps[req.page])(&req);

        // Make sure no sessions are locked when done.
        release_my_session_locks();
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

void quick_error(struct kreq *req, int e, const char *msg) {
  int retVal = 0;
  enum kcgi_err er;
  do {
    er = khttp_head(req, kresps[KRESP_STATUS], "%s", khttps[e]);
    if (KCGI_HUP == er) {
      fprintf(stderr, "khttp_head: interrupt\n");
      continue;
    } else if (KCGI_OK != er) {
      fprintf(stderr, "khttp_head: error: %d\n", er);
      break;
    }

    // Emit mime-type
    er = khttp_head(req, kresps[KRESP_CONTENT_TYPE], "%s",
                    kmimetypes[KMIME_APP_JSON]);
    if (KCGI_HUP == er) {
      fprintf(stderr, "khttp_head: interrupt\n");
      continue;
    } else if (KCGI_OK != er) {
      fprintf(stderr, "khttp_head: error: %d\n", er);
      break;
    }

    // Begin sending body
    er = khttp_body(req);
    if (KCGI_HUP == er) {
      fprintf(stderr, "khttp_body: interrupt\n");
      continue;
    } else if (KCGI_OK != er) {
      fprintf(stderr, "khttp_body: error: %d\n", er);
      break;
    }

    struct kjsonreq jsonreq;
    kjson_open(&jsonreq, req);
    kcgi_writer_disable(req);
    kjson_obj_open(&jsonreq);

    // Write some stuff in reply
    kjson_putstringp(&jsonreq, "message", msg);

    // Display error log as well.
    kjson_stringp_open(&jsonreq, "trace");
    for (int i = 0; i < error_count; i++) {
      char line[1024];
      snprintf(line, 1024, "%s\n", error_messages[i]);
      kjson_string_puts(&jsonreq, line);
    }
    kjson_string_close(&jsonreq);

    kjson_obj_close(&jsonreq);
    kjson_close(&jsonreq);

  } while (0);

  (void)retVal;
  return;
}

void begin_200(struct kreq *req) {
  enum kcgi_err er;

  do {
    // Emit 200 response
    er = khttp_head(req, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    if (KCGI_HUP == er) {
      fprintf(stderr, "khttp_head: interrupt\n");
      continue;
    } else if (KCGI_OK != er) {
      fprintf(stderr, "khttp_head: error: %d\n", er);
      break;
    }

    // Emit mime-type
    er = khttp_head(req, kresps[KRESP_CONTENT_TYPE], "%s",
                    kmimetypes[req->mime]);
    if (KCGI_HUP == er) {
      fprintf(stderr, "khttp_head: interrupt\n");
      continue;
    } else if (KCGI_OK != er) {
      fprintf(stderr, "khttp_head: error: %d\n", er);
      break;
    }

    // Begin sending body
    er = khttp_body(req);
    if (KCGI_HUP == er) {
      fprintf(stderr, "khttp_body: interrupt\n");
      continue;
    } else if (KCGI_OK != er) {
      fprintf(stderr, "khttp_body: error: %d\n", er);
      break;
    }

  } while (0);
}

void begin_500(struct kreq *req) {
  enum kcgi_err er;

  do {
    // Emit 500 response
    er = khttp_head(req, kresps[KRESP_STATUS], "%s", khttps[KHTTP_500]);
    if (KCGI_HUP == er) {
      fprintf(stderr, "khttp_head: interrupt\n");
      continue;
    } else if (KCGI_OK != er) {
      fprintf(stderr, "khttp_head: error: %d\n", er);
      break;
    }

    // Emit mime-type
    er = khttp_head(req, kresps[KRESP_CONTENT_TYPE], "%s",
                    kmimetypes[req->mime]);
    if (KCGI_HUP == er) {
      fprintf(stderr, "khttp_head: interrupt\n");
      continue;
    } else if (KCGI_OK != er) {
      fprintf(stderr, "khttp_head: error: %d\n", er);
      break;
    }

    // Begin sending body
    er = khttp_body(req);
    if (KCGI_HUP == er) {
      fprintf(stderr, "khttp_body: interrupt\n");
      continue;
    } else if (KCGI_OK != er) {
      fprintf(stderr, "khttp_body: error: %d\n", er);
      break;
    }

  } while (0);
}

static void fcgi_newsession(struct kreq *req) {
  int retVal = 0;

  // #384 forward instantiation
  struct session_meta *meta;

  do {

    LOG_INFO("Page handler entered.");

    struct kpair *survey = req->fieldmap[KEY_SURVEYID];
    if (!survey) {
      // No survey ID, so return 400
      quick_error(req, KHTTP_400, "Surveyid missing.");
      LOG_ERROR("surveyid missing from query string");
      break;
    }

    // #363 parse session meta
    meta = fcgirequest_parse_session_meta(req);
    if (!meta) {
      quick_error(req, KHTTP_500, "Session could not be created (1).");
      LOG_ERROR("fcgirequest_parse_session_meta() failed");
      break;
    }

    // #363 validate session meta
    enum khttp status = fcgirequest_validate_request(req, meta);
    if (status >= KHTTP_400) {
      free_session_meta(meta);
      meta = NULL;
      quick_error(req, status, "Invalid idendity provider check app configuration");
      LOG_ERRORV("fcgirequest_validate_request() returned status %d >= KHTTP_400 (%d)", KHTTP_400, status);
      break;
    }

    // #363 save session meta
    char session_id[1024];
    if (create_session(survey->val, session_id, meta)) {
      free_session_meta(meta);
      meta = NULL;
      quick_error(req, KHTTP_500, "Session could not be created (2).");
      LOG_ERROR("create_session() failed");
      break;
    }

    // reply, #363 free session_meta
    free_session_meta(meta);

    // reply
    begin_200(req);
    enum kcgi_err er = khttp_puts(req, session_id);
    if (er != KCGI_OK) {
      LOG_ERROR("khttp_puts() failed");
    }
    LOG_INFO("Leaving page handler");

  } while (0);

  (void)retVal;
  return;
}

static void fcgi_addanswer(struct kreq *req) {
  int retVal = 0;

  // #384 forward instantiation
  struct session *ses = NULL;
  struct answer *ans = NULL;
  struct nextquestions *nq = NULL;

  do {

    LOG_INFO("Entering page handler for addanswer.");

    ses = request_load_session(req);
    if (!ses) {
      quick_error(req, KHTTP_400, "Could not load specified session. Does it exist?");
      LOG_ERROR("Could not load session");
      break;
    }

    /*
     FILE *fp = open_log("session.log");
     dump_session(fp, ses);
    */

    // validate request against session meta (#363)
    enum khttp status = fcgirequest_validate_session_request(req, ses);
    if (status != KHTTP_200) {
      free_session(ses);
      ses = NULL;
      quick_error(req, status, "Invalid idendity provider, check app configuration");
      LOG_ERRORV("validate_session_meta_kreq() returned status %d != (%d)", KHTTP_200, status);
    }

    // validate requested action against session current state (#379)
    char reason[1024];
    if (validate_session_action(ACTION_SESSION_ADDANSWER, ses, reason, 1024)) {
      free_session(ses);
      ses = NULL;
      quick_error(req, KHTTP_400, reason);
      LOG_ERROR("Session action validation failed");
    }

    ans = request_load_answer(req);
    if (!ans) {
      free_session(ses);
      ses = NULL;
      quick_error(req, KHTTP_400, "Could not load answer");
      LOG_ERROR("Could not load answer");
      break;
    }

    if (session_add_answer(ses, ans)) {
      free_session(ses);
      ses = NULL;
      free_answer(ans);
      ans = NULL;
      quick_error(req, KHTTP_400, "Invalid answer, could not add to session.");
      LOG_ERROR("session_add_answer() failed.");
    }

    free_answer(ans);
    ans = NULL;

    // #332 next_questions data struct
    nq = get_next_questions(ses);
    if (!nq) {
      free_session(ses);
      ses = NULL;
      LOG_ERROR("get_next_questions() failed");
    }

    // joerg: break if session could not be updated
    if (save_session(ses)) {
      free_session(ses);
      ses = NULL;
      free_next_questions(nq);
      nq = NULL;
      quick_error(req, KHTTP_500, "Unable to update session.");
      LOG_ERROR("save_session failed()");
    }

    // All ok, so tell the caller the next question to be answered
    // #373, separate response handler for nextquestions
    if (response_nextquestion(req, ses, nq)) {
      free_session(ses);
      ses = NULL;
      free_next_questions(nq);
      nq = NULL;
      quick_error(req, KHTTP_500, "Could not load next questions for specified session.");
      LOG_ERROR("Could not load next questions for specified session");
    }

    free_session(ses);
    ses = NULL;
    free_next_questions(nq);
    nq = NULL;
    LOG_INFO("Leaving page handler.");

  } while (0);

  (void)retVal;
  return;
}


struct session *request_load_session(struct kreq *req) {
    struct kpair *arg = req->fieldmap[KEY_SESSIONID];
    if (!arg) {
      // No session ID, so return 400
      LOG_WARNV("Sessionid missing. from query string", 0);
      return NULL;
    }
    if (!arg->val) {
      LOG_WARNV("sessionid is blank", 0);
      return NULL;
    }

    // joerg: break if session could not be updated
    if (lock_session(arg->val)) {
      LOG_WARNV("failed to lock session '%s'", arg->val);
      return NULL;
    }

    struct session *ses = load_session(arg->val);
    if (!ses) {
      LOG_WARNV("Could not load session '%s'", arg->val);
      return NULL;
    }

    return ses;
}

struct answer *request_load_answer(struct kreq *req) {
    struct kpair *arg = req->fieldmap[KEY_ANSWER];
    if (!arg) {
      LOG_WARNV("answer is missing", 0);
      return NULL;
    }

    if (!arg->val) {
      LOG_WARNV("answer is blank", 0);
      return NULL;
    }

    // Deserialise answer
    struct answer *ans = calloc(sizeof(struct answer), 1);
    if (!ans) {
      LOG_WARNV("calloc() of answer structure failed.", 0);
      return NULL;
    }

    if (deserialise_answer(arg->val, ANSWER_FIELDS_PUBLIC, ans)) {
      free_answer(ans);
      LOG_WARNV("deserialise_answer() failed.", 0);
      return NULL;
    }

    if (!ans) {
      LOG_WARNV("deserialise_answer() failed. (answer is null)", 0);
      return NULL;
    }
    return ans;
}

static void fcgi_updateanswer(struct kreq *req) {
  int retVal = 0;

  // #384 forward instantiation
  struct session *ses = NULL;
  struct answer *ans = NULL;
  struct nextquestions *nq = NULL;

  do {

    LOG_INFO("Entering page handler.");

    ses = request_load_session(req);
    if (!ses) {
      quick_error(req, KHTTP_400, "Could not load specified session. Does it exist?");
      LOG_ERROR("Could not load session");
      break;
    }

    // validate request against session meta (#363)
    enum khttp status = fcgirequest_validate_session_request(req, ses);
    if (status != KHTTP_200) {
      free_session(ses);
      ses = NULL;
      quick_error(req, status, "Invalid idendity provider, check app configuration");
      LOG_ERRORV("validate_session_meta_kreq() returned status %d != (%d)", KHTTP_200, status);
    }

    // validate requested action against session current state (#379)
    char reason[1024];
    if (validate_session_action(ACTION_SESSION_ADDANSWER, ses, reason, 1024)) {
      free_session(ses);
      ses = NULL;
      quick_error(req, KHTTP_400, reason);
      LOG_ERROR("Session action validation failed");
    }

    ans = request_load_answer(req);
    if (!ans) {
      free_session(ses);
      ses = NULL;
      quick_error(req, KHTTP_400, "Could not load answer");
      LOG_ERROR("Could not load answer");
      break;
    }

    if (session_delete_answers_by_question_uid(ses, ans->uid, 0) < 0) {
      free_session(ses);
      ses = NULL;
      free_answer(ans);
      ans = NULL;
      // TODO could be both 400 or 500 (storage, serialization, not in session)
      quick_error(req, KHTTP_400, "Answer does not match existing session records.");
      LOG_ERROR("session_delete_answers_by_question_uid() failed");
      break;
    }

    if (session_add_answer(ses, ans)) {
      free_session(ses);
      ses = NULL;
      free_answer(ans);
      ans = NULL;
      quick_error(req, KHTTP_400, "Invalid answer, could not add to session.");
      LOG_ERROR("session_add_answer() failed.");
      break;
    }

    free_answer(ans);
    ans = NULL;

    // #332 next_questions data struct
    nq = get_next_questions(ses);
    if (!nq) {
      free_session(ses);
      ses = NULL;
      LOG_ERROR("get_next_questions() failed");
    }

    if (save_session(ses)) {
      free_session(ses);
      ses = NULL;
      free_next_questions(nq);
      nq = NULL;
      quick_error(req, KHTTP_500, "Unable to update session.");
      LOG_ERROR("save_session() failed");
    }

    // All ok, so tell the caller the next question to be answered
    // #373, separate response handler for nextquestions
    if (response_nextquestion(req, ses, nq)) {
      free_session(ses);
      ses = NULL;
      free_next_questions(nq);
      nq = NULL;
      quick_error(req, KHTTP_500, "Could not load next questions for specified session.");
      LOG_ERROR("Could not load next questions for specified session.");
    }

    free_session(ses);
    free_next_questions(nq);
    LOG_INFO("Leaving page handler.");

  } while (0);

  (void)retVal;
  return;
}

static void fcgi_delanswer(struct kreq *req) {
  int retVal = 0;

  // #384 forward instantiation
  struct session *ses = NULL;
  struct nextquestions *nq = NULL;

  do {

    LOG_INFO("Entering page handler.");

    struct kpair *arg = req->fieldmap[KEY_QUESTIONID];

    if (!arg) {
      quick_error(req, KHTTP_400, " Question is missing.");
      LOG_ERROR("question is missing");
    }

    if (!arg->val) {
      quick_error(req, KHTTP_400, "question is blank.");
      LOG_ERROR("question is blank");
    }

    ses = request_load_session(req);
    if (!ses) {
      quick_error(req, KHTTP_400, "Could not load specified session. Does it exist?");
      LOG_ERROR("Could not load session");
    }

    // validate request against session meta (#363)
    enum khttp status = fcgirequest_validate_session_request(req, ses);
    if (status != KHTTP_200) {
      free_session(ses);
      ses = NULL;
      quick_error(req, status, "Invalid idendity provider, check app configuration");
      LOG_ERRORV("validate_session_meta_kreq() returned status %d != (%d)", KHTTP_200, status);
    }

    // validate requested action against session current state (#379)
    char reason[1024];
    if (validate_session_action(ACTION_SESSION_DELETEANSWER, ses, reason, 1024)) {
      free_session(ses);
      ses = NULL;
      quick_error(req, KHTTP_400, reason);
      LOG_ERROR("Session action validation failed");
    }

    // We have a question -- so delete all answers to the given question

    if (session_delete_answers_by_question_uid(ses, arg->val, 0) < 0) {
      free_session(ses);
      ses = NULL;
      // TODO could be both 400 or 500 (storage, serialization, not in session)
      quick_error(req, KHTTP_400, "Answer does not match existing session records.");
      LOG_ERROR("session_delete_answers_by_question_uid() failed");
    }

    // #332 next_questions data struct
    nq = get_next_questions(ses);
    if (!nq) {
      free_session(ses);
      ses = NULL;
      LOG_ERROR("get_next_questions() failed");
    }

    if (save_session(ses)) {
      free_session(ses);
      ses = NULL;
      free_next_questions(nq);
      nq = NULL;
      quick_error(req, KHTTP_500, "Unable to update session.");
      LOG_ERROR("save_session() failed");
    }

    // All ok, so tell the caller the next question to be answered
    // #373, separate response handler for nextquestions
    if (response_nextquestion(req, ses, nq)) {
      free_session(ses);
      ses = NULL;
      free_next_questions(nq);
      nq = NULL;
      quick_error(req, KHTTP_500, "Could not load next questions for specified session.");
      LOG_ERROR("Could not load next questions for specified session");
    }

    free_session(ses);
    free_next_questions(nq);
    LOG_INFO("Leaving page handler.");

  } while (0);

  (void)retVal;
  return;
}

static void fcgi_delanswerandfollowing(struct kreq *req) {
  int retVal = 0;

  // #384 forward instantiation
  struct session *ses = NULL;
  struct nextquestions *nq = NULL;

  do {

    LOG_INFO("Entering page handler.");

    struct kpair *arg = req->fieldmap[KEY_QUESTIONID];

    if (!arg) {
      quick_error(req, KHTTP_400, " Question is missing.");
      LOG_ERROR("question is missing");
    }

    if (!arg->val) {
      quick_error(req, KHTTP_400, "question is blank.");
      LOG_ERROR("question is blank");
    }

    ses = request_load_session(req);
    if (!ses) {
      quick_error(req, KHTTP_400, "Could not load specified session. Does it exist?");
      LOG_ERROR("Could not load session");
    }

    // validate request against session meta (#363)
    enum khttp status = fcgirequest_validate_session_request(req, ses);
    if (status != KHTTP_200) {
      free_session(ses);
      ses = NULL;
      quick_error(req, status, "Invalid idendity provider, check app configuration");
      LOG_ERRORV("validate_session_meta_kreq() returned status %d != (%d)", KHTTP_200, status);
    }

    // validate requested action against session current state (#379)
    char reason[1024];
    if (validate_session_action(ACTION_SESSION_DELETEANSWER, ses, reason, 1024)) {
      free_session(ses);
      ses = NULL;
      quick_error(req, KHTTP_400, reason);
      LOG_ERROR("Session action validation failed");
    }

    // We have a question -- so delete all answers to the given question

    if (session_delete_answers_by_question_uid(ses, arg->val, 1) < 0) {
      free_session(ses);
      ses = NULL;
      // TODO could be both 400 or 500 (storage, serialization, not in session)
      quick_error(req, KHTTP_400, "Answer does not match existing session records.");
      LOG_ERROR("session_delete_answers_by_question_uid() failed");
      break;
    }

    // #332 next_questions data struct
    nq = get_next_questions(ses);
    if (!nq) {
      free_session(ses);
      ses = NULL;
      LOG_ERROR("get_next_questions() failed");
    }

    if (save_session(ses)) {
      free_session(ses);
      ses = NULL;
      free_next_questions(nq);
      nq = NULL;
      quick_error(req, KHTTP_500, "Unable to update session.");
      LOG_ERROR("save_session() failed");
    }

    // All ok, so tell the caller the next question to be answered
    // #373, separate response handler for nextquestions
    if (response_nextquestion(req, ses, nq)) {
      free_session(ses);
      ses = NULL;
      free_next_questions(nq);
      nq = NULL;
      quick_error(req, KHTTP_500, "Could not load next questions for specified session.");
      LOG_ERROR("Could not load next questions for specified session");
    }

    free_session(ses);
    free_next_questions(nq);
    LOG_INFO("Leaving page handler.");

  } while (0);

  (void)retVal;
  return;
}

static void fcgi_delsession(struct kreq *req) {
  int retVal = 0;

  // #384 forward instantiation
  struct session *ses = NULL;

  do {

    LOG_INFO("Entering page handler.");

    ses = request_load_session(req);
    if (!ses) {
      quick_error(req, KHTTP_400, "Could not load specified session. Does it exist?");
      LOG_ERROR("Could not load session");
    }

    // validate request against session meta (#363)
    enum khttp status = fcgirequest_validate_session_request(req, ses);
    if (status != KHTTP_200) {
      free_session(ses);
      ses = NULL;
      quick_error(req, status, "Invalid idendity provider, check app configuration");
      LOG_ERRORV("validate_session_meta_kreq() returned status %d != (%d)", KHTTP_200, status);
    }

    // validate requested action against session current state (#379)
    char reason[1024];
    if (validate_session_action(ACTION_SESSION_DELETEANSWER, ses, reason, 1024)) {
      free_session(ses);
      ses = NULL;
      quick_error(req, KHTTP_400, reason);
      LOG_ERROR("Session action validation failed");
    }

    if (delete_session(ses->session_id)) {
      free_session(ses);
      ses = NULL;
      quick_error(req, KHTTP_400, "Could not delete session. Does it exist?");
      LOG_ERROR("delete_session() failed");
    }

    free_session(ses);

    // reply
    begin_200(req);
    enum kcgi_err er = khttp_puts(req, "Session deleted");
    if (er != KCGI_OK)
      LOG_ERROR("khttp_puts() failed");
    LOG_INFO("Leaving page handler");

  } while (0);

  (void)retVal;
  return;
}

static void fcgi_nextquestion(struct kreq *req) {
  //  enum kcgi_err    er;
  int retVal = 0;

  // #384 forward instantiation
  struct session *ses = NULL;
  struct nextquestions *nq = NULL;

  do {

    LOG_INFO("Entering page handler.");

    ses = request_load_session(req);
    if (!ses) {
      quick_error(req, KHTTP_400, "Could not load specified session. Does it exist?");
      LOG_ERROR("Could not load session");
      break;
    }

    // validate request against session meta (#363)
    enum khttp status = fcgirequest_validate_session_request(req, ses);
    if (status != KHTTP_200) {
      free_session(ses);
      ses = NULL;
      quick_error(req, status, "Invalid idendity provider, check app configuration");
      LOG_ERRORV("validate_session_meta_kreq() returned status %d != (%d)", KHTTP_200, status);
    }

    // validate requested action against session current state (#379)
    char reason[1024];
    if (validate_session_action(ACTION_SESSION_NEXTQUESTIONS, ses, reason, 1024)) {
      free_session(ses);
      ses = NULL;
      quick_error(req, KHTTP_400, reason);
      LOG_ERROR("Session action validation failed");
    }

    // #332 next_questions data struct
    nq = get_next_questions(ses);
    if (!nq) {
      free_session(ses);
      ses = NULL;
      LOG_ERROR("get_next_questions() failed");
    }

    if (save_session(ses)) {
      free_session(ses);
      ses = NULL;
      free_next_questions(nq);
      nq = NULL;
      quick_error(req, KHTTP_500, "Unable to update session.");
      LOG_ERROR("save_session() failed");
    }

    // All ok, so tell the caller the next question to be answered
    // #373, separate response handler for nextquestions
    if (response_nextquestion(req, ses, nq)) {
      free_session(ses);
      ses = NULL;
      free_next_questions(nq);
      nq = NULL;
      quick_error(req, KHTTP_500, "Could not load next questions for specified session.");
      LOG_ERROR("Could not load next questions for specified session.");
    }

    free_session(ses);
    free_next_questions(nq);
    LOG_INFO("Leaving page handler.");
  } while (0);

  (void)retVal;

  return;
}

#define TEST_READ(X)                                                           \
  snprintf(failmsg, 16384, "Could not generate path ${SURVEY_HOME}/%s", X);    \
  if (generate_path(X, test_path, 8192)) {                                     \
    quick_error(req, KHTTP_500, failmsg);                                      \
    break;                                                                     \
  }                                                                            \
  snprintf(failmsg, 16384,                                                     \
           "Could not open for reading path ${SURVEY_HOME}/%s", X);            \
  f = fopen(test_path, "r");                                                   \
  if (!f) {                                                                    \
    quick_error(req, KHTTP_500, failmsg);                                      \
    break;                                                                     \
  }                                                                            \
  fclose(f);

#define TEST_WRITE(X)                                                          \
  snprintf(failmsg, 16384, "Could not generate path ${SURVEY_HOME}/%s", X);    \
  if (generate_path(X, test_path, 8192)) {                                     \
    quick_error(req, KHTTP_500, failmsg);                                      \
    break;                                                                     \
  }                                                                            \
  f = fopen(test_path, "w");                                                   \
  if (!f) {                                                                    \
    snprintf(failmsg, 16384,                                                   \
             "Could not open for writing path %s, errno=%d (%s)", test_path,   \
             errno, strerror(errno));                                          \
    quick_error(req, KHTTP_500, failmsg);                                      \
    break;                                                                     \
  }                                                                            \
  fclose(f);

#define TEST_MKDIR(X)                                                          \
  snprintf(failmsg, 16384, "Could not generate path ${SURVEY_HOME}/%s", X);    \
  if (generate_path(X, test_path, 8192)) {                                     \
    quick_error(req, KHTTP_500, failmsg);                                      \
    break;                                                                     \
  }                                                                            \
  if (mkdir(test_path, 0777)) {                                                \
    snprintf(failmsg, 16384, "Could not mkdir path %s, errno=%d (%s)",         \
             test_path, errno, strerror(errno));                               \
    quick_error(req, KHTTP_500, failmsg);                                      \
    break;                                                                     \
  }

#define TEST_REMOVE(X)                                                         \
  snprintf(failmsg, 16384, "Could not generate path ${SURVEY_HOME}/%s", X);    \
  if (generate_path(X, test_path, 8192)) {                                     \
    quick_error(req, KHTTP_500, failmsg);                                      \
    break;                                                                     \
  }                                                                            \
  if (remove(test_path)) {                                                     \
    if (errno != ENOENT) {                                                     \
      snprintf(failmsg, 16384,                                                 \
               "Could not remove file for writing path %s, errno=%d (%s)",     \
               test_path, errno, strerror(errno));                             \
      quick_error(req, KHTTP_500, failmsg);                                    \
      break;                                                                   \
    }                                                                          \
  }

static void fcgi_accesstest(struct kreq *req) {
  // Try to access paths, and report status.

  do {

    LOG_INFO("Entering page handler.");

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

    quick_error(req, KHTTP_200, "All okay.");

    LOG_INFO("Leaving page handler.");

  } while (0);

  return;
}

static void fcgi_fastcgitest(struct kreq *req) {
  do {
    LOG_INFO("Entering page handler.");
    quick_error(req, KHTTP_200, "All okay.");
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

  // #384 forward instantiation
  struct session *ses = NULL;

  do {

    LOG_INFO("Entering page handler.");

    ses = request_load_session(req);
    if (!ses) {
      quick_error(req, KHTTP_400, "Could not load specified session. Does it exist?");
      LOG_ERROR("Could not load session");
      break;
    }

    // validate request against session meta (#363)
    enum khttp status = fcgirequest_validate_session_request(req, ses);
    if (status != KHTTP_200) {
      free_session(ses);
      ses = NULL;
      quick_error(req, status, "Invalid idendity provider, check app configuration");
      LOG_ERRORV("validate_session_meta_kreq() returned status %d != (%d)", KHTTP_200, status);
    }

    // validate requested action against session current state (#379)
    char reason[1024];
    if (validate_session_action(ACTION_SESSION_ANALYSIS, ses, reason, 1024)) {
      free_session(ses);
      ses = NULL;
      quick_error(req, KHTTP_400, reason);
      LOG_ERROR("Session action validation failed");
    }

    const char *analysis = NULL;
    // String is allocated into heap to since we have no control over the lifetime of the Python string.
    // You need to free it
    if (get_analysis(ses, &analysis)) {
      free_session(ses);
      ses = NULL;
      quick_error(req, KHTTP_500, "Could not retrieve analysis.");
      LOG_ERROR("get_analysis() failed");
    }

    if (!analysis) {
      free_session(ses);
      ses = NULL;
      quick_error(req, KHTTP_500, "Could not retrieve analysis (NULL).");
      LOG_ERROR("get_analysis() returned NULL result");
    }

    if (!analysis[0]) {
      free_session(ses);
      ses = NULL;
      quick_error(req, KHTTP_500, "Could not retrieve analysis (empty result).");
      LOG_ERROR("get_analysis() returned empty result");
    }

    // store analysis with session
    if (session_add_datafile(ses->session_id, "analysis.json", analysis)) {
      LOG_WARNV("Could not add analysis.json for session.", 0);
      // do not break here
    }

    free_session(ses);
    ses = NULL;

    // reply
    er = khttp_head(req, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    if (KCGI_HUP == er) {
      fprintf(stderr, "khttp_head: interrupt\n");
      continue;
    } else if (KCGI_OK != er) {
      if (analysis) {
        free((char *)analysis);
      }
      fprintf(stderr, "khttp_head: error: %d\n", er);
      break;
    }

    // Emit mime-type
    er = khttp_head(req, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_APP_JSON]);
    if (KCGI_HUP == er) {
      fprintf(stderr, "khttp_head: interrupt\n");
      continue;
    } else if (KCGI_OK != er) {
      if (analysis) {
        free((char *)analysis);
      }
      fprintf(stderr, "khttp_head: error: %d\n", er);
      break;
    }

    // Begin sending body
    er = khttp_body(req);
    if (KCGI_HUP == er) {
      fprintf(stderr, "khttp_body: interrupt\n");
      continue;
    } else if (KCGI_OK != er) {
      if (analysis) {
        free((char *)analysis);
      }
      fprintf(stderr, "khttp_body: error: %d\n", er);
      break;
    }

    er = khttp_puts(req, analysis);
    // #288, we need to free the analysis
    if (analysis) {
      free((char *)analysis);
    }

    if (er != KCGI_OK) {
      LOG_ERROR("khttp_puts() failed");
    }

    LOG_INFO("Leaving page handler");
  } while (0);

  (void)retVal;

  return;
}

void response_nextquestion_add_choices(struct question *q , struct kjsonreq *resp) {
  // open "choices"
  kjson_arrayp_open(resp, "choices");
  size_t len = strlen(q->choices);
  char choice[1024] = { 0 };
  int i = 0;
  int k = 0;

  switch (q->type) {
    case QTYPE_MULTICHOICE:
    case QTYPE_MULTISELECT:
    // #98 add single checkbox choices
    case QTYPE_SINGLESELECT:
    case QTYPE_SINGLECHOICE:
    case QTYPE_CHECKBOX:
    // #205 add sequence fields
    case QTYPE_FIXEDPOINT_SEQUENCE:
    case QTYPE_DAYTIME_SEQUENCE:
    case QTYPE_DATETIME_SEQUENCE:
    case QTYPE_DIALOG_DATA_CRAWLER:

    if (!len) {
      kjson_putstring(resp, choice);
      break;
    }

    for (i = 0; i < len; i++) {
      if (q->choices[i] == ',') {
        choice[k + 1] = 0;
        kjson_putstring(resp, choice);
        // reset choice
        k = 0;
        choice[0] = 0;
      } else {
        choice[k] = q->choices[i];
        choice[k + 1] = 0;
        k++;
      }
    }

    // last element
    choice[k] = 0;
    if (k) {
      kjson_putstring(resp, choice);
    }

    break;

  default:
    break;
  } // switch

  // close "choices"
  kjson_array_close(resp);
  return;
}

/**
 * Render kcgi JSON response
 * #373, #363, #379
 */
int response_nextquestion(struct kreq *req, struct session *ses, struct nextquestions *nq) {
  int retVal = 0;

  do {
    if (!req) {
       LOG_ERROR("response_nextquestion(): kreq required (null)");
       break;
    }
    if (!ses) {
      LOG_ERROR("response_nextquestion(): session required (null)");
      break;
    }
    if (!nq) {
      LOG_ERROR("response_nextquestion(): nextquestions required (null)");
      break;
    }

    // json response

    struct kjsonreq resp;
    kjson_open(&resp, req);
    kcgi_writer_disable(req);
    khttp_head(req, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_head(req, kresps[KRESP_CONTENT_TYPE], "%s",
               kmimetypes[KMIME_APP_JSON]);
    khttp_body(req);

    kjson_obj_open(&resp);
    // #332 add status, message
    kjson_putintp(&resp, "status", nq->status);
    kjson_putstringp(&resp, "message", (nq->message != NULL) ? nq->message : "");

    kjson_arrayp_open(&resp, "next_questions");

    for (int i = 0; i < nq->question_count; i++) {
      // Output each question
      kjson_obj_open(&resp);
      kjson_putstringp(&resp, "id",          nq->next_questions[i]->uid);
      kjson_putstringp(&resp, "name",        nq->next_questions[i]->uid);
      kjson_putstringp(&resp, "title",       nq->next_questions[i]->question_text);
      kjson_putstringp(&resp, "description", nq->next_questions[i]->question_html);
      kjson_putstringp(&resp, "type",        question_type_names[nq->next_questions[i]->type]);

      // Provide default value if question not previously answered,
      // else provide the most recent deleted answer for this question. #186
      // #384, refactor
      struct answer *exists = session_get_answer(nq->next_questions[i]->uid, ses);
      if (exists && (exists->flags & ANSWER_DELETED)) {
        char default_value[8192] = { 0 };
        if (answer_get_value_raw(exists, default_value, 8192)) {
          LOG_WARNV("Failed to fetch default value from previously deleted answer to question '%s'", nq->next_questions[i]->uid);
          default_value[0] = 0;
        }
        kjson_putstringp(&resp, "default_value", default_value);
      } else {
        kjson_putstringp(&resp, "default_value", nq->next_questions[i]->default_value);
      }

      // #341 add min/max values, man kjson_putintp
      kjson_putintp(&resp, "min_value", (int64_t) nq->next_questions[i]->min_value);
      kjson_putintp(&resp, "max_value", (int64_t) nq->next_questions[i]->max_value);

      // #384, refactor due to an occasional memory overflow (outside?) issue when assembling choices, resulting in a never ending while loop
      response_nextquestion_add_choices(nq->next_questions[i], &resp);

      // #72 unit field
      kjson_putstringp(&resp, "unit", nq->next_questions[i]->unit);
      kjson_obj_close(&resp);

    } // endfor

    kjson_array_close(&resp);
    kjson_obj_close(&resp);
    kjson_close(&resp);

    LOG_INFO("End next questions handler.");

  } while(0);

  return retVal;
}
