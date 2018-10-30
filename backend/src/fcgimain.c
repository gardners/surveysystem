
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

#include "kcgi.h"
#include "kcgijson.h"

#include "errorlog.h"
#include "survey.h"
#include "serialisers.h"

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
	PAGE_UPDATEANWSER,
	PAGE_NEXTQUESTION,
	PAGE_DELANSWER,
	PAGE_DELSESSION,
	PAGE__MAX
};

typedef	void (*disp)(struct kreq *);

static void fcgi_newsession(struct kreq *);
static void fcgi_addanswer(struct kreq *);
static void fcgi_updateanswer(struct kreq *);
static void fcgi_nextquestion(struct kreq *);
static void fcgi_delanswer(struct kreq *);
static void fcgi_delsession(struct kreq *);

static const disp disps[PAGE__MAX] = {
  fcgi_newsession,
  fcgi_addanswer,
  fcgi_updateanswer,
  fcgi_nextquestion,
  fcgi_delanswer,
  fcgi_delsession
};

static const char *const pages[PAGE__MAX] = {
  "newsession",
  "addanswer",
  "updateanswer",
  "nextquestion",
  "delanswer",
  "delsession"
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
      {	LOG_ERROR("khttp_fcgi_init() failed.",""); }

    if (!fcgi) { LOG_ERROR("fcgi==NULL after call to khttp_fcgi_init()",""); }

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
      else
	// Call page dispatcher
	(*disps[req.page])(&req);
      
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

void quick_error(struct kreq *req,int e,char *msg)
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

void dump_errors_kcgi(struct kreq *req)
{
  for(int i=0;i<error_count;i++) {
    char msg[1024];
    snprintf(msg,1024,"%s<br>\n",error_messages[i]);
    khttp_puts(req,msg);
  }
  return;
}


static void fcgi_newsession(struct kreq *req)
{
  enum kcgi_err    er;
  int retVal=0;
  
  do {

    struct kpair *survey = req->fieldmap[KEY_SURVEYID];
    if (!survey) {
      // No survey ID, so return 400
      quick_error(req,KHTTP_400,"surveyid missing");
      break;
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

    
  } while(0);

  return;
}

static void fcgi_addanswer(struct kreq *req)
{
  enum kcgi_err    er;
  int retVal=0;
  
  do {

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

    struct kpair *answer = req->fieldmap[KEY_ANSWER];
    if (!answer) {
      // No answer, so return 400
      quick_error(req,KHTTP_400,"answer missing");
      break;
    }

    // 
    
    begin_200(req);

    // Write some stuff in reply
    er = khttp_puts(req, session_id);

    
  } while(0);

  return;
  
}

static void fcgi_updateanswer(struct kreq *r)
{
  
}

static void fcgi_delanswer(struct kreq *r)
{
  
}

static void fcgi_delsession(struct kreq *r)
{
  
}

static void fcgi_nextquestion(struct kreq *r)
{
  //  enum kcgi_err    er;
  int retVal=0;
  
  do {

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
      LOG_ERROR("get_next_questions() failed",session_id);
    
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
    kjson_arrayp_open(&req,"questions");
    for(int i=0;i<next_question_count;i++) {
      // Output each question
      kjson_obj_open(&req);
      kjson_putstringp(&req,q[i]->uid,"id");
      kjson_putstringp(&req,q[i]->uid,"name");
      kjson_putstringp(&req,q[i]->question_html,"title");
      kjson_putstringp(&req,q[i]->question_text,"title_text");
      switch (q[i]->type)
	{
	case QTYPE_FIXEDPOINT: kjson_putstringp(&req,"text","type"); break;
	case QTYPE_TEXT: kjson_putstringp(&req,"text","type"); break;
	case QTYPE_MULTICHOICE:
	  kjson_putstringp(&req,"radiogroup","type");
	  kjson_arrayp_open(&req,"choices");
	  for(int j=0;q[i]->choices[j];) {
	    char choice[65536];
	    int cl=0;
	    while(q[i]->choices[j+cl]&&(q[i]->choices[j+cl]!=','))
	      {
		if (cl<65535) {
		  choice[cl]=q[i]->choices[j+cl];
		  choice[cl+1]=0;
		}
		cl++;
	      }
	    kjson_putstring(&req,choice);
	    j+=cl;
	  }
	  kjson_array_close(&req);
	  break;
	case QTYPE_DATETIME:
	  kjson_putstringp(&req,"text","type");
	  kjson_putstringp(&req,"date","inputType");
	  break;
	default:
	  kjson_putstringp(&req,"text","type"); break;	  
	}
      
      kjson_obj_close(&req);
    }
    kjson_array_close(&req); 
    kjson_obj_close(&req); 
    kjson_close(&req);
        
  } while(0);

  return;
  
  
}
