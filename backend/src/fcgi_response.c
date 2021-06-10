#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "kcgi.h"
#include "kcgijson.h"

#include "question_types.h"
#include "survey.h"
#include "fcgi.h"
#include "errorlog.h"


/**
 * Open an HTTP response with a status code, a content-type and a mime type, then open the HTTP content body.
 * #268: add optional consistency sha1 - if passing session->consistency_hash: you need to free the session after this call :)
 */
void http_open(struct kreq *req, enum khttp status, enum kmime mime, char *etag) {
  enum kcgi_err er;

  do {
    // Emit 200 response
    er = khttp_head(req, kresps[KRESP_STATUS], "%s", khttps[status]);
    if (KCGI_HUP == er) {
      fprintf(stderr, "khttp_head: interrupt\n");
      continue;
    } else if (KCGI_OK != er) {
      fprintf(stderr, "khttp_head: error: %d\n", er);
      break;
    }

    // Emit mime-type
    er = khttp_head(req, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[mime]);
    if (KCGI_HUP == er) {
      fprintf(stderr, "khttp_head: interrupt\n");
      continue;
    } else if (KCGI_OK != er) {
      fprintf(stderr, "khttp_head: error: %d\n", er);
      break;
    }

    // #268, Emit session consistency_sha as Etag header
    er = khttp_head(req, kresps[KRESP_ETAG], "%s", (etag) ? etag : "");
    if (KCGI_HUP == er) {
      fprintf(stderr, "khttp_head: interrupt\n");
      continue;
    } else if (KCGI_OK != er) {
      fprintf(stderr, "khttp_head: error: %d\n", er);
      break;
    }

    // Begin sending body
    er = khttp_body(req);
    if (KCGI_HUP == er) {
      fprintf(stderr, "khttp_body: interrupt\n");
      continue;
    } else if (KCGI_OK != er) {
      fprintf(stderr, "khttp_body: error: %d\n", er);
      break;
    }

  } while (0);
}

/**
 * Open an HTTP response with an json error body
 */
void http_json_error(struct kreq *req, enum khttp status, const char *msg) {
  int retVal = 0;

  do {
    http_open(req, status, KMIME_APP_JSON, NULL);

    struct kjsonreq jsonreq;
    kjson_open(&jsonreq, req);
    kcgi_writer_disable(req);
    kjson_obj_open(&jsonreq);

    // Write some stuff in reply
    kjson_putstringp(&jsonreq, "message", msg);

    // Display error log as well.
    kjson_stringp_open(&jsonreq, "trace");
    for (int i = 0; i < error_count; i++) {
      char line[1024];
      snprintf(line, 1024, "%s\n", error_messages[i]);
      kjson_string_puts(&jsonreq, line);
    }
    kjson_string_close(&jsonreq);

    kjson_obj_close(&jsonreq);
    kjson_close(&jsonreq);

  } while (0);

  (void)retVal;
  return;
}

static void response_nextquestion_add_choices(struct question *q , struct kjsonreq *resp) {
  // open "choices"
  kjson_arrayp_open(resp, "choices");
  size_t len = strlen(q->choices);

  if (!len) {
    kjson_array_close(resp);
    return;
  }

  char choice[1024] = { 0 };
  int i = 0;
  int k = 0;

  switch (q->type) {
    case QTYPE_MULTICHOICE:
    case QTYPE_MULTISELECT:
    // #98 add single checkbox choices
    case QTYPE_SINGLESELECT:
    case QTYPE_SINGLECHOICE:
    case QTYPE_CHECKBOX:
    // #205 add sequence fields
    case QTYPE_FIXEDPOINT_SEQUENCE:
    case QTYPE_DAYTIME_SEQUENCE:
    case QTYPE_DATETIME_SEQUENCE:
    case QTYPE_DIALOG_DATA_CRAWLER:

    for (i = 0; i < len; i++) {
      if (q->choices[i] == ',') {
        choice[k + 1] = 0;
        kjson_putstring(resp, choice);
        // reset choice
        k = 0;
        choice[0] = 0;
      } else {
        choice[k] = q->choices[i];
        choice[k + 1] = 0;
        k++;
      }
    }

    // last element
    choice[k] = 0;
    if (k) {
      kjson_putstring(resp, choice);
    }

    break;

  default:
    break;
  } // switch

  // close "choices"
  kjson_array_close(resp);
  return;
}

/**
 * Render next_question JSON response
 * #373, #363, #379
 */
int fcgi_response_nextquestion(struct kreq *req, struct session *ses, struct nextquestions *nq) {
  int retVal = 0;

  do {
    if (!req) {
       LOG_ERROR("response_nextquestion(): kreq required (null)");
       break;
    }
    if (!ses) {
      LOG_ERROR("response_nextquestion(): session required (null)");
    }
    if (!nq) {
      LOG_ERROR("response_nextquestion(): nextquestions required (null)");
    }

    // json response

    struct kjsonreq resp;
    http_open(req, KHTTP_200, KMIME_APP_JSON, ses->consistency_hash);
    kjson_open(&resp, req);
    kcgi_writer_disable(req);

    // open nq object
    kjson_obj_open(&resp);
    // #332 add status, message
    kjson_putintp(&resp, "status", nq->status);
    kjson_putstringp(&resp, "message", (nq->message != NULL) ? nq->message : "");

    // #13 count given answers
    kjson_arrayp_open(&resp, "progress");
    kjson_putint(&resp, nq->progress[0]);
    kjson_putint(&resp, nq->progress[1]);
    kjson_array_close(&resp);

    // next questions
    kjson_arrayp_open(&resp, "next_questions");
    for (int i = 0; i < nq->question_count; i++) {
      // Output each question
      kjson_obj_open(&resp);
      kjson_putstringp(&resp, "id",            nq->next_questions[i]->uid);
      kjson_putstringp(&resp, "name",          nq->next_questions[i]->uid);
      kjson_putstringp(&resp, "title",         nq->next_questions[i]->question_text);
      kjson_putstringp(&resp, "description",   nq->next_questions[i]->question_html);
      kjson_putstringp(&resp, "type",          question_type_names[nq->next_questions[i]->type]);
      kjson_putstringp(&resp, "default_value", nq->next_questions[i]->default_value);

      // #341 add min/max values, man kjson_putintp
      kjson_putintp(&resp, "min_value", (int64_t) nq->next_questions[i]->min_value);
      kjson_putintp(&resp, "max_value", (int64_t) nq->next_questions[i]->max_value);

      // #384, refactor due to an occasional memory overflow (outside?) issue when assembling choices, resulting in a never ending while loop
      response_nextquestion_add_choices(nq->next_questions[i], &resp);

      // #72 unit field
      kjson_putstringp(&resp, "unit", nq->next_questions[i]->unit);
      kjson_obj_close(&resp);

    } // endfor

    kjson_array_close(&resp);
    kjson_obj_close(&resp);
    kjson_close(&resp);

    LOG_INFO("End next questions handler.");

  } while(0);

  return retVal;
}
