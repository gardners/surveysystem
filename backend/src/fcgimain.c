
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

#include "kcgi.h"

#include "errorlog.h"
#include "survey.h"
#include "serialisers.h"

enum key {
  KEY_SURVEYID,
  KEY_SESSIONID,
  KEY_QUESTIONID,
  KEY_ANSWER,
  KEY__MAX
};

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
  if (validate_session_id(kp->val)) return 1;
  else return 0;
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
  
  if (deserialise_answer(kp->val,&a)) return 1;
  else return 0;
}

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

static void fcgi_newsession(struct kreq *req)
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
    
    // Write some stuff in reply
    er = khttp_puts(req, "Hello, world!<br>\n");

    struct kpair *p;
    if ((p = req->fieldmap[KEY_SURVEYID])) {
      khttp_puts(req,"Has surveyid: ");
      khttp_puts(req, p->val);
    }
    else khttp_puts(req,"No surveyid provided");
    
    
  } while(0);

  return;
}

static void fcgi_addanswer(struct kreq *r)
{
  
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
  
}
