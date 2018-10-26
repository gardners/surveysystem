
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
}

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
				   NULL, 0, // Keys for parsing
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

      // Emit 200 response
      er = khttp_head(&req, kresps[KRESP_STATUS], 
		      "%s", khttps[KHTTP_200]);
      if (KCGI_HUP == er) {
	fprintf(stderr, "khttp_head: interrupt\n");
	khttp_free(&req);
	continue;
      } else if (KCGI_OK != er) {
	fprintf(stderr, "khttp_head: error: %d\n", er);
	khttp_free(&req);
	break;
      }

      // Emit mime-type
      er = khttp_head(&req, kresps[KRESP_CONTENT_TYPE], 
		      "%s", kmimetypes[req.mime]);
      if (KCGI_HUP == er) {
	fprintf(stderr, "khttp_head: interrupt\n");
	khttp_free(&req);
	continue;
      } else if (KCGI_OK != er) {
	fprintf(stderr, "khttp_head: error: %d\n", er);
	khttp_free(&req);
	break;
      }

      // Begin sending body
      er = khttp_body(&req);
      if (KCGI_HUP == er) {
	fprintf(stderr, "khttp_body: interrupt\n");
	khttp_free(&req);
	continue;
      } else if (KCGI_OK != er) {
	fprintf(stderr, "khttp_body: error: %d\n", er);
	khttp_free(&req);
	break;
      }

      // Write some stuff in reply
      er = khttp_puts(&req, "Hello, world!\n");
      if (KCGI_HUP == er) {
	fprintf(stderr, "khttp_puts: interrupt\n");
	khttp_free(&req);
	continue;
      } else if (KCGI_OK != er) {
	fprintf(stderr, "khttp_puts: error: %d\n", er);
	khttp_free(&req);
	break;
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

static void fcgi_newsession(struct kreq *)
{
  
}
