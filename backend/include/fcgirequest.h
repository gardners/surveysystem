#ifndef __FCGIREQUEST_H__
#define __FCGIREQUEST_H__

#include "kcgi.h"
#include "survey.h"

#define X_HEADER_MW_USER "X-SurveyProxy-Auth-User"
#define X_HEADER_MW_GROUP "X-SurveyProxy-Auth-Group"

struct session_meta *fcgirequest_parse_session_meta(struct kreq *req);
enum khttp fcgirequest_validate_request(struct kreq *req, struct session_meta *meta);
enum khttp fcgirequest_validate_session_request(struct kreq *req, struct session *ses);
#endif

