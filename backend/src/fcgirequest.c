#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "kcgi.h"

#include "survey.h"
#include "fcgirequest.h"
#include "errorlog.h"

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
 * Determines (but does not validate) authorisation type from an incoming kcgi request
 * #363
 */
int get_autorisation_type(struct kreq *req) {
  // is trusted middleware
  if (getenv("SS_TRUSTED_MIDDLEWARE")) {
    return IDENDITY_HTTP_TRUSTED;
  }

  // direct auth (server)
  if (req->auth) {
    if (req->rawauth.type == KAUTH_BASIC) {
      return IDENDITY_HTTP_BASIC;
    }

    if (req->rawauth.type == KAUTH_DIGEST) {
      return IDENDITY_HTTP_DIGEST;
    }
  }

  // no auth
  return IDENDITY_HTTP_PUBLIC;
}

/**
 * Parses session meta from an incoming kcgi request
 * The returned session_meta structure needs to be freed
 * #363
 */
struct session_meta *fcgirequest_parse_session_meta(struct kreq *req) {

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

  // #363 parse session meta
  struct session_meta *meta = calloc(sizeof(struct session_meta), 1);
  if (!meta) {
    LOG_WARNV("Could not calloc() session_meta structure.", 0);
    return NULL;
  }

  // authority
  char authority[1024];
  int l = snprintf(authority, 1024, "%s(%hu)", req->remote, req->port);
  if (l < 1) {
    LOG_WARNV("idendity provider snprintf() for authority failed", 0);
    free(meta);
    return NULL;
  }
  meta->authority = strndup(authority, 1024);

  // authorisation type
  meta->provider = get_autorisation_type(req);

  // user and group
  if (meta->provider == IDENDITY_HTTP_DIGEST ) {
    meta->user = strndup(req->rawauth.d.digest.user, 1024);
    meta->group = strndup(req->rawauth.d.digest.realm, 1024);
    return meta;
  }

  if (meta->provider == IDENDITY_HTTP_BASIC ) {
    meta->user = strndup(req->rawauth.d.basic.response, 1024);
    meta->group = NULL;
    return meta;
  }

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
    return meta;
  }

  //IDENDITY_HTTP_PUBLIC
  meta->user = NULL;
  meta->group = NULL;
  return meta;
}

/**
 * Validates incoming kcgi request against server config, using previously parsed session meta
 * #363
 */
enum khttp fcgirequest_validate_request(struct kreq *req, struct session_meta *meta) {

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

/**
 * Validates session meta, parsed from incoming kcgi request, against previously stored session meta (header)
 * We do not manage authorisation or idendity in the backend, only verify if the request source is consitent with thew initial newsession request
 */
enum khttp fcgirequest_validate_session_authority( struct session_meta *meta, struct session *ses) {
  if (!meta) {
    LOG_WARNV("Cannot validate request. request meta is null for session '%s'.", ses->session_id);
    return KHTTP_500;
  }

  if (!ses) {
    LOG_WARNV("Cannot validate request. session is null.", 0);
    return KHTTP_500;
  }

  // parse initial authority from session
  struct answer *authority = session_get_header("@authority", ses);
  if (!authority) {
    LOG_WARNV("'@authority' header missing in session '%s'", ses->session_id);
    return KHTTP_502;
  }

  // all requests must match provider type
  int provider = authority->value;
  if(provider != meta->provider) {
    LOG_WARNV("Invalid request. Provider type mismatch %d != %d for session '%s'.", provider, meta->provider, ses->session_id);
    return KHTTP_502;
  }

  if (provider != IDENDITY_HTTP_TRUSTED) {
    return KHTTP_200;
  }

  // if middleware: (fcgi env SS_TRUSTED_MIDDLEWARE is set) match current authority (ip, port)
  if (!authority->text || !strlen(authority->text)){
    LOG_WARNV("Invalid request. authority value is empty in session '%s'.", ses->session_id);
    return KHTTP_502;
  }
  if (strcmp(authority->text, meta->authority)) {
    LOG_WARNV("Invalid request. Provider authority string mismatch '%s' != '%s' for session '%s'.", authority->text, meta->authority, ses->session_id);
    return KHTTP_502;
  }

  return KHTTP_200;
}

/**
 * Validate a loaded session against request
 * The backend does not manage authorisation or idendity, it relies on outer wrappers,
 * We only verify if the request source is consitent with thew initial newsession request
 */

enum khttp fcgirequest_validate_session_request(struct kreq *req, struct session *ses) {
  enum khttp status;
  struct session_meta *meta = NULL;

  if (!req) {
    LOG_WARNV("Cannot validate request. request meta is null for session '%s'.", ses->session_id);
    return KHTTP_500;
  }

  if (!ses) {
    LOG_WARNV("Cannot validate request. session is null.", 0);
    return KHTTP_500;
  }

  // #363 parse session meta
  meta = fcgirequest_parse_session_meta(req);
  if (!meta) {
    LOG_WARNV("create_session_meta_kreq() failed",  0);
    return KHTTP_500;
  }

  // validate session meta against request
  status = fcgirequest_validate_request(req, meta);
  if (status != KHTTP_200) {
    free_session_meta(meta);
    LOG_WARNV("fcgirequest_validate_request() status %d != (%d)", KHTTP_200, status);
    return status;
  }

  // validate session meta against session
  status = fcgirequest_validate_session_authority(meta, ses);
  if (status != KHTTP_200) {
    free_session_meta(meta);
    LOG_WARNV("fcgirequest_validate_session_authority() status %d != (%d)", KHTTP_200, status);
    return status;
  }

  free_session_meta(meta);
  return KHTTP_200;
}
