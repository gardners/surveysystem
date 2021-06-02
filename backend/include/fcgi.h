#ifndef __FCGIREQUEST_H__
#define __FCGIREQUEST_H__

#include "kcgi.h"
#include "survey.h"

#define X_HEADER_MW_USER "X-SurveyProxy-Auth-User"
#define X_HEADER_MW_GROUP "X-SurveyProxy-Auth-Group"

struct session_meta *fcgi_request_parse_meta(struct kreq *req);
enum khttp fcgi_request_validate_meta_kreq(struct kreq *req, struct session_meta *meta);
enum khttp fcgi_request_validate_meta_session(struct kreq *req, struct session *ses);

#endif

