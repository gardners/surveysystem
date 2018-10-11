
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
    
    if (KCGI_OK != khttp_fcgi_init(&fcgi, NULL, 0, NULL, 0, 0))
      {	LOG_ERROR("khttp_fcgi_init() failed.",""); }

    if (!fcgi) { LOG_ERROR("fcgi==NULL after call to khttp_fcgi_init()",""); }
    
    for (;;) {
      er = khttp_fcgi_parse(fcgi, &req);
      if (KCGI_EXIT == er) {
	fprintf(stderr, "khttp_fcgi_parse: terminate\n");
	break;
      } else if (KCGI_OK != er) {
	fprintf(stderr, "khttp_fcgi_parse: error: %d\n", er);
	break;
      }
      
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
