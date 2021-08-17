#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "kcgi.h"

#include "survey.h"
#include "fcgi.h"
#include "serialisers.h"
#include "validators.h"
#include "errorlog.h"


/**
 * Fetch the field value (param) for a given key from kreq.fieldmap or
 */
char *fcgi_request_get_field_value(enum key field, struct kreq *req) {
  // man khttp_parse: struct kpair **fieldmap
  if (!req->fieldsz) {
    return NULL;
  }
  struct kpair *pair = req->fieldmap[field];
  return (pair) ? pair->val : NULL;
}

/**
 * Fetch anonymous body from req.fields (since #461)
 */
char *fcgi_request_get_anonymous_body(struct kreq *req) {
  // man khttp_parse: struct kpair struct kpair *fields
  if (!req->fieldsz) {
    return NULL;
  }

  // verify that no key is assigned
  if (req->fields[0].key && strlen(req->fields[0].key)) {
    return  NULL;
  }

  return req->fields[0].val;
}

/**
 * Fetch the field value (param) for a given key (string) from kreq.fields (#260)
 */
char *fcgi_request_get_field_value_by_name(char *field, struct kreq *req) {
  for (size_t i = 0; i < req->fieldsz; i++) {
    if (strcmp(req->fields[i].key, field) == 0) {
      return req->fields[i].val;
    }
  }
  return NULL;
}

/**
 * Fetch consistency sha1 value from request, either ith 'if-Match' header or
 * #260, #268
 */
char *fcgi_request_get_consistency_hash(struct kreq *req) {
  // man khttp_parse: struct kpair struct kpair *fields
  struct khead *header = req->reqmap[KREQU_IF_MATCH];
  if (header) {
    return header->val;
  }

  return fcgi_request_get_field_value(KEY_IF_MATCH, req);
}


/**
 * parse and validate a list of deserialised answers from an incoming kreq (#260)
 *  - validate answers against list of uids and session questions
 *  - No error will be assigned if values could not be found in request
 */
struct answer_list *fcgi_request_parse_answers_serialised(struct kreq *req, struct session *ses, int *error) {
  int retVal = 0;

  struct answer_list *list = NULL;
  struct string_list *uids = NULL;

  do {
    BREAK_IF(req == NULL, SS_ERROR_ARG, "req");
    BREAK_IF(ses == NULL, SS_ERROR_ARG, "ses");

    char *serialised  = fcgi_request_get_field_value(KEY_ANSWER, req);
    if (!serialised) {
      serialised = fcgi_request_get_anonymous_body(req);
    }

    if (!serialised) {
      LOG_INFO("no serialised answers found in request");
      break;
    }

    uids = deserialise_string_list(ses->next_questions, ',');
    BREAK_IF(uids == NULL, SS_ERROR_MEM,  NULL);

    // load answers from request TODO error check in fuction
    list = deserialise_answers(serialised, ANSWER_SCOPE_PUBLIC);
    if (!list) {
      BREAK_CODE(SS_INVALID_ANSWER, "Could not deserialise answer list");
    }

    // validate answer uids against ses->next_questions
    if (list->len != uids->len) {
      BREAK_CODEV(SS_MISMATCH_NEXTQUESTIONS, "answers count doesn't match next questions (%ld != %ld)", list->len, uids->len);
    }

    for (size_t i = 0; i < uids->len; i++) {
      if (strcmp(uids->items[i], list->answers[i]->uid)) {
        BREAK_CODEV(SS_MISMATCH_NEXTQUESTIONS, "Could find answer '%s' in request values", uids->items[i]);
      }
    }

  } while(0);

  free_string_list(uids);

  if (retVal) {
    free_answer_list(list);
    list = NULL;
  }

  *error = retVal;
  return list;
}

/**
 * Parse a list of deserialised answers values (uid1=value1&uid2=value2) from an incoming kreq
 * Note: the *uids list serves only as a lookup for answer keys, the validation for completeness nedds to be done separately
 * #260
 */
struct answer_list *fcgi_request_parse_answers_values(struct kreq *req, struct session *ses, int *error) {
  int retVal = 0;

  struct answer_list *list = NULL;
  struct string_list *uids = NULL;

  do {
    BREAK_IF(req == NULL, SS_ERROR_ARG, "req");
    BREAK_IF(ses == NULL, SS_ERROR_ARG, "ses");

    uids = deserialise_string_list(ses->next_questions, ',');
    BREAK_IF(uids == NULL, SS_ERROR_MEM,  NULL);

    list = calloc(1, sizeof(struct answer_list));
    BREAK_IF(list == NULL, SS_ERROR_MEM, "struct answer list");

    char *value;
    struct question *qn;

    for (size_t i = 0; i < uids->len; i++) {
      // get value
      value = fcgi_request_get_field_value_by_name(uids->items[i], req);
      if (!value) {
        BREAK_CODEV(SS_MISMATCH_NEXTQUESTIONS, "Could find answer '%s' in request values", uids->items[i]);
      }

      // init answer
      list->answers[i] = calloc(sizeof(struct answer), 1);
      BREAK_IF(list == NULL, SS_ERROR_MEM, "struct answer");

      // build answer
      qn = session_get_question(uids->items[i], ses);
      if (!qn) {
        BREAK_CODEV(SS_NOSUCH_QUESTION, "Could  answer '%s' does not exist in session (session corrupted?)", uids->items[i]);
      }
      list->answers[i]->uid = strdup(qn->uid);
      list->answers[i]->type = qn->type;

      // set answer value
      if (answer_set_value_raw(list->answers[i], value)) {
        BREAK_ERRORV("failed to set raw value for answer '%s' in session", uids->items[i]);
      }

      // progress
      list->len++;
    }

    // purge list if no values are found
    if (!list->len) {
      free_answer_list(list);
      list = NULL;
      LOG_INFO("no answer values found in request");
      break;
    }

  } while(0);

  free_string_list(uids);

  if (retVal) {
    free_answer_list(list);
    list = NULL;
  }

  *error = retVal;
  return list;
}


/**
 * Determines (but does not validate) authorisation type from an incoming kcgi request
 * #363
 */
static int get_autorisation_type(struct kreq *req) {
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
 * Validates session meta, parsed from incoming kcgi request, against previously stored session meta (header)
 * We do not manage authorisation or idendity in the backend, only verify if the request source is consitent with thew initial newsession request
 */
static enum khttp validate_session_authority( struct session_meta *meta, struct session *ses) {
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
 * Validates session meta, parsed from incoming kcgi request, against previously stored session meta (header)
 * We do not manage authorisation or idendity in the backend, only verify if the request source is consitent with thew initial newsession request
 */
int validate_session_authority_1(struct session_meta *meta, struct session *ses) {
  int retVal = 0;

  do {
    BREAK_IF(meta == NULL, SS_ERROR_ARG, "meta");
    BREAK_IF(ses == NULL, SS_ERROR_ARG, "ses");

    // parse initial authority from session
    struct answer *authority = session_get_header("@authority", ses);
    if (!authority) {
      BREAK_CODE(SS_CONFIG_MALFORMED_SESSION, "'@authority' header missing");
    }

    // all requests must match provider type
    int provider = authority->value;
    if(provider != meta->provider) {
      BREAK_CODEV(SS_CONFIG_PROXY, "provider type mismatch %d != %d ", provider, meta->provider);
    }

    if (provider != IDENDITY_HTTP_TRUSTED) {
      break; // skip and return ok
    }

    // if middleware: (fcgi env SS_TRUSTED_MIDDLEWARE is set) match current authority (ip, port)
    if (!authority->text || !strlen(authority->text)){
      BREAK_CODE(SS_CONFIG_PROXY, "Invalid request. authority value is empty ");
    }

    if (strcmp(authority->text, meta->authority)) {
      BREAK_CODEV(SS_CONFIG_PROXY, "Invalid request. Provider authority string mismatch '%s' != '%s'", authority->text, meta->authority);
    }
  } while(0);

  return retVal;
}

/**
 * Parses session meta from an incoming kcgi request
 * The returned session_meta structure needs to be freed
 * #363
 */
struct session_meta *fcgi_request_parse_meta(struct kreq *req) {

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
 * Validates incoming kcgi request against a set of allowed methods
 * pass [KMETHOD__MAX]  for allowing all
 */
enum khttp fcgi_request_validate_method(struct kreq *req, enum kmethod allowed[], size_t length) {

  for (size_t i = 0; i < length; i++) {
    if (allowed[i] == KMETHOD__MAX) {
      // allow all
      return KHTTP_200;
    }

    if (allowed[i] == req->method) {
      return KHTTP_200;
    }
  }

  return KHTTP_405;
}

/**
 * Validate a loaded session against request
 * The backend does not manage authorisation or idendity, it relies on outer wrappers,
 * We only verify if the request source is consitent with thew initial newsession request
 */
enum khttp fcgi_request_validate_meta_session(struct kreq *req, struct session *ses) {
  enum khttp status;
  struct session_meta *meta = NULL;

  if (!req) {
    LOG_WARNV("Cannot validate request. request is null for session '%s'.", ses->session_id);
    return KHTTP_500;
  }

  if (!ses) {
    LOG_WARNV("Cannot validate request. session is null.", 0);
    return KHTTP_500;
  }

  // #363 parse session meta
  meta = fcgi_request_parse_meta(req);
  if (!meta) {
    LOG_WARNV("create_session_meta_kreq() failed",  0);
    return KHTTP_500;
  }

  // validate session meta against request
  status = fcgi_request_validate_meta_kreq(req, meta);
  if (status != KHTTP_200) {
    free_session_meta(meta);
    LOG_WARNV("fcgi_request_validate_meta_kreq() status %d != (%d)", KHTTP_200, status);
    return status;
  }

  // validate session meta against session
  status = validate_session_authority(meta, ses);
  if (status != KHTTP_200) {
    free_session_meta(meta);
    LOG_WARNV("validate_session_authority() status %d != (%d)", KHTTP_200, status);
    return status;
  }

  free_session_meta(meta);
  return KHTTP_200;
}

/**
 * Validates incoming kcgi request against server config, using previously parsed session meta
 * #363
 */
enum khttp fcgi_request_validate_meta_kreq(struct kreq *req, struct session_meta *meta) {

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
      LOG_WARNV("Middleware request is not authenticated.", 0);
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
 * Validates incoming kcgi request against server config, using previously parsed session meta
 * #363
 * #260 renamed fcgi_request_validate_meta_kreq()
 */
int fcgi_request_validate_session_idendity(struct kreq *req, struct session_meta *meta) {
  int retVal = 0;

  do {
    BREAK_IF(req == NULL, SS_ERROR_ARG, "req");
    BREAK_IF(meta == NULL, SS_ERROR_ARG, "meta");

    // no authorisation
    if (meta->provider <= IDENDITY_HTTP_PUBLIC) {
      break;
    }

    // direct auth via basic or digest
    if (meta->provider == IDENDITY_HTTP_BASIC || meta->provider == IDENDITY_HTTP_DIGEST) {
      // authorisation on server level passed? this should normally not occur and indicates some misconfiguration
      if (!req->auth || !req->rawauth.authorised) {
        BREAK_CODEV(SS_INVALID_CREDENTIALS, "invalid authorisation (provider: %d)", meta->provider);
      }
    }

    // indirect auth via trusted (and authorised) middleware
    if (meta->provider == IDENDITY_HTTP_TRUSTED) {
      char *mw = getenv("SS_TRUSTED_MIDDLEWARE");

      // misconfiguration
      if (!mw) {
        BREAK_CODE(SS_CONFIG_PROXY, "Middleware authorisation, env 'SS_TRUSTED_MIDDLEWARE' missing");
      }

      // guard, in case meta was not handed through parsing
      if (!meta->authority) {
        BREAK_CODEV(SS_CONFIG_PROXY, "Middleware authorisation, authority missing (provider: %d)", meta->provider);
      }

      // string check: is middleware trusted ip:port correct?
      if (strncmp(mw, meta->authority, 1024) != 0) {
        BREAK_CODEV(SS_CONFIG_PROXY, "Middleware: source is invalid: '%s'", meta->authority);
      }

      // headers parsed before
      if (!meta->user || !strlen(meta->user)) {
        BREAK_CODE(SS_INVALID_CREDENTIALS_PROXY, "Invalid trusted middleware request, X_HEADER_MW_USER missing");
      }

      if (!meta->group || !strlen(meta->group)) {
        BREAK_CODE(SS_INVALID_CREDENTIALS_PROXY, "Invalid trusted middleware request, X_HEADER_MW_GROUP missing");
      }

      // trusted mw needs to authenticate on server level
      if (!req->auth) {
        BREAK_CODE(SS_INVALID_CREDENTIALS_PROXY, "Middleware request is not authenticated.");
      }

      if(!req->rawauth.authorised) {
        BREAK_CODE(SS_INVALID_CREDENTIALS_PROXY, "Middleware request is not authorised.");
      }
    }
  } while(0);

  // valid auth
  return retVal;
}

/**
 * Fetch and desrialise the kreq 'answer' param to an answer struct.
 * - param has to be validate beforehand
 */
struct answer *fcgi_request_load_answer(struct kreq *req) {
  int retVal = 0;
  struct answer *ans = NULL;

  do {
    BREAK_IF(req == NULL, SS_ERROR_ARG, "req");

    char *serialised = fcgi_request_get_field_value(KEY_ANSWER, req);

    // Deserialise answer
    ans = calloc(sizeof(struct answer), 1);
    if (!ans) {
      BREAK_ERRORV("calloc() of answer structure failed.", 0);
    }

    if (deserialise_answer(serialised, ANSWER_SCOPE_PUBLIC, ans)) {
      BREAK_ERRORV("deserialise_answer() failed.", 0);
    }
  } while(0);

  if (retVal) {
    free_answer(ans);
    return NULL;
  }

  return ans;
}

/**
 * Fetch and desrialise the kreq 'sessionid' param to a session struct.
 * - param has to be validate beforehand
 */
struct session *fcgi_request_load_session(struct kreq *req) {
  int retVal = 0;
  struct session *ses = NULL;

  do {
    int err;
    BREAK_IF(req == NULL, SS_ERROR_ARG, "req");

    char *session_id = fcgi_request_get_field_value(KEY_SESSION_ID, req);
    if (validate_session_id(session_id)) {
      BREAK_ERROR("Invalid survey id");
    }

    // joerg: break if session could not be updated
    if (lock_session(session_id)) {
      BREAK_ERRORV("failed to lock session '%s'", session_id);
    }

    ses = load_session(session_id, &err);
    if (!ses) {
      BREAK_CODEV(err, "Could not load session '%s'", session_id);
    }
  } while(0);

  if (retVal) {
    return NULL;
  }

  return ses;
}


/**
 * Fetch and desrialise the kreq 'sessionid' param to a session struct.
 * - param has to be validate beforehand
 */
struct session *fcgi_request_load_and_verify_session(struct kreq *req, enum actions action, int *error) {
  int retVal = 0;

  struct session *ses = NULL;
  struct session_meta *meta = NULL;

  do {
    *error = 0;
    int res;

    BREAK_IF(req == NULL, SS_ERROR_ARG, "req");

    char *session_id = fcgi_request_get_field_value(KEY_SESSION_ID, req);
    if (validate_session_id(session_id)) {
      BREAK_CODEV(SS_INVALID_SESSION_ID, "session: '%s'", (session_id) ? session_id : "(null)");
    }

    // joerg: break if session could not be updated
    if (lock_session(session_id)) {
      BREAK_CODEV(SS_SYSTEM_LOCK_SESSION, "session: '%s'", session_id);
    }

    // TODO verify path before loading, separate errors
    ses = load_session(session_id, &res);
    if (!ses) {
      BREAK_CODEV(res, "session: '%s'", session_id);
    }

    // #363 parse session meta
    meta = fcgi_request_parse_meta(req);
    if (!meta) {
      BREAK_CODEV(SS_SYSTEM_LOAD_SESSION_META, "session: '%s'", session_id);
    }

    // validate request against session meta (#363)
    res = fcgi_request_validate_session_idendity(req, meta);
    if (res) {
      BREAK_CODEV(res, "session: '%s'", session_id);
    }

    // validate session meta against session
    res = validate_session_authority_1(meta, ses);
    if (res) {
      BREAK_CODEV(res, "session: '%s'", session_id);
    }

    // validate requested action against session current state (#379)
    char reason[1024]; // TODO remove reason
    res = validate_session_action(action, ses, reason, 1024);
    if (res) {
      BREAK_CODEV(res, "session: '%s', reason: '%s';", reason);
    }

  } while(0);

  free_session_meta(meta);

  if (retVal) {
    free_session(ses);
    ses = NULL;
  }

  *error = retVal;
  return ses;
}
