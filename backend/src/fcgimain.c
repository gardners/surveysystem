
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

#include "kcgi.h"
#include "kcgijson.h"

#include "errorlog.h"
#include "survey.h"
#include "serialisers.h"
#include "question_types.h"

int kvalid_surveyid(struct kpair *kp) {
  // Only use our validation here, not one of the pre-defined ones
  kp->type = KPAIR__MAX;

  // Is okay
  kp->parsed.s = kp->val;
  return 1;
}

int kvalid_sessionid(struct kpair *kp) {
  // Only use our validation here, not one of the pre-defined ones
  kp->type = KPAIR__MAX;

  kp->parsed.s = kp->val;
  if (validate_session_id(kp->val)) return 0;
  else return 1;
}

int kvalid_questionid(struct kpair *kp) {
  // Only use our validation here, not one of the pre-defined ones
  kp->type = KPAIR__MAX;

  kp->parsed.s = kp->val;
  // Is okay
  return kvalid_string(kp);
}

int kvalid_answer(struct kpair *kp) {
  // Only use our validation here, not one of the pre-defined ones
  kp->type = KPAIR__MAX;

  kp->parsed.s = kp->val;
  
  struct answer a;

  // XXX Remember deserialised answer and keep it in memory to save parsing twice?
  
  if (deserialise_answer(kp->val,&a)) return 0;
  else return 1;
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
  
void dump_errors_kcgi(struct kreq *req)
{
  for(int i=0;i<error_count;i++) {
    char msg[1024];
    snprintf(msg,1024,"%s<br>\n",error_messages[i]);
    khttp_puts(req,msg);
  }
  return;
}

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
      er = khttp_fcgi_parse(fcgi, &req);
      if (KCGI_EXIT == er) {
	fprintf(stderr, "khttp_fcgi_parse: terminate\n");
	break;
      } else if (KCGI_OK != er) {
	fprintf(stderr, "khttp_fcgi_parse: error: %d\n", er);
	break;
      }

      // Fail if we can't find the page, or the mime type is wrong
      if (PAGE__MAX == req.page || 
	  KMIME_TEXT_HTML != req.mime) {
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

    khttp_fcgi_free(fcgi);
    
      
  } while(0);

  if (retVal) {
    fprintf(stderr,"Survey FASTCGI service failed:\n");
    dump_errors(stderr);
  }
  return retVal;
}

void quick_error(struct kreq *req,int e,const char *msg)
{
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
    
    // Write some stuff in reply
    er = khttp_puts(req, msg);

    // Display error log as well.
    dump_errors_kcgi(req);
    
  } while(0);

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
      quick_error(req,KHTTP_400,"surveyid missing");
      LOG_ERROR("surveyid missing from query string");
    }

    char session_id[1024];
    if (create_session(survey->val,session_id)) {
      begin_500(req);
      er = khttp_puts(req, "<h1>500 Internal Error. Session could not be created.</h1>\n");
      er = khttp_puts(req, "Error log:<br>\n");
      dump_errors_kcgi(req);
      break;
    }    
    
    begin_200(req);

    // Write some stuff in reply
    er = khttp_puts(req, session_id);

    LOG_INFO("Leaving page handler");
    
  } while(0);

  return;
}

static void fcgi_addanswer(struct kreq *req)
{
  int retVal=0;
  
  do {

    LOG_INFO("Entering page handler.");
    
    struct kpair *session = req->fieldmap[KEY_SESSIONID];
    if (!session) {
      // No session ID, so return 400
      quick_error(req,KHTTP_400,"sessionid missing");
      LOG_ERROR("sessionid missing from query string");
    }
    if (!session->val) {
      quick_error(req,KHTTP_400,"sessionid is blank");
      LOG_ERROR("sessionid is blank");
    }
    char *session_id=session->val;

    if (lock_session(session_id)) LOG_ERRORV("Failed to lock session '%s'",session_id);    
    
    struct session *s=load_session(session_id);
    if (!s) {
      quick_error(req,KHTTP_400,"Could not load specified session. Does it exist?");
      LOG_ERRORV("Could not load session '%s'",session_id);
    }

    struct kpair *answer = req->fieldmap[KEY_ANSWER];
    if (!answer) {
      // No answer, so return 400
      quick_error(req,KHTTP_400,"answer missing");
      LOG_ERROR("answer is missing");
    }
    if (!answer->val) {
      quick_error(req,KHTTP_400,"answer is blank");
      break;
    }

    // Deserialise answer
    struct answer a;
    if (deserialise_answer(answer->val,&a)) {
      quick_error(req,KHTTP_400,"Could not deserialise answer");
      break;
    }

    if (session_add_answer(s,&a)) {
      quick_error(req,KHTTP_400,"session_add_answer() failed");
      break;
    }
    if (save_session(s)) LOG_ERRORV("save_session('%s') failed",session_id);

    
    // All ok, so tell the caller the next question to be answered
    fcgi_nextquestion(req);

    LOG_INFO("Leaving page handler.");
    
  } while(0);

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
      quick_error(req,KHTTP_400,"sessionid missing");
      break;
    }
    if (!session->val) {
      quick_error(req,KHTTP_400,"sessionid is blank");
      break;
    }
    char *session_id=session->val;

    if (lock_session(session_id)) LOG_ERRORV("Failed to lock session '%s'",session_id);    
    
    struct session *s=load_session(session_id);
    if (!s) {
      quick_error(req,KHTTP_400,"Could not load specified session. Does it exist?");
      break;
    }

    struct kpair *answer = req->fieldmap[KEY_ANSWER];
    if (!answer) {
      // No answer, so return 400
      quick_error(req,KHTTP_400,"answer missing");
      break;
    }
    if (!answer->val) {
      quick_error(req,KHTTP_400,"answer is blank");
      break;
    }

    // Deserialise answer
    struct answer a;
    if (deserialise_answer(answer->val,&a)) {
      quick_error(req,KHTTP_400,"Could not deserialise answer");
      break;
    }

    if (session_delete_answers_by_question_uid(s,a.uid,0)<0) {
      quick_error(req,KHTTP_400,"session_delete_answers_by_question_uid() failed");
      break;
    }
    if (session_add_answer(s,&a)) {
      quick_error(req,KHTTP_400,"session_add_answer() failed");
      break;
    }
    if (save_session(s)) {
      quick_error(req,KHTTP_400,"save_session() failed");
      LOG_ERRORV("save_session('%s') failed",session_id);
    }
    
    // All ok, so tell the caller the next question to be answered
    fcgi_nextquestion(req);

    LOG_INFO("Leaving page handler.");
    
  } while(0);

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
      quick_error(req,KHTTP_400,"sessionid missing");
      break;
    }
    if (!session->val) {
      quick_error(req,KHTTP_400,"sessionid is blank");
      break;
    }
    char *session_id=session->val;
    if (lock_session(session_id)) LOG_ERRORV("Failed to lock session '%s'",session_id);    
    struct session *s=load_session(session_id);
    if (!s) {
      quick_error(req,KHTTP_400,"Could not load specified session. Does it exist?");
      break;
    }

    struct kpair *question = req->fieldmap[KEY_QUESTIONID];
    struct kpair *answer = req->fieldmap[KEY_ANSWER];
    if ((!answer)&&(!question)) {
      // No answer, so return 400
      quick_error(req,KHTTP_400,"answer missing");
      break;
    }
    if (answer&&(!answer->val)) {
      quick_error(req,KHTTP_400,"answer is blank");
      break;
    }
    if (question&&(!question->val)) {
      quick_error(req,KHTTP_400,"question is blank");
      break;
    }
    if (answer&&question) {
      quick_error(req,KHTTP_400,"You cannot provide both a question ID and and answer when deleting answer(s) to a question");
      break;
    }

    // We have an answer -- so delete the specific answer
    if (answer&&answer->val) {
      // Deserialise answer
      struct answer a;
      if (deserialise_answer(answer->val,&a)) {
	quick_error(req,KHTTP_400,"deserialise_answer() failed");
	break;
      }
      // We have an answer, so try to delete it.
      if (session_delete_answer(s,&a,0)) {
	quick_error(req,KHTTP_400,"session_delete_answer() failed");
	break;
      }
    }
    else if (question&&question->val) {
      // No answer give, so delete all answers to the given question
      if (session_delete_answers_by_question_uid(s,question->val,0)<0) {
	quick_error(req,KHTTP_400,"session_delete_answers_by_question_uid() failed");
	break;
      }      
    }
    else {
      quick_error(req,KHTTP_400,"Either a question ID or an answer must be providd");
      break;
    }
    if (save_session(s)) {
      quick_error(req,KHTTP_400,"save_session() failed");
      LOG_ERRORV("save_session('%s') failed",session_id);
    }
    
    // All ok, so tell the caller the next question to be answered
    fcgi_nextquestion(req);

    LOG_INFO("Leaving page handler.");
    
  } while(0);

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
      quick_error(req,KHTTP_400,"sessionid missing");
      break;
    }
    if (!session->val) {
      quick_error(req,KHTTP_400,"sessionid is blank");
      break;
    }
    char *session_id=session->val;
    if (lock_session(session_id)) LOG_ERRORV("Failed to lock session '%s'",session_id);    
    struct session *s=load_session(session_id);
    if (!s) {
      quick_error(req,KHTTP_400,"Could not load specified session. Does it exist?");
      break;
    }

    struct kpair *question = req->fieldmap[KEY_QUESTIONID];
    struct kpair *answer = req->fieldmap[KEY_ANSWER];
    if ((!answer)&&(!question)) {
      // No answer, so return 400
      quick_error(req,KHTTP_400,"answer missing");
      break;
    }
    if (answer&&(!answer->val)) {
      quick_error(req,KHTTP_400,"answer is blank");
      break;
    }
    if (question&&(!question->val)) {
      quick_error(req,KHTTP_400,"question is blank");
      break;
    }
    if (answer&&question) {
      quick_error(req,KHTTP_400,"You cannot provide both a question ID and and answer when deleting answer(s) to a question");
      break;
    }

    // We have an answer -- so delete the specific answer
    if (answer&&answer->val) {
      // Deserialise answer
      struct answer a;
      if (deserialise_answer(answer->val,&a)) {
	quick_error(req,KHTTP_400,"deserialise_answer() failed");
	break;
      }
      // We have an answer, so try to delete it.
      if (session_delete_answer(s,&a,1)) {
	quick_error(req,KHTTP_400,"session_delete_answer() failed");
	break;
      }
    }
    else if (question&&question->val) {
      // No answer give, so delete all answers to the given question
      if (session_delete_answers_by_question_uid(s,question->val,0)<0) {
	quick_error(req,KHTTP_400,"session_delete_answers_by_question_uid() failed");
	break;
      }      
    }
    else {
      quick_error(req,KHTTP_400,"Either a question ID or an answer must be providd");
      break;
    }
    if (save_session(s)) {
      quick_error(req,KHTTP_400,"save_session() failed");
      LOG_ERRORV("save_session('%s') failed",session_id);
    }
    
    // All ok, so tell the caller the next question to be answered
    fcgi_nextquestion(req);

    LOG_INFO("Leaving page handler.");
    
  } while(0);

  return;   
}


static void fcgi_delsession(struct kreq *r)
{
  //  enum kcgi_err    er;
  int retVal=0;
  
  do {

    LOG_INFO("Entering page handler.");
    
    struct kpair *session = r->fieldmap[KEY_SESSIONID];
    if (!session) {
      // No session ID, so return 400
      quick_error(r,KHTTP_400,"sessionid missing");
      break;
    }
    if (!session->val) {
      quick_error(r,KHTTP_400,"sessionid is blank");
      break;
    }
    char *session_id=session->val;

    if (lock_session(session_id)) LOG_ERRORV("Failed to lock session '%s'",session_id);
    
    struct session *s=load_session(session_id);
    if (!s) {
      quick_error(r,KHTTP_400,"Could not load specified session. Does it exist?");
      break;
    }
    free(s);
    if (delete_session(session_id)) {
      quick_error(r,KHTTP_400,"Could not delete session. Does it exist?");
      LOG_ERRORV("delete_session('%s') failed",session_id);
    }

    quick_error(r,KHTTP_200,"Session deleted.");
    
    LOG_INFO("Leaving page handler.");
    
  } while(0);
  
  return;  
}

static void fcgi_nextquestion(struct kreq *r)
{
  //  enum kcgi_err    er;
  int retVal=0;
  
  do {

    LOG_INFO("Entering page handler.");

    struct kpair *session = r->fieldmap[KEY_SESSIONID];
    if (!session) {
      // No session ID, so return 400
      quick_error(r,KHTTP_400,"sessionid missing");
      break;
    }
    if (!session->val) {
      quick_error(r,KHTTP_400,"sessionid is blank");
      break;
    }
    char *session_id=session->val;

    struct session *s=load_session(session_id);
    if (!s) {
      quick_error(r,KHTTP_400,"Could not load specified session. Does it exist?");
      break;
    }
    struct question *q[1024];
    int next_question_count=0;
    if (get_next_questions(s,q,1024,&next_question_count))
      LOG_ERRORV("get_next_questions('%s') failed",session_id);
    
    struct kjsonreq req;
    kjson_open(&req, r); 
    kcgi_writer_disable(r); 
    khttp_head(r, kresps[KRESP_STATUS], 
	       "%s", khttps[KHTTP_200]); 
    khttp_head(r, kresps[KRESP_CONTENT_TYPE], 
	       "%s", kmimetypes[r->mime]); 
    khttp_body(r);
    kjson_obj_open(&req);
    kjson_putint(&req, next_question_count); 
    kjson_arrayp_open(&req,"next_questions");
    for(int i=0;i<next_question_count;i++) {
      // Output each question
      kjson_obj_open(&req);
      kjson_putstringp(&req,"id",q[i]->uid);
      kjson_putstringp(&req,"name",q[i]->uid);
      kjson_putstringp(&req,"title",q[i]->question_text);
      kjson_putstringp(&req,"description",q[i]->question_html);
      kjson_putstringp(&req,"type",question_type_names[q[i]->type]);
      // Provide default value if question not previously answered,
      // else provide the most recent deleted answer for this question. #186
      {
	int found_value=0;	
	struct answer *a;
	for(int i=0;i<s->answer_count;i++)
	  if(!strcmp(s->answers[i]->uid,q[i]->uid)) {
	    if (s->answers[i]->flags&ANSWER_DELETED)
	      {
		char rendered[8192];
		snprintf(rendered,8192,"%s",s->answers[i]->text);
		
		switch(q[i]->type)
		  {
		  case QTYPE_INT:	    snprintf(rendered,8192,"%lld",s->answers[i]->value); break;
		  case QTYPE_FIXEDPOINT:    snprintf(rendered,8192,"%lld",s->answers[i]->value); break;
		  case QTYPE_MULTICHOICE:   break;
		  case QTYPE_MULTISELECT:   break;
		  case QTYPE_LATLON:        snprintf(rendered,8192,"%lld,%lld",s->answers[i]->lat,s->answers[i]->lon); break;
		  case QTYPE_DATETIME:      snprintf(rendered,8192,"%lld",s->answers[i]->time_begin); break;
		  case QTYPE_DAYTIME:       snprintf(rendered,8192,"%lld",s->answers[i]->time_begin); break;
		  case QTYPE_TIMERANGE:     snprintf(rendered,8192,"%lld,%lld",s->answers[i]->time_begin,s->answers[i]->time_end); break;
		  case QTYPE_UPLOAD:        break;
		  case QTYPE_TEXT:          break;
		  case QTYPE_CHECKBOX:      break;
		  case QTYPE_HIDDEN:        break;
		  case QTYPE_TEXTAREA:      break;
		  case QTYPE_EMAIL:         break;
		  case QTYPE_PASSWORD:      break;
		  case QTYPE_SINGLECHOICE:  break;
		  case QTYPE_SINGLESELECT:  break;
		  case QTYPE_UUID:          break;
		  default:
		    LOG_ERRORV("Unknown question type #%d in session '%s'",q[i]->type,session_id);
		    break;
		  }
		  kjson_putstringp(&req,"default_value",rendered);
	      }
	  }
      }
      
      switch (q[i]->type)
	{
	case QTYPE_MULTICHOICE:
	case QTYPE_MULTISELECT:
	  //#98 add single checkbox choices
	case QTYPE_SINGLESELECT:
	case QTYPE_SINGLECHOICE:
	case QTYPE_CHECKBOX: 
	  
	  kjson_arrayp_open(&req,"choices");
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
		kjson_putstring(&req,choice);
	      }
	      j+=cl;
	      if (q[i]->choices[j+cl]==',') j++;
	    }
	  }
	  kjson_array_close(&req);
	  break;
	default:
	  break;
	}
      // #72 unit field
      kjson_putstringp(&req,"unit",q[i]->unit);
      
      kjson_obj_close(&req);
    }
    kjson_array_close(&req); 
    kjson_obj_close(&req); 
    kjson_close(&req);

    LOG_INFO("Leaving page handler.");
    
  } while(0);

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


static void fcgi_analyse(struct kreq *r)
{
  //  enum kcgi_err    er;
  int retVal=0;
  
  do {

    LOG_INFO("Entering page handler.");

    struct kpair *session = r->fieldmap[KEY_SESSIONID];
    if (!session) {
      // No session ID, so return 400
      quick_error(r,KHTTP_400,"sessionid missing");
      break;
    }
    if (!session->val) {
      quick_error(r,KHTTP_400,"sessionid is blank");
      break;
    }
    char *session_id=session->val;

    struct session *s=load_session(session_id);
    if (!s) {
      quick_error(r,KHTTP_400,"Could not load specified session. Does it exist?");
      break;
    }
    const unsigned char *analysis=NULL;
    // string is returned from python in a way that we don't have to deallocate it here
    if (get_analysis(s,&analysis)) {
      quick_error(r,KHTTP_500,"Could not retrieve analysis.");
      LOG_ERRORV("get_analysis('%s') failed",session_id);
    }
    if (!analysis) {
      quick_error(r,KHTTP_500,"Could not retrieve analysis (NULL result).");
      LOG_ERRORV("get_analysis('%s') returned NULL result",session_id);
    }
    if (!analysis[0]) {
      quick_error(r,KHTTP_500,"Could not retrieve analysis (empty result).");
      LOG_ERRORV("get_analysis('%s') returned empty result",session_id);
    }
    quick_error(r,KHTTP_200,(const char *)analysis);

    LOG_INFO("Leaving page handler.");
    
  } while(0);

  return;  
}
