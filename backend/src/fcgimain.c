
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "kcgi.h"
#include "kcgijson.h"

#include "errorlog.h"
#include "survey.h"
#include "serialisers.h"
#include "question_types.h"
#include "timeutils.h"

#define          CHECKPOINT() { fprintf(stderr,"%s:%d:%s():pid=%d: Checkpoint\n",__FILE__,__LINE__,__FUNCTION__,getpid()); }

int kvalid_surveyid(struct kpair *kp) {

  // XXX - This process runs in the sandboxed child environment, from where
  // it is not possible to emit any log output.  Very annoying, but we have
  // to live with it.
  LOG_MUTE();
  
  int retVal=0;
  do {
  
    if (!kp) LOG_ERROR("kp is NULL");

    // Only use our validation here, not one of the pre-defined ones
    kp->type = KPAIR__MAX;
    
    LOG_WARNV("Validating surveyid",1);
    
    // Is okay
    kp->parsed.s = kp->val;
  } while(0);
  if (retVal) retVal=0; else retVal=1;

  LOG_UNMUTE();
  return retVal;
}

int kvalid_sessionid(struct kpair *kp)
{
  // XXX - This process runs in the sandboxed child environment, from where
  // it is not possible to emit any log output.  Very annoying, but we have
  // to live with it.
  LOG_MUTE();
  
  int retVal=0;
  do {
    if (!kp) LOG_ERROR("kp is NULL");
   
    // Only use our validation here, not one of the pre-defined ones
    kp->type = KPAIR__MAX;

    kp->parsed.s = kp->val;
    LOG_WARNV("Validating sessionid",1);
    if (validate_session_id(kp->val)) {
      LOG_ERROR("validate_session_id failed");
    }
  } while(0);
  if (retVal) retVal=0; else retVal=1;
  LOG_UNMUTE();
  return retVal;
}

int kvalid_questionid(struct kpair *kp)
{
  // XXX - This process runs in the sandboxed child environment, from where
  // it is not possible to emit any log output.  Very annoying, but we have
  // to live with it.
  LOG_MUTE();
  
  int retVal=0;
  do {
    if (!kp) LOG_ERROR("kp is NULL");
    // Only use our validation here, not one of the pre-defined ones
    kp->type = KPAIR__MAX;

    LOG_WARNV("Validating questionid",1);
    
    kp->parsed.s = kp->val;
    // Is okay
    if (kvalid_string(kp)) retVal=0;
    else LOG_ERROR("questionid is not a valid string");
  } while(0);
  if (retVal) retVal=0; else retVal=1;
  LOG_UNMUTE();
  return retVal;
}

int kvalid_answer(struct kpair *kp) {

  // XXX - This process runs in the sandboxed child environment, from where
  // it is not possible to emit any log output.  Very annoying, but we have
  // to live with it.
  LOG_MUTE();

  int retVal=0;

  do {

    LOG_WARNV("Validating answer",1);
        
    // Only use our validation here, not one of the pre-defined ones
    kp->type = KPAIR__MAX;
    
    kp->parsed.s = kp->val;

    struct answer *a=calloc(sizeof(struct answer),1);
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
  } while(0);

  LOG_WARNV("retVal=%d",retVal);
  
  if (retVal) retVal=0; else retVal=1;
  LOG_UNMUTE();
  return retVal;
}

enum key {
  KEY_SURVEYID,
  KEY_SESSIONID,
  KEY_QUESTIONID,
  KEY_ANSWER,
  KEY__MAX
};

static const struct kvalid keys[KEY__MAX] = {
  { kvalid_surveyid, "surveyid"},
  { kvalid_sessionid, "sessionid"},
  { kvalid_questionid, "questionid"},
  { kvalid_answer, "answer"}
};



enum	page {
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

typedef	void (*disp)(struct kreq *);

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
  fcgi_newsession,
  fcgi_addanswer,
  fcgi_updateanswer,
  fcgi_nextquestion,
  fcgi_delanswer,
  fcgi_delanswerandfollowing,
  fcgi_delsession,
  fcgi_accesstest,
  fcgi_fastcgitest,
  fcgi_analyse
};

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
 
void usage(void)
{
  fprintf(stderr,
	  "usage: surveyfcgi -- Start fast CGI service\n");
};

int main(int argc,char **argv)
{
  int retVal=0;

  do {
    if (argc>1) { usage(); retVal=-1; break; }

    if (!getenv("SURVEY_HOME")) {
      fprintf(stderr,"SURVEY_HOME environment variable must be set to data directory for survey system.\n");
      usage();
      retVal=-1;
      break;
    }

    struct kreq      req;
    struct kfcgi    *fcgi=NULL;
    enum kcgi_err    er;

    if (KCGI_OK != khttp_fcgi_init(&fcgi,
				   keys, KEY__MAX, // CGI variable parse definitions
				   pages, PAGE__MAX,  // Pages for parsing
				   PAGE_NEWSESSION))
      {	LOG_ERROR("khttp_fcgi_init() failed."); }

    if (!fcgi) { LOG_ERROR("fcgi==NULL after call to khttp_fcgi_init()"); }

    // For each request
    for (;;) {

      // Clear our internal error log
      clear_errors();

      // Parse request
      fprintf(stderr,"Calling fcgi_parse()\n");
      er = khttp_fcgi_parse(fcgi, &req);
      fprintf(stderr,"Returned from fcgi_parse()\n");

      if (KCGI_EXIT == er) {
	LOG_WARNV("khttp_fcgi_parse: terminate, becausee er == KCGI_EXIT",1);
	fprintf(stderr,"khttp_fcgi_parse: terminate, becausee er == KCGI_EXIT");
	break;
      } else if (KCGI_OK != er) {
	LOG_WARNV("khttp_fcgi_parse: error: %d\n", er);
	fprintf(stderr,"khttp_fcgi_parse: error: %d\n", er);
	break;
      }

      // Fail if we can't find the page, or the mime type is wrong
      LOG_WARNV("Considering whether to throw a 404 (req.page=%d, MAX=%d; req.mime=%d, KMIME=%d)",
		(int)req.page,(int)PAGE__MAX,
		(int)KMIME_TEXT_HTML, (int)req.mime);
      if (PAGE__MAX == req.page || 
	  KMIME_TEXT_HTML != req.mime) {
	LOG_WARNV("Throwing a 404 error",1);
	er = khttp_head(&req, kresps[KRESP_STATUS], 
			"%s", khttps[KHTTP_404]);
	if (KCGI_HUP == er) {
	  fprintf(stderr, "khttp_head: interrupt\n");
	  continue;
	} else if (KCGI_OK != er) {
	  fprintf(stderr, "khttp_head: error: %d\n", er);
	  break;
	}
      }
      else {
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
    
      
  } while(0);

  if (retVal) {
    fprintf(stderr,"Survey FASTCGI service failed:\n");
    dump_errors(stderr);
  }
  return retVal;
}

void quick_error(struct kreq *req,int e,const char *msg)
{
  int retVal=0;
  enum kcgi_err    er;
  do {
    er = khttp_head(req, kresps[KRESP_STATUS], 
		    "%s", khttps[e]);
    if (KCGI_HUP == er) {
      fprintf(stderr, "khttp_head: interrupt\n");
      continue;
    } else if (KCGI_OK != er) {
      fprintf(stderr, "khttp_head: error: %d\n", er);
      break;
    }
    
    // Emit mime-type
    er = khttp_head(req, kresps[KRESP_CONTENT_TYPE], 
		    "%s", kmimetypes[KMIME_APP_JSON]);
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
    for(int i = 0; i < error_count; i++) {
      char line[1024];
      snprintf(line, 1024, "%s\n", error_messages[i]);
      kjson_string_puts(&jsonreq, line);
    }
    kjson_string_close(&jsonreq); 
    
    kjson_obj_close(&jsonreq); 
    kjson_close(&jsonreq);
    
  } while(0);

  (void)retVal;
  return;
}  

void begin_200(struct kreq *req)
{
  enum kcgi_err    er;

  do {
    // Emit 200 response
    er = khttp_head(req, kresps[KRESP_STATUS], 
		    "%s", khttps[KHTTP_200]);
    if (KCGI_HUP == er) {
      fprintf(stderr, "khttp_head: interrupt\n");
      continue;
    } else if (KCGI_OK != er) {
      fprintf(stderr, "khttp_head: error: %d\n", er);
      break;
    }
    
    // Emit mime-type
    er = khttp_head(req, kresps[KRESP_CONTENT_TYPE], 
		    "%s", kmimetypes[req->mime]);
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
    

  } while(0);
}

void begin_500(struct kreq *req)
{
  enum kcgi_err    er;

  do {
    // Emit 500 response
    er = khttp_head(req, kresps[KRESP_STATUS], 
		    "%s", khttps[KHTTP_500]);
    if (KCGI_HUP == er) {
      fprintf(stderr, "khttp_head: interrupt\n");
      continue;
    } else if (KCGI_OK != er) {
      fprintf(stderr, "khttp_head: error: %d\n", er);
      break;
    }
    
    // Emit mime-type
    er = khttp_head(req, kresps[KRESP_CONTENT_TYPE], 
		    "%s", kmimetypes[req->mime]);
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
        
  } while(0);
}

static void fcgi_newsession(struct kreq *req)
{
  enum kcgi_err    er;
  int retVal=0;
  
  do {

    LOG_INFO("Page handler entered.");
    
    struct kpair *survey = req->fieldmap[KEY_SURVEYID];
    if (!survey) {
      // No survey ID, so return 400
      LOG_ERROR("surveyid missing from query string");
      quick_error(req, KHTTP_400, "Surveyid missing.");
      break;
    }

    char session_id[1024];
    if (create_session(survey->val,session_id)) {
      quick_error(req, KHTTP_500, "Session could not be created.");
      LOG_ERROR("create_session() failed");
      break;
    }    
    
    // create session meta log file, log user and creation time
    
    char *user_name = "anonymous";
    if (req->rawauth.d.digest.user) {
      user_name = strdup(req->rawauth.d.digest.user);
    }
    
    time_t now = time(0);
    char created[25] = { '\0' };
    if(!format_time_ISO8601(now, created, 25)) {
      LOG_WARNV("could not fromat ISO8601 timestamp for session %s.", session_id );
    }
  
    char user_message[1024];
    snprintf(user_message, 1024, "username %s\nsurveyid %s\nsessionid %s\ncreated %s by user %s", user_name, survey->val, session_id, created, user_name);
    
    if (session_add_userlog_message(session_id, user_message)) {
      quick_error(req, KHTTP_500, "Error handling session user data.");
      LOG_ERRORV("Could not add user info for session %s.", session_id);
      break;
    }
    
    // reply
    begin_200(req);
    er = khttp_puts(req, session_id);
    if (er!=KCGI_OK) LOG_ERROR("khttp_puts() failed");
    LOG_INFO("Leaving page handler");
    
  } while(0);

  (void)retVal;
  return;
}

static void fcgi_addanswer(struct kreq *req)
{
  int retVal=0;
  
  do {

    LOG_INFO("Entering page handler for addanswer.");
    
    struct kpair *session = req->fieldmap[KEY_SESSIONID];
    if (!session) {
      // No session ID, so return 400
      quick_error(req,KHTTP_400,"Sessionid missing.");
      LOG_ERROR("Sessionid missing. from query string");
      break;
    }
    
    if (!session->val) {
      quick_error(req,KHTTP_400,"sessionid is blank");
      LOG_ERROR("sessionid is blank");
      break;
    }
    
    char *session_id=session->val;
    
    // joerg: break if session could not be updated
    if (lock_session(session_id)) {
      quick_error(req,KHTTP_500,"Failed to lock session");
      LOG_ERRORV("failed to lock session '%s'",session_id);
      break;
    }
    
    struct session *s=load_session(session_id);
    
    if (!s) {
      quick_error(req,KHTTP_400,"Could not load specified session. Does it exist?");
      LOG_ERRORV("Could not load session '%s'",session_id);
      break;
    }

    struct kpair *answer = req->fieldmap[KEY_ANSWER];
    if (!answer) {
      // No answer, so return 400
      quick_error(req,KHTTP_400,"Answer missing.");
      LOG_ERROR("answer is missing");
      break;
    }
    
    if (!answer->val) {
      quick_error(req,KHTTP_400,"Answer is blank.");
      LOG_ERROR("answer is blank");
      break;
    }

    // Deserialise answer
    struct answer *a=calloc(sizeof(struct answer),1);
    if (!a) {
      quick_error(req,KHTTP_500, "Error allocating answer.");
      LOG_ERROR("calloc() of answer structure failed.");
      break;
    }

    if (deserialise_answer(answer->val, ANSWER_FIELDS_PUBLIC, a)) {
      free_answer(a); a=NULL;
      quick_error(req,KHTTP_400,"Could not deserialise answer.");
      LOG_ERROR("deserialise_answer() failed.");
      break;
    }

    if (session_add_answer(s,a)) {
      free_answer(a); a=NULL;
      quick_error(req,KHTTP_400,"Invalid answer, could not add to session.");
      LOG_ERROR("session_add_answer() failed.");
      break;
    }
    
    if (a) free_answer(a);
    a=NULL;
    
    // joerg: break if session could not be updated
    if (save_session(s)) {
      quick_error(req,KHTTP_500,"Unable to update session.");
      LOG_ERRORV("save_session('%s') failed",session_id);
      break;
    }

    
    // All ok, so tell the caller the next question to be answered
    fcgi_nextquestion(req);
    LOG_INFO("Leaving page handler.");
    
  } while(0);

  (void)retVal;
  return;
  
}

static void fcgi_updateanswer(struct kreq *req)
{
  int retVal=0;
  
  do {

    LOG_INFO("Entering page handler.");

    struct kpair *session = req->fieldmap[KEY_SESSIONID];
    
    if (!session) {
      // No session ID, so return 400
      quick_error(req,KHTTP_400,"Sessionid missing.");
      LOG_ERROR("Sessionid missing. from query string");
      break;
    }
    
    if (!session->val) {
      quick_error(req,KHTTP_400,"sessionid is blank");
      LOG_ERROR("sessionid is blank");
      break;
    }
    
    char *session_id=session->val;

    // joerg: break if session could not be updated
    if (lock_session(session_id)) {
      quick_error(req,KHTTP_500,"Failed to lock session");
      LOG_ERRORV("failed to lock session '%s'",session_id);
      break;
    }
    
    struct session *s=load_session(session_id);
    
    if (!s) {
      quick_error(req,KHTTP_400,"Could not load specified session. Does it exist?");
      LOG_ERRORV("Could not load session '%s'",session_id);
      break;
    }

    struct kpair *answer = req->fieldmap[KEY_ANSWER];
    if (!answer) {
      // No answer, so return 400
      quick_error(req,KHTTP_400,"Answer missing.");
      LOG_ERROR("answer is missing");
      break;
    }
    
    if (!answer->val) {
      quick_error(req,KHTTP_400,"Answer is blank.");
      LOG_ERROR("answer is blank");
      break;
    }

    // Deserialise answer
    struct answer *a=calloc(sizeof(struct answer),1);
    if (!a) {
      quick_error(req,KHTTP_500, "Error allocating answer.");
      LOG_ERROR("calloc() of answer structure failed.");
      break;
    }
    
    if (deserialise_answer(answer->val, ANSWER_FIELDS_PUBLIC, a)) {
      free_answer(a); a=NULL;
      quick_error(req,KHTTP_400,"Could not deserialise answer.");
      LOG_ERROR("deserialise_answer() failed.");
      break;
    }
    
    if (!a) {
      quick_error(req,KHTTP_400,"Required Answer value is blank.");
      LOG_ERROR("deserialise_answer() failed. (answer is null)");
      break;
    }
    
    if (session_delete_answers_by_question_uid(s,a->uid,0)<0) {
      if (a) free_answer(a);
      a=NULL;
      // TODO could be both 400 or 500 (storage, serialization, not in session)
      quick_error(req,KHTTP_400,"Answer does not match existing session records.");
      LOG_ERROR("session_delete_answers_by_question_uid() failed");
      break;
    }
    
    if (session_add_answer(s,a)) {
      if (a) free_answer(a);
      a=NULL;
      quick_error(req,KHTTP_400,"Invalid answer, could not add to session.");
      LOG_ERROR("session_add_answer() failed.");
      break;
    }
    
    if (a) free_answer(a);
    a=NULL;

    if (save_session(s)) {
      quick_error(req,KHTTP_500,"Unable to update session.");
      LOG_ERRORV("save_session('%s') failed",session_id);
      break;
    }
    
    // All ok, so tell the caller the next question to be answered
    fcgi_nextquestion(req);
    LOG_INFO("Leaving page handler.");
    
  } while(0);

  (void)retVal;
  return; 
}  


static void fcgi_delanswer(struct kreq *req)
{
  int retVal=0;
  
  do {

    LOG_INFO("Entering page handler.");

    struct kpair *session = req->fieldmap[KEY_SESSIONID];

    if (!session) {
      // No session ID, so return 400
      quick_error(req,KHTTP_400,"Sessionid missing.");
      LOG_ERROR("Sessionid missing. from query string");
      break;
    }
    
    if (!session->val) {
      quick_error(req,KHTTP_400,"sessionid is blank");
      LOG_ERROR("sessionid is blank");
      break;
    }
    
    char *session_id=session->val;
    
    if (lock_session(session_id)) {
      quick_error(req,KHTTP_500,"Failed to lock session");
      LOG_ERRORV("failed to lock session '%s'",session_id);
      break;
    }
    
    struct session *s=load_session(session_id);
    
    if (!s) {
      quick_error(req,KHTTP_400,"Could not load specified session. Does it exist?");
      LOG_ERRORV("Could not load session '%s'",session_id);
      break;
    }

    struct kpair *question = req->fieldmap[KEY_QUESTIONID];
    struct kpair *answer = req->fieldmap[KEY_ANSWER];
    
    if ((!answer)&&(!question)) {
      // No answer, so return 400
      quick_error(req,KHTTP_400,"Question or Answer missing.");
      LOG_ERROR("question or answer is missing");
      break;
    }
    
    if (answer&&(!answer->val)) {
      quick_error(req,KHTTP_400,"Answer is blank.");
      LOG_ERROR("answer is blank");
      break;
    }
    
    if (question&&(!question->val)) {
      quick_error(req,KHTTP_400,"Question is blank.");
      LOG_ERROR("question is blank");
      break;
    }
    
    if (answer&&question) {
      quick_error(req,KHTTP_400,"You cannot provide both a question ID and and answer when deleting answer(s) to a question.");
      LOG_ERROR("Yinvalid - provided both a question ID and and answer for deletion.");
      break;
    }

    if (answer&&answer->val) {
      
      /* 
       * We have an answer -- so delete the specific answer 
       */
       
      // Deserialise answer
      struct answer *a=calloc(sizeof(struct answer),1);
      if (!a) {
	quick_error(req,KHTTP_500, "Error allocating answer.");
	LOG_ERROR("calloc() of answer structure failed.");
	break;
      }

      if (deserialise_answer(answer->val, ANSWER_FIELDS_PUBLIC, a)) {
	if (a) free_answer(a);
	a=NULL;
	quick_error(req,KHTTP_400,"Could not deserialise answer.");
	LOG_ERROR("deserialise_answer() failed.");
	break;
      }
      
      // We have an answer, so try to delete it.
      if (session_delete_answer(s,a,0)) {
	if (a) free_answer(a);
	a=NULL;
	// TODO could be both 400 or 500 (storage, serialization, not in session)
	quick_error(req,KHTTP_400,"Answer fmarked for deletion does not match existing session records.");
	LOG_ERROR("session_delete_answer() failed");
	break;
      }
    }
    else if (question&&question->val) {
      
      /* 
       * We have a question -- so delete all answers to the given question 
       */
       
      if (session_delete_answers_by_question_uid(s,question->val,0)<0) {
	// TODO could be both 400 or 500 (storage, serialization, not in session)
	quick_error(req,KHTTP_400,"Answer does not match existing session records.");
	LOG_ERROR("session_delete_answers_by_question_uid() failed");
	break;
      }      
    }
    else {
      quick_error(req,KHTTP_400,"Either a question ID or an answer must be provided");
      LOG_ERROR("either a question ID or an answer must be provided");
      break;
    }
    
    if (save_session(s)) {
      quick_error(req,KHTTP_500,"Unable to update session.");
      LOG_ERRORV("save_session('%s') failed",session_id);
      break;
    }
    
    // All ok, so tell the caller the next question to be answered
    fcgi_nextquestion(req);
    LOG_INFO("Leaving page handler.");
    
  } while(0);

  (void)retVal;
  return;   
}

static void fcgi_delanswerandfollowing(struct kreq *req)
{
  int retVal=0;
  
  do {

    LOG_INFO("Entering page handler.");

    struct kpair *session = req->fieldmap[KEY_SESSIONID];

    if (!session) {
      // No session ID, so return 400
      quick_error(req,KHTTP_400,"Sessionid missing.");
      LOG_ERROR("Sessionid missing. from query string");
      break;
    }
    
    if (!session->val) {
      quick_error(req,KHTTP_400,"sessionid is blank");
      LOG_ERROR("sessionid is blank");
      break;
    }
    
    char *session_id=session->val;
    
    if (lock_session(session_id)) {
      quick_error(req,KHTTP_500,"Failed to lock session");
      LOG_ERRORV("failed to lock session '%s'",session_id);
      break;
    }
    
    struct session *s=load_session(session_id);
    
    if (!s) {
      quick_error(req,KHTTP_400,"Could not load specified session. Does it exist?");
      LOG_ERRORV("Could not load session '%s'",session_id);
      break;
    }

    struct kpair *question = req->fieldmap[KEY_QUESTIONID];
    struct kpair *answer = req->fieldmap[KEY_ANSWER];

    if ((!answer)&&(!question)) {
      // No answer, so return 400
      quick_error(req,KHTTP_400,"Question or Answer missing.");
      LOG_ERROR("question or answer is missing");
      break;
    }
    
    if (answer&&(!answer->val)) {
      quick_error(req,KHTTP_400,"Answer is blank.");
      LOG_ERROR("answer is blank");
      break;
    }
    
    if (question&&(!question->val)) {
      quick_error(req,KHTTP_400,"Question is blank.");
      LOG_ERROR("question is blank");
      break;
    }
    
    if (answer&&question) {
      quick_error(req,KHTTP_400,"You cannot provide both a question ID and and answer when deleting answer(s) to a question.");
      LOG_ERROR("Yinvalid - provided both a question ID and and answer for deletion.");
      break;
    }

    if (answer&&answer->val) {

      /* 
       * We have an answer -- so delete the specific answer 
       */
      
      // Deserialise answer
      struct answer *a=calloc(sizeof(struct answer),1);
      if (!a) {
	quick_error(req,KHTTP_500, "Error allocating answer.");
	LOG_ERROR("calloc() of answer structure failed.");
	break;
      }
      
      if (deserialise_answer(answer->val, ANSWER_FIELDS_PUBLIC, a)) {
	if (a) free_answer(a);
	a=NULL;
	quick_error(req,KHTTP_400,"Could not deserialise answer.");
	LOG_ERROR("deserialise_answer() failed.");
	break;
      }
      
      // We have an answer, so try to delete it.
      if (session_delete_answer(s,a,0)) {
	if (a) free_answer(a);
	a=NULL;
	// TODO could be both 400 or 500 (storage, serialization, not in session)
	quick_error(req,KHTTP_400,"Answer fmarked for deletion does not match existing session records.");
	LOG_ERROR("session_delete_answer() failed");
	break;
      }
    }
    else if (question&&question->val) {
      
      /* 
       * We have a question -- so delete all answers to the given question 
       */
       
      if (session_delete_answers_by_question_uid(s,question->val,1)<0) {
	      // TODO could be both 400 or 500 (storage, serialization, not in session)
	      quick_error(req,KHTTP_400,"Answer does not match existing session records.");
	      LOG_ERROR("session_delete_answers_by_question_uid() failed");
	      break;
      }   
    }
    else {
      quick_error(req,KHTTP_400,"Either a question ID or an answer must be provided");
      LOG_ERROR("either a question ID or an answer must be provided");
      break;
    }
    
    if (save_session(s)) {
      quick_error(req,KHTTP_500,"Unable to update session.");
      LOG_ERRORV("save_session('%s') failed",session_id);
      break;
    }
    
    // All ok, so tell the caller the next question to be answered
    fcgi_nextquestion(req);
    LOG_INFO("Leaving page handler.");
    
  } while(0);

  (void)retVal;
  return;   
}


static void fcgi_delsession(struct kreq *req)
{
  enum kcgi_err    er;
  int retVal=0;
  
  do {

    LOG_INFO("Entering page handler.");
    
    struct kpair *session = req->fieldmap[KEY_SESSIONID];

    if (!session) {
      // No session ID, so return 400
      quick_error(req,KHTTP_400,"Sessionid missing.");
      LOG_ERROR("Sessionid missing. from query string");
      break;
    }
    
    if (!session->val) {
      quick_error(req,KHTTP_400,"sessionid is blank");
      LOG_ERROR("sessionid is blank");
      break;
    }

    char *session_id=session->val;

    if (lock_session(session_id)) {
      quick_error(req,KHTTP_500,"Failed to lock session");
      LOG_ERRORV("failed to lock session '%s'",session_id);
      break;
    }
    
    struct session *s=load_session(session_id);

    if (!s) {
      quick_error(req,KHTTP_400,"Could not load specified session. Does it exist?");
      LOG_ERRORV("Could not load session '%s'",session_id);
      break;
    }
    
    free(s);
    
    if (delete_session(session_id)) {
      quick_error(req,KHTTP_400,"Could not delete session. Does it exist?");
      LOG_ERRORV("delete_session('%s') failed",session_id);
      break;
    }

    // log event in session meta log file
    
    char *user_name = "anonymous";
    if (req->rawauth.d.digest.user) {
      user_name = strdup(req->rawauth.d.digest.user);
    }
    
    time_t now = time(0);
    char deleted[25] = { '\0' };
    if(!format_time_ISO8601(now, deleted, 25)) {
      LOG_WARNV("could not fromat ISO8601 timestamp for session %s.", session_id );
    }
  
    char user_message[1024];
    snprintf(user_message, 1024, "deleted %s by user %s", deleted, user_name);
    
    if (session_add_userlog_message(session_id, user_message)) {
      quick_error(req, KHTTP_500, "Error handling session user data.");
      LOG_ERRORV("Could not add user info for session %s.", session_id);
      break;
    }
    
    // reply
    begin_200(req);
    er = khttp_puts(req, "Session deleted");
    if (er!=KCGI_OK) LOG_ERROR("khttp_puts() failed");
    LOG_INFO("Leaving page handler");
    
  } while(0);

  (void)retVal;
  return;  
}

static void fcgi_nextquestion(struct kreq *req)
{
  //  enum kcgi_err    er;
  int retVal=0;
  
  do {

    LOG_INFO("Entering page handler.");

    struct kpair *session = req->fieldmap[KEY_SESSIONID];
    if (!session) {
      // No session ID, so return 400
      quick_error(req,KHTTP_400,"Sessionid missing.");
      LOG_ERROR("Sessionid missing. from query string");
      break;
    }
    
    if (!session->val) {
      quick_error(req,KHTTP_400,"sessionid is blank");
      LOG_ERROR("sessionid is blank");
      break;
    }
    
    char *session_id=session->val;
    
    // joerg: added lock here. It may be an edge case, however a user could run a session on two devices/browser tabs
    if (lock_session(session_id)) {
      quick_error(req,KHTTP_500,"Failed to lock session");
      LOG_ERRORV("failed to lock session '%s'",session_id);
      break;
    }

    struct session *s=load_session(session_id);
    
    if (!s) {
      quick_error(req,KHTTP_400,"Could not load specified session. Does it exist?");
      LOG_ERRORV("Could not load session '%s'",session_id);
      break;
    }
    
    struct question *q[1024];
    int next_question_count=0;
    
    if (get_next_questions(s,q,1024,&next_question_count)) {
      quick_error(req,KHTTP_500,"Could not get next questions.");
      LOG_ERRORV("get_next_questions('%s') failed",session_id);
      break;
    }
    
    // json response
    
    struct kjsonreq resp;
    kjson_open(&resp, req); 
    kcgi_writer_disable(req); 
    khttp_head(req, kresps[KRESP_STATUS], 
	       "%s", khttps[KHTTP_200]); 
    khttp_head(req, kresps[KRESP_CONTENT_TYPE], 
	       "%s", kmimetypes[KMIME_APP_JSON]); 
    khttp_body(req);
    
    kjson_obj_open(&resp);
    kjson_putint(&resp, next_question_count); 
    kjson_arrayp_open(&resp,"next_questions");
    for(int i=0;i<next_question_count;i++) {
      // Output each question
      kjson_obj_open(&resp);
      kjson_putstringp(&resp,"id",q[i]->uid);
      kjson_putstringp(&resp,"name",q[i]->uid);
      kjson_putstringp(&resp,"title",q[i]->question_text);
      kjson_putstringp(&resp,"description",q[i]->question_html);
      kjson_putstringp(&resp,"type",question_type_names[q[i]->type]);
      // Provide default value if question not previously answered,
      // else provide the most recent deleted answer for this question. #186
      {
	for(int j=0;j<s->answer_count;j++)
	  if(!strcmp(s->answers[j]->uid,q[i]->uid)) {
	    if (s->answers[j]->flags&ANSWER_DELETED)
	      {
		char rendered[8192];
		snprintf(rendered,8192,"%s",s->answers[j]->text);
		
		switch(q[i]->type)
		  {
		  case QTYPE_INT:                 snprintf(rendered,8192,"%lld",s->answers[j]->value); break;
		  case QTYPE_FIXEDPOINT:          snprintf(rendered,8192,"%lld",s->answers[j]->value); break;
		  case QTYPE_MULTICHOICE:         break;
		  case QTYPE_MULTISELECT:         break;
		  case QTYPE_LATLON:              snprintf(rendered,8192,"%lld,%lld",s->answers[j]->lat,s->answers[j]->lon); break;
		  case QTYPE_DATETIME:            snprintf(rendered,8192,"%lld",s->answers[j]->time_begin); break;
		  case QTYPE_DAYTIME:             snprintf(rendered,8192,"%lld",s->answers[j]->time_begin); break;
		  case QTYPE_TIMERANGE:           snprintf(rendered,8192,"%lld,%lld",s->answers[j]->time_begin,s->answers[j]->time_end); break;
		  case QTYPE_UPLOAD:              break;
		  case QTYPE_TEXT:                break;
		  case QTYPE_CHECKBOX:            break;
		  case QTYPE_HIDDEN:              break;
		  case QTYPE_TEXTAREA:            break;
		  case QTYPE_EMAIL:               break;
		  case QTYPE_PASSWORD:            break;
		  case QTYPE_SINGLECHOICE:        break;
		  case QTYPE_SINGLESELECT:        break;
		  case QTYPE_UUID:                break;
		  // #205 add sequence fields
		  case QTYPE_FIXEDPOINT_SEQUENCE: break;
		  case QTYPE_DAYTIME_SEQUENCE: 	  break;
		  case QTYPE_DATETIME_SEQUENCE:   break;
		  case QTYPE_DIALOG_DATA_CRAWLER: break;
		  
		  default:
		    LOG_ERRORV("Unknown question type #%d in session '%s'",q[i]->type,session_id);
		    break;
		  }
		  kjson_putstringp(&resp,"default_value",rendered);
	      }
	  }
      }
      
      switch (q[i]->type)
	{
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
	  
	  kjson_arrayp_open(&resp,"choices");
	  int len=strlen(q[i]->choices);
	  if (len) {
	    for(int j=0;q[i]->choices[j];) {
	      char choice[65536];
	      int cl=0;
	      choice[0]=0;
	      while(
		    ((j+cl)<len)
		    &&q[i]->choices[j+cl]
		    &&(q[i]->choices[j+cl]!=',')
		    )
		{
		  if (cl<65535) {
		    choice[cl]=q[i]->choices[j+cl];
		    choice[cl+1]=0;
		  }
		  cl++;
		} 
	      // #74 skip empty values
	      if (q[i]->choices[j]!=',') {
		kjson_putstring(&resp,choice);
	      }
	      j+=cl;
	      if (q[i]->choices[j+cl]==',') j++;
	    }
	  }
	  kjson_array_close(&resp);
	  break;
	default:
	  break;
	}
      // #72 unit field
      kjson_putstringp(&resp,"unit",q[i]->unit);
      
      kjson_obj_close(&resp);
    }
    kjson_array_close(&resp); 
    kjson_obj_close(&resp); 
    kjson_close(&resp);

    LOG_INFO("Leaving page handler.");
    
  } while(0);

  (void)retVal;
  
  return;  
}

#define TEST_READ(X)  snprintf(failmsg,16384,"Could not generate path ${SURVEY_HOME}/%s",X); \
  if (generate_path(X,test_path,8192)) { \
      quick_error(req,KHTTP_500,failmsg); \
      break; \
    } \
  snprintf(failmsg,16384,"Could not open for reading path ${SURVEY_HOME}/%s",X); \
  f=fopen(test_path,"r");						\
    if (!f) { \
      quick_error(req,KHTTP_500,failmsg); \
      break; \
    } \
    fclose(f); \

#define TEST_WRITE(X)  snprintf(failmsg,16384,"Could not generate path ${SURVEY_HOME}/%s",X); \
  if (generate_path(X,test_path,8192)) { \
      quick_error(req,KHTTP_500,failmsg); \
      break; \
    } \
  f=fopen(test_path,"w");			\
    if (!f) { \
      snprintf(failmsg,16384,"Could not open for writing path %s, errno=%d (%s)",test_path,errno,strerror(errno)); \
      quick_error(req,KHTTP_500,failmsg); \
      break; \
    } \
    fclose(f); \

#define TEST_MKDIR(X)  snprintf(failmsg,16384,"Could not generate path ${SURVEY_HOME}/%s",X); \
  if (generate_path(X,test_path,8192)) { \
      quick_error(req,KHTTP_500,failmsg); \
      break; \
    } \
  if (mkdir(test_path,0777)) {						\
      snprintf(failmsg,16384,"Could not mkdir path %s, errno=%d (%s)",test_path,errno,strerror(errno)); \
      quick_error(req,KHTTP_500,failmsg); \
      break; \
    }

#define TEST_REMOVE(X)  snprintf(failmsg,16384,"Could not generate path ${SURVEY_HOME}/%s",X); \
  if (generate_path(X,test_path,8192)) { \
      quick_error(req,KHTTP_500,failmsg); \
      break; \
    } \
  if(remove(test_path)) { \
    if (errno != ENOENT) { \
      snprintf(failmsg,16384,"Could not remove file for writing path %s, errno=%d (%s)",test_path,errno,strerror(errno)); \
      quick_error(req,KHTTP_500,failmsg); \
      break; \
      } \
    } \

static void fcgi_accesstest(struct kreq *req)
{
  // Try to access paths, and report status.

  do {  

    LOG_INFO("Entering page handler.");

    char test_path[8192];
    char failmsg[16384];
    FILE *f=NULL;

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
    
    quick_error(req,KHTTP_200,"All okay.");

    LOG_INFO("Leaving page handler.");
    
  } while (0);

  return;  
}

static void fcgi_fastcgitest(struct kreq *req)
{
  do {
    LOG_INFO("Entering page handler.");
    quick_error(req,KHTTP_200,"All okay.");
    LOG_INFO("Leaving page handler.");
  } while (0);
  return;
}


static void fcgi_analyse(struct kreq *req)
{
  enum kcgi_err    er;
  int retVal=0;
  
  do {

    LOG_INFO("Entering page handler.");

    struct kpair *session = req->fieldmap[KEY_SESSIONID];

    if (!session) {
      // No session ID, so return 400
      quick_error(req,KHTTP_400,"Sessionid missing.");
      LOG_ERROR("Sessionid missing. from query string");
      break;
    }
    
    if (!session->val) {
      quick_error(req,KHTTP_400,"sessionid is blank");
      LOG_ERROR("sessionid is blank");
      break;
    }
    
    char *session_id=session->val;

    // joerg: added lock here. It may be an edge case, however a user could run a session on two devices/browser tabs
    if (lock_session(session_id)) {
      quick_error(req,KHTTP_500,"Failed to lock session");
      LOG_ERRORV("failed to lock session '%s'",session_id);
      break;
    }

    struct session *s=load_session(session_id);
    
    if (!s) {
      quick_error(req,KHTTP_400,"Could not load specified session. Does it exist?");
      LOG_ERRORV("Could not load session '%s'",session_id);
      break;
    }

    const char *analysis=NULL;
    // String is allocated into heap to since we have no control over the lifetime of the Python string.
    // You need to free it
    if (get_analysis(s,&analysis)) {
      quick_error(req,KHTTP_500,"Could not retrieve analysis.");
      LOG_ERRORV("get_analysis('%s') failed",session_id);
      break;
    }

    if (!analysis) {
      quick_error(req,KHTTP_500,"Could not retrieve analysis (NULL).");
      LOG_ERRORV("get_analysis('%s') returned NULL result",session_id);
      break;
    }
    
    if (!analysis[0]) {
      quick_error(req,KHTTP_500,"Could not retrieve analysis (empty result).");
      LOG_ERRORV("get_analysis('%s') returned empty result",session_id);
      break;
    }
    
    // log event in session meta log file
    
    char *user_name = "anonymous";
    if (req->rawauth.d.digest.user) {
      user_name = strdup(req->rawauth.d.digest.user);
    }
    
    time_t now = time(0);
    char finished[25] = { '\0' };
    if(!format_time_ISO8601(now, finished, 25)) {
      LOG_WARNV("could not fromat ISO8601 timestamp for session %s.", session_id );
    }
  
    char user_message[1024];
    snprintf(user_message, 1024, "finished %s by user %s", finished, user_name);
    
    if (session_add_userlog_message(session_id, user_message)) {
      quick_error(req, KHTTP_500, "Error handling session user data.");
      LOG_ERRORV("Could not add user info for session %s.", session_id);
      break;
    }
    
    // store analysis with session
    if (session_add_datafile(session_id, "analysis.json", analysis)) {
      LOG_ERRORV("Could not add analysis.json for session %s.", session_id);
      // do not break here on error
    }
    
    // reply
    er = khttp_head(req, kresps[KRESP_STATUS], 
		    "%s", khttps[KHTTP_200]);
    if (KCGI_HUP == er) {
      fprintf(stderr, "khttp_head: interrupt\n");
      continue;
    } else if (KCGI_OK != er) {
      fprintf(stderr, "khttp_head: error: %d\n", er);
      break;
    }
    
    // Emit mime-type
    er =  khttp_head(req, kresps[KRESP_CONTENT_TYPE], 
	       "%s", kmimetypes[KMIME_APP_JSON]); 
    if (KCGI_HUP == er) {
      fprintf(stderr, "khttp_head: interrupt\n");
      continue;
    } else if (KCGI_OK != er) {
      if (analysis) { free((char*) analysis); }
      fprintf(stderr, "khttp_head: error: %d\n", er);
      break;
    }

    // Begin sending body
    er = khttp_body(req);
    if (KCGI_HUP == er) {
      fprintf(stderr, "khttp_body: interrupt\n");
      continue;
    } else if (KCGI_OK != er) {
      if (analysis) { free((char*) analysis); }
      fprintf(stderr, "khttp_body: error: %d\n", er);
      break;
    }

    er = khttp_puts(req, analysis);
    // #288, we need to free the analysis
    if (analysis) { free((char*) analysis); }
    if (er!=KCGI_OK) LOG_ERROR("khttp_puts() failed");
    LOG_INFO("Leaving page handler");
  } while(0);

  (void)retVal;
  
  return;  
}
