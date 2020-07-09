#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "kcgi.h"

#include "survey.h"
#include "fcgirequest.h"
#include "errorlog.h"

// some debug functions

extern char **environ;
void log_envs() {
  int i = 1;
  char *s = *environ;

  char tmp[1024];
  char debug[8029] = "env:\n";
  for (; s; i++) {
    snprintf(tmp, 1024, "  |_ %s\n", s);
    strcat(debug, tmp);
    s = *(environ+i);
  }
  LOG_INFO(debug);
}

void log_headers(struct kreq *req){
  char tmp[1024];
  char debug[8029] = "Request headers:\n";
  for (int i = 0; i < req->reqsz; i++) {
    snprintf(tmp, 1024, "  |_ %s: %s\n", req->reqs[i].key, req->reqs[i].val);
    strcat(debug, tmp);
  }

  LOG_INFO(debug);
}

void log_fields(struct kreq *req){
  char tmp[1024];
  char debug[8029] = "Request fields:\n";
  for (int i = 0; i < req->fieldsz; i++) {
    snprintf(tmp, 1024, "  |_ %s: %s\n", req->fields[i].key, req->fields[i].val);
    strcat(debug, tmp);
  }

  LOG_INFO(debug);
}

void log_cookies(struct kreq *req){
  char tmp[1024];
  char debug[8029] = "Cookies:\n";
  for (int i = 0; i < req->cookiesz; i++) {
    snprintf(tmp, 1024, "  |_ %s: %s\n", req->cookies[i].key, req->cookies[i].val);
    strcat(debug, tmp);
  }

  LOG_INFO(debug);
}

void log_session_meta(struct session_meta *meta){
  LOG_INFOV(
    "session_meta\n"
    "  |_ user: %s\n"
    "  |_ group: %s\n"
    "  |_ authority: %s\n"
    "  |_ provider: %d\n",

    meta->user,
    meta->group,
    meta->authority,
    meta->provider
  );
}

/**
 * parses (but does not validate) authorisation type
 * #363
 */
int get_autorisation_type(struct kreq *req) {

  // case trusted middleware
  if (getenv("SS_TRUSTED_MIDDLEWARE")) {
    return IDENDITY_HTTP_TRUSTED;
  }

  // case server auth
  if (req->auth) {

    if (req->rawauth.type == KAUTH_BASIC) {
      return IDENDITY_HTTP_BASIC;
    }

    if (req->rawauth.type == KAUTH_DIGEST) {
      return IDENDITY_HTTP_DIGEST;
    }

  }

  // no server auth
  return IDENDITY_HTTP_PUBLIC;
}

/**
 * parse session meta from request
 * Note that *meta needs to be freed
 * #363
 */
int parse_session_meta_kreq(struct kreq *req, struct session_meta *meta) {
  int retVal = 0;

  /*
  LOG_INFOV(
    "kreq: auth info\n"
    " |_ req->auth '%d',\n"
    " |_ req->rawauth.authorised '%d',\n"
    " |_ req->rawauth.type: '%d'\n"
    " |_ req->rawauth.digest: '%s'\n"
    " |_ req->rawauth.d.digest.user: '%s'\n"
    " |_ req->rawauth.d.digest.realm: '%s'\n"
    " |_ req->rawauth.d.basic.response: '%s'",
    req->auth,
    req->rawauth.authorised,
    req->rawauth.type,
    (req->rawauth.type == KAUTH_DIGEST) ? req->rawauth.digest : "none",
    (req->rawauth.type == KAUTH_DIGEST) ? req->rawauth.d.digest.user: "none",
    (req->rawauth.type == KAUTH_DIGEST) ? req->rawauth.d.digest.realm: "none",
    (req->rawauth.type == KAUTH_BASIC) ? req->rawauth.d.basic.response : "none");
  */

  do {
    // authority
    char authority[1024];
    int l = snprintf(authority, 1024, "%s(%hu)", req->remote, req->port);
    if (l < 1) {
      LOG_ERROR("idendity provider snprintf() for authority failed");
    }
    meta->authority = strndup(authority, 1024);

    // authorisation type
    meta->provider = get_autorisation_type(req);

    // provider
    if (meta->provider == IDENDITY_HTTP_DIGEST ) {
      meta->user = strndup(req->rawauth.d.digest.user, 1024);
      meta->group = strndup(req->rawauth.d.digest.realm, 1024);
      break;
    }

    if (meta->provider == IDENDITY_HTTP_BASIC ) {
        meta->user = strndup(req->rawauth.d.basic.response, 1024);
        meta->group = NULL;
      break;
    }

    // user, group
    if (meta->provider == IDENDITY_HTTP_TRUSTED) {

      // fetch user info from x-headers
      // check if x -headers are set and copy
      int count = 0;
      for (int i = 0; i < req->reqsz; i++) {
        if (0 == strcasecmp(req->reqs[i].key, X_HEADER_MW_USER)) {
          meta->user = strndup(req->reqs[i].val, 1024);
          count++;
        }
        if (0 == strcasecmp(req->reqs[i].key, X_HEADER_MW_GROUP)) {

          meta->group = strndup(req->reqs[i].val, 1024);
          count++;
        }
        if (count == 2) {
          break;
        }
      }

      break;
    }

    //case IDENDITY_HTTP_PUBLIC
    meta->user = NULL;
    meta->group = NULL;

  } while (0);

  return retVal;
}

/**
 * validate parsed session meta and return http status code
 * #363
 */
enum khttp validate_session_meta_kreq(struct kreq *req, struct session_meta *meta) {

  // no authorisation
  if (meta->provider <= IDENDITY_HTTP_PUBLIC) {
    return KHTTP_200;
  }

  // direct auth via basic or digest
  if (meta->provider == IDENDITY_HTTP_BASIC || meta->provider == IDENDITY_HTTP_DIGEST) {
    // authorisation on server level passed? this should normally not occur and indicates some misconfiguration
    if (!req->auth) {
      return KHTTP_401;
    }

    if(!req->rawauth.authorised) {
      return KHTTP_401;
    }
  }

  // indirect auth via trusted (and authorised) middleware
  if (meta->provider == IDENDITY_HTTP_TRUSTED) {
    char *mw = getenv("SS_TRUSTED_MIDDLEWARE");

    // guard, in case meta was not handed through parsing
    if (!meta->authority) {
      LOG_WARNV("Middleware: session meta not parsed completely.",0);
      return KHTTP_500;
    }

    // misconfiguration
    if (!mw || !strlen(mw)) {
      LOG_WARNV("Middleware: env 'SS_TRUSTED_MIDDLEWARE' defined but empty", 0);
      return KHTTP_500;
    }

    // string check: is middleware trusted ip:port correct?
    if (strncmp(mw, meta->authority, 1024) != 0) {
      LOG_WARNV("Middleware: source is invalid: '%s'", meta->authority);
      return KHTTP_407;
    }

    // headers parsed before
    if (!meta->user || !strlen(meta->user)) {
      LOG_WARNV("Invalid trusted middleware request, required x-headers missing: '%s'", X_HEADER_MW_USER );
      return KHTTP_407;
    }

    if (!meta->group || !strlen(meta->group)) {
      LOG_WARNV("Invalid trusted middleware request, required x-headers missing: '%s'", X_HEADER_MW_GROUP);
      return KHTTP_407;
    }

    // trusted mw needs to authenticate on server level
    if (!req->auth) {
      LOG_WARNV("Middleware request is not authendicated.", 0);
      return KHTTP_407;
    }

    if(!req->rawauth.authorised) {
      LOG_WARNV("Middleware request is not authorised.", 0);
      return KHTTP_407;
    }
  }

  // valid auth
  return KHTTP_200;
}
