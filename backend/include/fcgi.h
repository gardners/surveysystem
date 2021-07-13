#ifndef __FCGI_SS_H__
#define __FCGI_SS_H__

#include "kcgi.h"
#include "survey.h"

#define X_HEADER_MW_USER "X-SurveyProxy-Auth-User"
#define X_HEADER_MW_GROUP "X-SurveyProxy-Auth-Group"

// fcgi_main.c
enum key { KEY_SURVEYID, KEY_SESSIONID, KEY_QUESTIONID, KEY_ANSWER, KEY__MAX };

enum page {
  PAGE_INDEX, // #389 add root page
  PAGE_NEWSESSION,
  PAGE_ADDANSWER,
  PAGE_ANSWERS, // #260, #461
  PAGE_UPDATEANSWER,
  PAGE_NEXTQUESTION,
  PAGE_DELANSWER,
  PAGE_DELPREVANSWER, // #268
  PAGE_DELSESSION,
  PAGE_ACCESTEST,
  PAGE_FCGITEST,
  PAGE_ANALYSE,
  PAGE__MAX
};

// fcgi_request.c

struct session_meta *fcgi_request_parse_meta(struct kreq *req);
enum khttp fcgi_request_validate_meta_kreq(struct kreq *req, struct session_meta *meta);
enum khttp fcgi_request_validate_meta_session(struct kreq *req, struct session *ses);
enum khttp fcgi_request_validate_method(struct kreq *req, enum kmethod allowed[], size_t length); // #260, #461

char *fcgi_request_get_field_value(enum key field, struct kreq *req);
struct answer *fcgi_request_load_answer(struct kreq *req);
struct session *fcgi_request_load_session(struct kreq *req);

// fcgi_response.c

void http_open(struct kreq *req, enum khttp status, enum kmime mime, char *etag);
void http_json_error(struct kreq *req, enum khttp status, const char *msg);

int fcgi_response_nextquestion(struct kreq *req, struct session *ses, struct nextquestions *nq);
#endif

