/*
  Serialisers and de-serialisers for various structures.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errorlog.h"
#include "question_types.h"
#include "survey.h"

#define REPORT_IF_FAILED()                                                     \
  {                                                                            \
    if (retVal)                                                                \
      fprintf(stderr, "%s:%d: %s() failed.\n", __FILE__, __LINE__,             \
              __FUNCTION__);                                                   \
  }

/*
  Escape a string so that it can be safely embedded in a CSV file, that uses colons as delimeters
*/
int escape_string(char *in, char *out, int max_len) {
  int retVal = 0;
  int out_len = 0;
  for (int i = 0; in[i]; i++) {

    switch (in[i]) {

    case '\r':
    case '\n':
    case '\t':
    case '\b':
      if (out_len >= max_len) {
        LOG_ERRORV("escaped version of string '%s' is too long", out);
      } else {
        out[out_len++] = '\\';
      }

      if (out_len >= max_len) {
        LOG_ERRORV("escaped version of string '%s' is too long", out);
      } else {
        out[out_len++] = in[i];
      }
      break;

    case ':':
    case '\\':
      if (out_len >= max_len) {
        LOG_ERRORV("escaped version of string '%s' is too long", out);
      } else {
        out[out_len++] = ':';
      }

      if (out_len >= max_len) {
        LOG_ERRORV("escaped version of string '%s' is too long", out);
      } else {
        out[out_len++] = in[i];
      }
      break;

    default:
      if (out_len >= max_len) {
        LOG_ERRORV("escaped version of string '%s' is too long", out);
      } else {
        out[out_len++] = in[i];
      }
      break;

    } // endswitch
  }   // endfor

  out[out_len] = 0;

  if (retVal == 0) {
    retVal = out_len;
  }
  return retVal;
}

/*
  Write out an integer in a format that can be embedded in a CSV file
*/
int serialise_int(int in, char *out, int max_len) {
  int retVal = 0;
  char temp[16];
  do {
    snprintf(temp, 16, "%d", in);
    if (strlen(temp) >= max_len) {
      LOG_ERRORV("integer converts to over-long string '%s...'", temp);
    } else {
      strcpy(out, temp);
      retVal = strlen(temp);
    }
  } while (0);
  return retVal;
}

/*
  Write out a 64-bit integer in a format that can be embedded in a CSV file
*/
int serialise_longlong(long long in, char *out, int max_len) {
  int retVal = 0;
  char temp[32];
  do {
    snprintf(temp, 32, "%lld", in);
    if (strlen(temp) >= max_len) {
      LOG_ERRORV("long long converts to over-long string '%s...'", temp);
    } else {
      strcpy(out, temp);
      retVal = strlen(temp);
    }
  } while (0);
  return retVal;
}

/*
  Check if adding append_len to existing_len would exceed max_len
*/
int space_check(int append_len, int existing_len, int max_len) {
  int retVal = 0;
  do {
    if ((existing_len + append_len) > max_len) {
      LOG_ERRORV("Insufficient space to append string of %d chars", append_len);
    }
  } while (0);
  return retVal;
}

/*
  Parse the next colon delimited field from the input
  string in, at position *in_offset.
  De-escape colons and selected control characters that we
  allow.
*/
int deserialise_parse_field(char *in, int *in_offset, char *out) {
  int retVal = 0;
  int offset = *in_offset;
  int olen = 0;

  do {
    if (offset) {
      if (in[offset] != ':') {
        LOG_ERRORV("Expected : before next field when deseralising at offset "
                   "%d of '%s'\n",
                   offset, in);
        break;
      } else {
        offset++;
      }
    }

    out[olen] = 0;
    if (!in)
      LOG_ERROR("input string is NULL");
    if (!in[0])
      LOG_ERROR("input string is empty");

    for (; in[offset] && (olen < 16383) && in[offset] != ':'; offset++) {
      // Allow some \ escape characters
      if (in[offset] == '\\') {
        if (!in[offset + 1])
          LOG_ERRORV("String '%s' ends in \\\n", in);
        switch (in[offset + 1]) {
        case ':':
        case '\\':
          out[olen++] = in[offset + 1];
          break;
        case 'r':
          out[olen++] = '\r';
          break;
        case 'n':
          out[olen++] = '\n';
          break;
        case 'b':
          out[olen++] = '\b';
          break;
        default:
          LOG_ERRORV("Illegal escape character 0x%02x at offset %d of '%s'\n",
                     in[offset + 1], offset + 1, in);
          break;
        }
        offset++;
        out[olen] = 0;
      } else {
        out[olen++] = in[offset];
        out[olen] = 0;
      }
    } // endfor

    *in_offset = offset;

  } while (0);

  return retVal;
}

/*
  Take an integer from a CSV field, and turn it back into a C int
*/
int deserialise_int(char *field, int *s) {
  int retVal = 0;
  do {
    if (!field)
      LOG_ERROR("field is NULL");
    if (!strlen(field))
      LOG_ERROR("field is empty string");

    int offset = 0;
    if (field[offset] == '-') {
      offset++;
    }

    for (int i = offset; field[i]; i++) {
      if (field[i] < '0' || field[i] > '9') {
        LOG_ERRORV("integer field '%s' contains non-digit", field);
      }
    }

    if (!retVal) {
      *s = atoi(field);
    }

  } while (0);
  return retVal;
}

/*
  Take a 64-bit integer from a CSV field, and turn it back into a C long long
*/
int deserialise_longlong(char *field, long long *s) {
  int retVal = 0;
  do {
    if (!field)
      LOG_ERROR("field is NULL");
    if (!strlen(field))
      LOG_ERROR("field is empty string");

    int offset = 0;
    if (field[offset] == '-') {
      offset++;
    }

    for (int i = offset; field[i]; i++) {
      if (field[i] < '0' || field[i] > '9') {
        LOG_ERRORV("long long field '%s' contains non-digit", field);
      }
    }

    if (!retVal) {
      *s = atoll(field);
    }

  } while (0);
  return retVal;
}

/*
  Take a string from a CSV field, and store it in a C char *.
  (This is mostly a pass-through, and exists so that there is a common
  interface for all deserialisation functions, so that the short-hand macros
  can be used to make the top-level serialisation and deserialisation code
  work.
*/
int deserialise_string(char *field, char **s) {
  int retVal = 0;
  do {
    *s = NULL;
    if (!field) {
      LOG_ERROR("field is NULL");
    } else {
      *s = strdup(field);
    }

    if (!*s) {
      LOG_ERROR("field is empty string");
    }
  } while (0);
  return retVal;
}

/*
  A set of macros that we use to perform deserialisation.
  _BEGIN sets up the necessary environment, including providing the
  string buffer to store each parsesd field.
  _COMPLETE ends the same, and checks that we used the entire string.
  _NEXT_FIELD parses out the next field, ready for reconstituting.
  _THING calls _NEXT_FIELD, and then calls the appropriate deserialiser
  to parse the field, and store it into the correct field of the data
  structure.
  The remaining three are wrappers for _THING that provide the most
  common deserialisers as arguments, to make the high-level deserialisation
  code as simple as possible.

  All macros eventually will call LOG_ERROR and break out of the parsing
  if any error occurs, and will record the specific error, making it much easier
  to debug when things go wrong.
*/
#define DESERIALISE_BEGIN(O, L, ML)                                            \
  {                                                                            \
    int offset = 0;                                                            \
    char field[16384];
#define DESERIALISE_COMPLETE(O, L, ML)                                         \
  if (offset < L) {                                                            \
    LOG_ERROR("Junk at end of serialised object");                             \
  }                                                                            \
  }
#define DESERIALISE_NEXT_FIELD()                                               \
  if (deserialise_parse_field(in, &offset, field)) {                           \
    LOG_ERRORV("failed to parse next field '%s' at offset %d", &in[offset],    \
               offset);                                                        \
  }
#define DESERIALISE_THING(S, DESERIALISER)                                     \
  DESERIALISE_NEXT_FIELD();                                                    \
  if (DESERIALISER(field, &S)) {                                               \
    LOG_ERRORV("call to " #DESERIALISER " failed with string '%s'", field);    \
  }
#define DESERIALISE_INT(S) DESERIALISE_THING(S, deserialise_int)
#define DESERIALISE_STRING(S) DESERIALISE_THING(S, deserialise_string);
#define DESERIALISE_LONGLONG(S) DESERIALISE_THING(S, deserialise_longlong)

/*
  We then have a similar set of macros for the serialisation process.
  APPEND_STRING and APPEND_COLON append to the serialised string being
  built.

  We then have _BEGIN and _COMPLETE macros, similar for deserialisation.
  Also, the _THING and derivative macros perform the opposite of the
  DESERIALISE_* equivalents.  The result is that the serialiser and
  deserialiser code at the top level looks almosts identical, except for
  the DE prefix on the macros for deserialising.  This is designed to help
  keep the serialiser and deserialiser functions for given structures in
  step, even if the structure contents evolves over time.
*/
#define APPEND_STRING(NEW, NL, O, L)                                           \
  {                                                                            \
    strcpy(&O[L], NEW);                                                        \
    L += NL;                                                                   \
  }

#define APPEND_COLON(O, L, ML)                                                 \
  {                                                                            \
    if (space_check(1, L, ML)) {                                               \
      LOG_ERROR("serialised string too long");                                 \
    }                                                                          \
    O[L++] = ':';                                                              \
    O[L] = 0;                                                                  \
  }

#define SERIALISE_BEGIN(O, L, ML)                                              \
  {                                                                            \
    int encoded_len = 0;                                                       \
    const int encoded_max_len = 65536;                                         \
    char encoded[encoded_max_len];                                             \
    L = 0;

#define SERIALISE_COMPLETE(O, L, ML)                                           \
  if (L > 0) {                                                                 \
    L--;                                                                       \
    O[L] = 0;                                                                  \
  }                                                                            \
  }

#define SERIALISE_THING(S, SERIALISER)                                         \
  encoded_len = SERIALISER(S, encoded, encoded_max_len);                       \
  if (encoded_len < 0)                                                         \
    break;                                                                     \
  if (space_check(encoded_len, len, max_len))                                  \
    break;                                                                     \
  APPEND_STRING(encoded, encoded_len, out, len);                               \
  APPEND_COLON(out, len, max_len);

#define SERIALISE_STRING(S) SERIALISE_THING(S, escape_string);
#define SERIALISE_INT(S) SERIALISE_THING(S, serialise_int);
#define SERIALISE_LONGLONG(S) SERIALISE_THING(S, serialise_longlong);

/*
  Question types are an ENUM, and for clarity in the question definitions we
  store the strings rather than uninteligible ENUM index values.
 */
int serialise_question_type(int qt, char *out, int out_max_len) {
  int retVal = 0;
  do {
    if (qt < 1)
      LOG_ERRORV("was asked to serialise an illegal question type #%d", qt);
    if (qt > NUM_QUESTION_TYPES)
      LOG_ERRORV("was asked to serialise an illegal question type #%d", qt);
    if (strlen(question_type_names[qt]) >= out_max_len)
      LOG_ERRORV("question type '%s' name too long", question_type_names[qt]);

    strcpy(out, question_type_names[qt]);
    retVal = strlen(out);
  } while (0);

  REPORT_IF_FAILED();
  return retVal;
}

int deserialise_question_type(char *field, int *s) {
  int retVal = 0;
  do {
    int qt;

    for (qt = 1; qt < NUM_QUESTION_TYPES; qt++) {
      if (!strcasecmp(field, question_type_names[qt])) {
        retVal = 0;
        *s = qt;
        break;
      }
    }

    if (qt == NUM_QUESTION_TYPES)
      LOG_ERRORV("invalid question type name '%s'", field);
  } while (0);

  return retVal;
}

/*
  Top-level function for serialising a question that has been passed in
  in a struct question.  It uses the various macros defined above to
  make a very clear and succinct description of what is required.
*/
int serialise_question(struct question *q, char *out, int max_len) {
  int retVal = 0;

  do {
    int len = 0;
    SERIALISE_BEGIN(out, len, max_len);

    SERIALISE_STRING(q->uid);
    SERIALISE_STRING(q->question_text);
    SERIALISE_STRING(q->question_html);
    SERIALISE_THING(q->type, serialise_question_type);
    SERIALISE_INT(q->flags);
    SERIALISE_STRING(q->default_value);
    SERIALISE_LONGLONG(q->min_value);
    SERIALISE_LONGLONG(q->max_value);
    SERIALISE_INT(q->decimal_places);
    SERIALISE_INT(q->num_choices);
    SERIALISE_STRING(q->choices);
    // #72 unit field
    SERIALISE_STRING(q->unit);
    // Trim terminal separator character
    SERIALISE_COMPLETE(out, len, max_len);

  } while (0);

  return retVal;
}

/*
  For debugging it can be helpful to dump a question structure to
  stdout or a file.
*/
int dump_question(FILE *f, char *msg, struct question *q) {
  int retVal = 0;
  do {
    char temp[8192];
    fprintf(f, "%s:\n", msg);
    escape_string(q->uid, temp, 8192);
    fprintf(f, "  uid='%s'\n", temp);
    escape_string(q->question_text, temp, 8192);
    fprintf(f, "  question_text='%s'\n", temp);
    escape_string(q->question_html, temp, 8192);
    fprintf(f, "  question_html='%s'\n", temp);
    fprintf(f, "  question type=%s\n",
            ((q->type >= 1) && (q->type <= NUM_QUESTION_TYPES))
                ? question_type_names[q->type]
                : "<unknown>");
    fprintf(f, "  flags=0x%08X\n", q->flags);
    escape_string(q->default_value, temp, 8192);
    fprintf(f, "  default_value='%s'\n", temp);
    fprintf(f, "  min_value=%lld\n", q->min_value);
    fprintf(f, "  max_value=%lld\n", q->max_value);
    fprintf(f, "  decimal_places=%d\n", q->decimal_places);
    fprintf(f, "  num_choices=%d\n", q->num_choices);
    // #72 unit field
    fprintf(f, "  unit=%s\n", q->unit);
  } while (0);

  return retVal;
}

/*
  For debugging it can be helpful to dump an answer structure to
  stdout or a file.
*/
int dump_answer(FILE *f, char *msg, struct answer *a) {
  int retVal = 0;
  do {
    char temp[8192];
    fprintf(f, "%s:\n", msg);
    escape_string(a->uid, temp, 8192);
    fprintf(f, "  uid='%s'\n", temp);
    escape_string(a->text, temp, 8192);
    fprintf(f, "  text='%s'\n", temp);
    fprintf(f, "  value=%lld\n", a->value);
    fprintf(f, "  lat=%lld\n", a->lat);
    fprintf(f, "  lon=%lld\n", a->lon);
    fprintf(f, "  time_begin=%lld\n", a->time_begin);
    fprintf(f, "  time_end=%lld\n", a->time_end);
    fprintf(f, "  time_zone_delta=%d\n", a->time_zone_delta);
    fprintf(f, "  dst_delta=%d\n", a->dst_delta);
    escape_string(a->unit, temp, 8192);
    fprintf(f, "  unit='%s'\n", temp);
    fprintf(f, "  flags=%d\n", a->flags);
    fprintf(f, "  stored=%lld\n", a->stored);
  } while (0);

  return retVal;
}

/*
  This should match exactly the field order and types as used
  in serialise_question(), so that it can reconstute a
  question structure from a serialised string version of a
  question.
 */
int deserialise_question(char *in, struct question *q) {
  int retVal = 0;
  int len = 0;
  do {
    DESERIALISE_BEGIN(out, len, max_len);

    DESERIALISE_STRING(q->uid);
    DESERIALISE_STRING(q->question_text);
    DESERIALISE_STRING(q->question_html);
    DESERIALISE_THING(q->type, deserialise_question_type);
    DESERIALISE_INT(q->flags);
    DESERIALISE_STRING(q->default_value);
    DESERIALISE_LONGLONG(q->min_value);
    DESERIALISE_LONGLONG(q->max_value);
    DESERIALISE_INT(q->decimal_places);
    DESERIALISE_INT(q->num_choices);
    DESERIALISE_STRING(q->choices);
    // #72 unit field
    DESERIALISE_STRING(q->unit);
    // Check that we are at the end of the input string
    DESERIALISE_COMPLETE(out, len, max_len);

  } while (0);

  return retVal;
}

/*
  Similar to serialise_question(), but for answer structures.
 */
int serialise_answer(struct answer *a, char *out, int max_len) {
  int retVal = 0;
  do {

    int len = 0;
    SERIALISE_BEGIN(out, len, max_len);

    SERIALISE_STRING(a->uid);
    SERIALISE_STRING(a->text);
    SERIALISE_LONGLONG(a->value);
    SERIALISE_LONGLONG(a->lat);
    SERIALISE_LONGLONG(a->lon);
    SERIALISE_LONGLONG(a->time_begin);
    SERIALISE_LONGLONG(a->time_end);
    SERIALISE_INT(a->time_zone_delta);
    SERIALISE_INT(a->dst_delta);
    // #72 unit field
    SERIALISE_STRING(a->unit);
    // #186 - Add "answer deleted" flag
    SERIALISE_INT(a->flags);
    // #162 storage timestamp
    SERIALISE_LONGLONG(a->stored);

    // Trim terminal separator character
    SERIALISE_COMPLETE(out, len, max_len);
  } while (0);

  return retVal;
}

/*
  The top-level function for converting a CSV string
  representation of an answer structure back into a live
  structure.  As with {de,}serialise_question(),
  this function and serialise_answer() must be matched
  in the order and list of fields that they process.
 */
int deserialise_answer(char *in, enum answer_visibility visibility,
                       struct answer *a) {
  int retVal = 0;

  do {
    int len = 0;
    DESERIALISE_BEGIN(out, len, max_len);

    DESERIALISE_STRING(a->uid);
    DESERIALISE_STRING(a->text);
    DESERIALISE_LONGLONG(a->value);
    DESERIALISE_LONGLONG(a->lat);
    DESERIALISE_LONGLONG(a->lon);
    DESERIALISE_LONGLONG(a->time_begin);
    DESERIALISE_LONGLONG(a->time_end);
    DESERIALISE_INT(a->time_zone_delta);
    DESERIALISE_INT(a->dst_delta);
    // #72 unit field
    DESERIALISE_STRING(a->unit);

    if (visibility > ANSWER_FIELDS_PUBLIC) {
      // #186 add "answer deleted" flag
      DESERIALISE_INT(a->flags);
      // #162 storage timestamp
      DESERIALISE_LONGLONG(a->stored);
    }

    // Check that we are at the end of the input string
    DESERIALISE_COMPLETE(out, len, max_len);
  } while (0);

  return retVal;
}

/*
  The following macros make it easier to compare fields between two instances of
  a structure.
 */
#define COMPARE_INT(S)                                                         \
  {                                                                            \
    if (q1->S > q2->S) {                                                       \
      LOG_MAYBE_ERRORV(mismatchIsError,                                        \
                       #S " fields do not match: '%d' vs '%d'", q1->S, q1->S); \
    } else if (q1->S < q2->S) {                                                \
      LOG_MAYBE_ERRORV(mismatchIsError, #S " fields do not match", "");        \
    } else                                                                     \
      retVal = 0;                                                              \
    if (retVal)                                                                \
      break;                                                                   \
  }
#define COMPARE_LONGLONG(S) COMPARE_INT(S)
#define COMPARE_STRING(S)                                                      \
  {                                                                            \
    if ((!q1->S) || (!q2->S)) {                                                \
      LOG_MAYBE_ERRORV(mismatchIsError,                                        \
                       #S " fields dot not match '%s' vs '%s'", q1->S, q2->S); \
    } else {                                                                   \
      if (strcmp(q1->S, q2->S)) {                                              \
        fprintf(stderr, #S " fields do not match\n");                          \
        LOG_MAYBE_ERRORV(mismatchIsError,                                      \
                         #S " fields do not match '%s' vs '%s'", q1->S,        \
                         q2->S);                                               \
      }                                                                        \
    }                                                                          \
  }

/*
  Using the above convenience macros, quickly compare all fields in a pair of
  question structures, so that the semantic equivalence of them can be tested.
  It returns 0 if the structures are equivalent, and -1 otherwise.
  If mistmatchIsError is non-zero, then the error logging facility will record
  each difference.  Otherwise, mismatches are considered not to be an error.
*/
int compare_questions(struct question *q1, struct question *q2,
                      int mismatchIsError) {
  int retVal = 0;
  do {
    COMPARE_STRING(uid);
    COMPARE_STRING(question_text);
    COMPARE_STRING(question_html);
    COMPARE_INT(type);
    COMPARE_INT(flags);
    COMPARE_STRING(default_value);
    COMPARE_LONGLONG(min_value);
    COMPARE_LONGLONG(max_value);
    COMPARE_INT(decimal_places);
    COMPARE_INT(num_choices);
    // #72 unit field
    COMPARE_STRING(unit);
  } while (0);
  return retVal;
}

/*
  Similar to compare_question(), but for comparing answers.
  (Note that because we are using the same convenience macros, the two
  structures must be called q1 and q2, although they are answers, not
  questions).
 */
int compare_answers(struct answer *q1, struct answer *q2, int mismatchIsError) {
  int retVal = 0;
  do {
    COMPARE_STRING(uid);
    COMPARE_STRING(text);
    COMPARE_LONGLONG(value);
    COMPARE_LONGLONG(lat);
    COMPARE_LONGLONG(lon);
    COMPARE_LONGLONG(time_begin);
    COMPARE_LONGLONG(time_end);
    COMPARE_INT(time_zone_delta);
    COMPARE_INT(dst_delta);
    // #72 unit field
    COMPARE_STRING(unit);
    // #186 flags
    COMPARE_INT(flags);
    // #162 storage timestamp
    COMPARE_LONGLONG(stored);
  } while (0);
  return retVal;
}
