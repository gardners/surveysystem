#ifndef __FCGI_SS_H__
#define __FCGI_SS_H__

#include "kcgi.h"
#include "survey.h"

#define X_HEADER_MW_USER "X-SurveyProxy-Auth-User"
#define X_HEADER_MW_GROUP "X-SurveyProxy-Auth-Group"

// fcgi_main.c
enum key {
  KEY_SURVEY_ID,
  KEY_SESSION_ID,
  KEY_QUESTION_ID,
  KEY_ANSWER,
  KEY_IF_MATCH,
  KEY__MAX
};

enum page {
  PAGE_INDEX,     // #389 add root page

  PAGE_SESSION,   // #260
  PAGE_QUESTIONS, // #260
  PAGE_ANSWERS,   // #260, #461
  PAGE_ANALYSIS,  // #260

  PAGE_ACCESTEST,
  PAGE_FCGITEST,
  PAGE__MAX
};

// fcgi_request.c

struct session_meta *fcgi_request_parse_meta(struct kreq *req);
enum khttp fcgi_request_validate_meta_kreq(struct kreq *req, struct session_meta *meta);
enum khttp fcgi_request_validate_meta_session(struct kreq *req, struct session *ses);
enum khttp fcgi_request_validate_method(struct kreq *req, enum kmethod allowed[], size_t length); // #260, #461

char *fcgi_request_get_field_value(enum key field, struct kreq *req);
char *fcgi_request_get_consistency_hash(struct kreq *req); // #260

struct answer *fcgi_request_load_answer(struct kreq *req);
struct session *fcgi_request_load_session(struct kreq *req);

// #260
struct session *fcgi_request_load_and_verify_session(struct kreq *req, enum actions action, int *error);
int fcgi_request_validate_session_idendity(struct kreq *req, struct session_meta *meta);
int fcgi_request_validate_meta_session_1(struct kreq *req, struct session *ses);

int fcgi_error_response(struct kreq *req, int retVal);

struct answer_list *fcgi_request_parse_answers_serialised(struct kreq *req, struct session *ses, int *error);
struct answer_list *fcgi_request_parse_answers_values(struct kreq *req, struct session *ses, int *error);

// fcgi_response.c

int http_open(struct kreq *req, enum khttp status, enum kmime mime, char *etag);
void http_json_error(struct kreq *req, enum khttp status, const char *msg);

int fcgi_response_nextquestion(struct kreq *req, struct session *ses, struct nextquestions *nq);
#endif

