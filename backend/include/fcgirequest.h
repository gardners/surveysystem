#ifndef __FCGIREQUEST_H__
#define __FCGIREQUEST_H__

#include "kcgi.h"
#include "survey.h"

#define X_HEADER_MW_USER "X-SurveyProxy-Auth-User"
#define X_HEADER_MW_GROUP "X-SurveyProxy-Auth-Group"

int parse_session_meta_kreq(struct kreq *req, struct session_meta *meta);
enum Authorized {
  MWAUTHORITY_NOT_DEFINED,
  MWAUTHORITY_INVALID,
  MWAUTHORITY_INVALID_XHEADER,
  MWAUTHORITY_NO_AUTHENTICATION,
  MW_AUTHORITY_OK,
};
enum khttp validate_session_meta_kreq(struct kreq *req, struct session_meta *meta);

#endif

