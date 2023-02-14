/*
  Serialisers and de-serialisers for various structures.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "errorlog.h"
#include "question_types.h"
#include "survey.h"
#include "serialisers.h"

// #366, set retVal condition to < 0
#define REPORT_IF_FAILED()                                                     \
  {                                                                            \
    if (retVal < 0)                                                            \
      fprintf(stderr, "%s:%d: %s() failed.\n", __FILE__, __LINE__,             \
              __FUNCTION__);                                                   \
  }

/*
  Escape a string so that it can be safely embedded in a CSV file, that uses colons as delimeters
*/
int escape_string(char *in, char *out, int max_len) {
  int out_len = 0;

  // #421 allow NULL pointers
  if (in == NULL) {
    out[0] = 0;
    return 0;
  }

  for (int i = 0; in[i]; i++) {

    if ((out_len + 1) >= max_len) {
      LOG_WARNV("escaped version of string '%s' is too long", out);
      out[out_len] = 0;
      return 0;
    }

    switch (in[i]) {

      case '\r':
        out[out_len++] = '\\';
        out[out_len++] = 'r';
        break;
      case '\n':
        out[out_len++] = '\\';
        out[out_len++] = 'n';
        break;
      case '\t':
        out[out_len++] = '\\';
        out[out_len++] = 't';
        break;
      case '\b':
        out[out_len++] = '\\';
        out[out_len++] = 'b';
      break;
      case ':':
        out[out_len++] = '\\';
        out[out_len++] = ':';
      break;
      default:
        out[out_len++] = in[i];

    }

  }   // endfor

  out[out_len] = 0;
  return out_len;
}

int serialiser_count_columns(char separator, char *line) {
  int retVal = 0;
  int count = 1;

  do {
    if (!line) {
      BREAK_ERROR("line is NULL");
    }

    char prev = ' ';
    for(int i = 0; line[i] != '\0'; i++) {
      if(prev != '\\' && line[i] == '\n') {
        BREAK_ERROR("multi line string: not allowed. line break detected");
        break;
      }

      if(prev != '\\' && line[i] == separator) {
          count++;
      }
      prev = line[i];
    }
  } while (0);

  if (!retVal) {
    retVal = count;
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
      BREAK_ERRORV("integer converts to over-long string '%s...'", temp);
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
      BREAK_ERRORV("long long converts to over-long string '%s...'", temp);
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
      BREAK_ERRORV("Insufficient space to append string of %d chars", append_len);
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
        BREAK_ERRORV("Expected : before next field when deseralising at offset "
                   "%d of '%s'\n",
                   offset, in);
        break;
      } else {
        offset++;
      }
    }

    out[olen] = 0;
    if (!in)
      BREAK_ERROR("input string is NULL");
    if (!in[0])
      BREAK_ERROR("input string is empty");

    for (; in[offset] && (olen < 16383) && in[offset] != ':'; offset++) {
      // Allow some \ escape characters
      if (in[offset] == '\\') {
        if (!in[offset + 1])
          BREAK_ERRORV("String '%s' ends in \\\n", in);
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
            case 't':
              out[olen++] = '\t';
              break;
            case 'b':
              out[olen++] = '\b';
              break;
            default:
              BREAK_ERRORV("Illegal escape character 0x%02x at offset %d of '%s'\n",
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
      BREAK_ERROR("int field '%s' is NULL");
    if (!strlen(field))
      BREAK_ERROR("int field '%s' is empty string");

    int offset = 0;
    if (field[offset] == '-') {
      offset++;
    }

    for (int i = offset; field[i]; i++) {
      if (field[i] < '0' || field[i] > '9') {
        BREAK_ERRORV("int field '%s' contains non-digit", field);
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
      BREAK_ERROR("long long field '%s' is NULL");
    if (!strlen(field))
      BREAK_ERROR("long long field '%s' is empty string");

    int offset = 0;
    if (field[offset] == '-') {
      offset++;
    }

    for (int i = offset; field[i]; i++) {
      if (field[i] < '0' || field[i] > '9') {
        BREAK_ERRORV("long long field '%s' contains non-digit", field);
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
      BREAK_ERROR("string field is NULL");
    } else {
      *s = strdup(field);
    }

    if (!*s) {
      BREAK_ERROR("string field is empty string");
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

  All macros eventually will call BREAK_ERROR and break out of the parsing
  if any error occurs, and will record the specific error, making it much easier
  to debug when things go wrong.
*/
#define DESERIALISE_BEGIN(O, L, ML)                                            \
  {                                                                            \
    int offset = 0;                                                            \
    char field[16384];
#define DESERIALISE_COMPLETE(O, L, ML)                                         \
  if (offset < L) {                                                            \
    BREAK_ERROR("Junk at end of serialised object");                             \
  }                                                                            \
  }
#define DESERIALISE_NEXT_FIELD()                                               \
  if (deserialise_parse_field(in, &offset, field)) {                           \
    BREAK_ERRORV("failed to parse next field '%s' at offset %d", &in[offset],    \
               offset);                                                        \
  }
#define DESERIALISE_THING(S, DESERIALISER)                                     \
  DESERIALISE_NEXT_FIELD();                                                    \
  if (DESERIALISER(field, &S)) {                                               \
    BREAK_ERRORV("call to " #DESERIALISER " failed with string '%s'", field);    \
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
      BREAK_ERROR("serialised string too long");                               \
    }                                                                          \
    O[L++] = ':';                                                              \
    O[L] = 0;                                                                  \
  }

#define SERIALISE_BEGIN(O, L, ML)                                              \
  {                                                                            \
    int encoded_len = 0;                                                       \
    const int encoded_max_len = MAX_LINE;                                      \
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
    if (qt < 1) {
      BREAK_ERRORV("was asked to serialise an illegal question type #%d", qt);
    }
    if (qt > NUM_QUESTION_TYPES) {
      BREAK_ERRORV("was asked to serialise an illegal question type #%d", qt);
    }
    if (strlen(question_type_names[qt]) >= out_max_len) {
      BREAK_ERRORV("question type '%s' name too long", question_type_names[qt]);
    }

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

    for (qt = 1; qt <= NUM_QUESTION_TYPES; qt++) {
      if (!strcasecmp(field, question_type_names[qt])) {
        retVal = 0;
        *s = qt;
        break;
      }
    }

    if (qt > NUM_QUESTION_TYPES) {
      BREAK_ERRORV("invalid question type name '%s'", field);
    }
  } while (0);

  return retVal;
}

/**
 * appends fragment to a char* pointer, divided by a given separator
 * - this function allocates all required memory, for starting a list pass in an unallocated NULL pointer
 * - the callee is required to free the allocated memory himself
 * - the separator can be disabled by using the NULL byte, the fragment will then be appended without separator
 *
 * #482, #461
 */
char *serialise_list_append_alloc(char *src, char *in, const char separator) {
  int retVal = 0;

  do {
    if (!in) {
      LOG_WARNV("fragment to append is NULL, skipping", 0);
      break;
    }

    size_t slen = (src) ? strlen(src) : 0;
    size_t ilen = (in) ? strlen(in) : 0;

    size_t cap = slen + ilen;
    cap += 2; // separator + NUL (we are wasting one byte if !src)

    src = realloc(src, cap * sizeof(char));
    if(!src) {
        BREAK_ERROR("error reallocating memory for src char*");
    }

    // add separator if in is not first element
    if(slen && slen) {
        src[slen] = separator;
        if(separator) {
            slen++;
        }
    }

    size_t i = 0;
    while (i < ilen) {
        src[slen + i] = in[i];
        i++;
    }
    src[slen + i] = '\0';


  } while (0);

  if (retVal) {
      LOG_WARNV("error appending fragment '%s'", in);
  }

  return src;
}

/**
 * deserialises a string into a char** array by a given separator.
 * if *in is NULL then an empty list will be returned
 */
struct string_list *deserialise_string_list(char *in, const char separator) {
  int retVal = 0;
  struct string_list *list = NULL;

  do {
    list = calloc(1, sizeof(struct string_list));
    BREAK_IF(list == NULL, SS_ERROR_MEM, "string list");

    if(!in) {
      break; //return empty, allocated list
    }

    int count = serialiser_count_columns(',', in);
    if(!count) {
      break;
    }

    list->items = malloc(count * sizeof(char*));
    BREAK_IF(list->items == NULL, SS_ERROR_MEM, "string list items ptr");

    char *sav;
    char *line = parse_line(in, separator, &sav);

    while(line != NULL) {
      list->items[list->len] = line; // allocated already
      list->len++;
      // next
      line = parse_line(NULL, separator, &sav);
    }
  } while (0);

  if (retVal) {
    free_string_list(list);
    return NULL;
  }

  return list;
}

void free_string_list(struct string_list *list) {
  if (!list) {
    return;
  }

  for (size_t i = 0; i < list->len; i++) {
    free(list->items[i]);
  }

  list->len = 0;
  free(list);
}


/**
 * deserialises a string into a char** array by a given separator.
 */
char **deserialise_list_alloc(char *in, const char separator, size_t *len) {
  int retVal = 0;
  char **list = NULL;

  do {
    *(len) = 0; // set length to zero before handling input

    if(!in) {
      break;
    }

    int count = serialiser_count_columns(',', in);

    list = malloc(count * sizeof(char*));
    if (!list) {
      BREAK_ERROR("error allocating memory for char** list ptr");
    }
    char *sav;
    char *line = parse_line(in, separator, &sav);

    while(line != NULL) {
      list[*(len)] = line; // allocated already
      // next
      *(len)= *(len) + 1;
      line = parse_line(NULL, separator, &sav);
    }
  } while (0);

  if (retVal) {
      return NULL;
  }
  return list;
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
int dump_question(FILE *f, struct question *q) {
  int retVal = 0;
  do {
    fprintf(f,
      "{\n"
      "  uid: \"%s\"\n"
      "  question_text: \"%s\"\n"
      "  question_html: \"%s\"\n"
      "  question type: \"%s\"\n"
      "  flags: 0x%08X\n"
      "  default_value: \"%s\"\n"
      "  min_value: %lld\n"
      "  max_value: %lld\n"
      "  decimal_places: %d\n"
      "  num_choices: %d\n"
      "  unit: \"%s\"\n"
      "}\n",

      (q->uid) ? q->uid : "(null)",
      (q->question_text) ? q->question_text : "(null)",
      (q->question_html) ? q->question_html : "(null)",
      (q->type >= 1 && q->type <= NUM_QUESTION_TYPES) ? question_type_names[q->type] : "<unknown>",
      q->flags,
      (q->default_value) ? q->default_value : "(null)",
      q->min_value,
      q->max_value,
      q->decimal_places,
      q->num_choices,
      (q->unit) ? q->unit : "(null)"
    );
  } while (0);

  return retVal;
}

/*
  For debugging it can be helpful to dump an answer structure to
  stdout or a file.
*/
int dump_answer(FILE *f, struct answer *a) {
  int retVal = 0;
  do {
    fprintf(f,
      "{\n"
      "  uid: \"%s\"\n"
      "  type: \"%s\"\n"
      "  text: \"%s\"\n"
      "  value: %lld\n"
      "  lat: %lld\n"
      "  lon: %lld\n"
      "  time_begin: %lld\n"
      "  time_end: %lld\n"
      "  time_zone_delta: %d\n"
      "  dst_delta: %d\n"
      "  unit: \"%s\"\n"
      "  flags: %d\n"
      "  stored: %lld\n"
      "}\n",

      (a->uid) ? a->uid : "(null)",
      (a->type >= 1 && a->type <= NUM_QUESTION_TYPES) ? question_type_names[a->type] : "<unknown>",
      (a->text) ? a->text : "(null)",
      a->value,
      a->lat,
      a->lon,
      a->time_begin,
      a->time_end,
      a->time_zone_delta,
      a->dst_delta,
      (a->unit) ? a->unit : "(null)",
      a->flags,
      a->stored
    );
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

/**
* Top-level function for serialising a answer that has been passed in
* in a struct answer.  It uses the various macros defined above to
* make a very clear and succinct description of what is required.
*
* #72, add unit field
* #162, add storage timestamp
* #186, add "answer deleted" flag
* #274, add visibility scope
* #358, add question type
* #413, add pre-validation (column count)
* #448 remove 'unit' from public answer
 */
int serialise_answer(struct answer *a, enum answer_scope scope, char *out, int max_len) {
  int retVal = 0;
  do {

    int len = 0;

    switch (scope) {
      case ANSWER_SCOPE_PUBLIC:
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
        SERIALISE_COMPLETE(out, len, max_len);
        break;

      case ANSWER_SCOPE_CHECKSUM:
        SERIALISE_BEGIN(out, len, max_len);
        SERIALISE_STRING(a->uid);
        SERIALISE_THING(a->type, serialise_question_type);
        SERIALISE_STRING(a->text);
        SERIALISE_LONGLONG(a->value);
        SERIALISE_LONGLONG(a->lat);
        SERIALISE_LONGLONG(a->lon);
        SERIALISE_LONGLONG(a->time_begin);
        SERIALISE_LONGLONG(a->time_end);
        SERIALISE_INT(a->time_zone_delta);
        SERIALISE_INT(a->dst_delta);
        SERIALISE_STRING(a->unit);
        SERIALISE_INT(a->flags);
        SERIALISE_COMPLETE(out, len, max_len);
      break;

      default:
        SERIALISE_BEGIN(out, len, max_len);
        SERIALISE_STRING(a->uid);
        SERIALISE_THING(a->type, serialise_question_type);
        SERIALISE_STRING(a->text);
        SERIALISE_LONGLONG(a->value);
        SERIALISE_LONGLONG(a->lat);
        SERIALISE_LONGLONG(a->lon);
        SERIALISE_LONGLONG(a->time_begin);
        SERIALISE_LONGLONG(a->time_end);
        SERIALISE_INT(a->time_zone_delta);
        SERIALISE_INT(a->dst_delta);
        SERIALISE_STRING(a->unit);
        SERIALISE_INT(a->flags);
        SERIALISE_LONGLONG(a->stored);
        SERIALISE_COMPLETE(out, len, max_len);
    }

  } while (0);

  return retVal;
}

/**
* The top-level function for converting a CSV string
* representation of an answer structure back into a live
* structure.  As with {de,}serialise_question(),
* this function and serialise_answer() must be matched
* in the order and list of fields that they process.
*
* #72, add unit field
* #162, add storage timestamp
* #186, add "answer deleted" flag
* #274, add visibility scope
* #358, add question type
* #413, add pre-validation (column count)
* #448 remove 'unit' from public answer
*/
int deserialise_answer(char *in, enum answer_scope scope, struct answer *a) {
  int retVal = 0;

  do {
    int len = 0;

    if (!in) {
      BREAK_ERROR("answer string is null");
    }

    int cols = serialiser_count_columns(':', in);
    if (cols < 0) {
      BREAK_ERROR("invalid answer line");
    }
    if (cols != scope) {
      BREAK_ERRORV("invalid column count in answer line: %d != %d", cols, scope);
    }

    switch (scope) {
      case ANSWER_SCOPE_PUBLIC:
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
        DESERIALISE_COMPLETE(out, len, max_len);
        break;

      case ANSWER_SCOPE_CHECKSUM:
        DESERIALISE_BEGIN(out, len, max_len);
        DESERIALISE_STRING(a->uid);
        DESERIALISE_THING(a->type, deserialise_question_type);
        DESERIALISE_STRING(a->text);
        DESERIALISE_LONGLONG(a->value);
        DESERIALISE_LONGLONG(a->lat);
        DESERIALISE_LONGLONG(a->lon);
        DESERIALISE_LONGLONG(a->time_begin);
        DESERIALISE_LONGLONG(a->time_end);
        DESERIALISE_INT(a->time_zone_delta);
        DESERIALISE_INT(a->dst_delta);
        DESERIALISE_STRING(a->unit);
        DESERIALISE_INT(a->flags);
        DESERIALISE_COMPLETE(out, len, max_len);
        break;

      default:
        DESERIALISE_BEGIN(out, len, max_len);
        DESERIALISE_STRING(a->uid);
        DESERIALISE_THING(a->type, deserialise_question_type);
        DESERIALISE_STRING(a->text);
        DESERIALISE_LONGLONG(a->value);
        DESERIALISE_LONGLONG(a->lat);
        DESERIALISE_LONGLONG(a->lon);
        DESERIALISE_LONGLONG(a->time_begin);
        DESERIALISE_LONGLONG(a->time_end);
        DESERIALISE_INT(a->time_zone_delta);
        DESERIALISE_INT(a->dst_delta);
        DESERIALISE_STRING(a->unit);
        DESERIALISE_INT(a->flags);
        DESERIALISE_LONGLONG(a->stored);
        DESERIALISE_COMPLETE(out, len, max_len);
    }

  } while (0);

  return retVal;
}

/*
  The following macros make it easier to compare fields between two instances of
  a structure.
 */
#define COMPARE_INT(S)                                                             \
  {                                                                                \
    if (q1->S > q2->S) {                                                           \
      if (mismatchIsError) {                                                       \
        BREAK_ERRORV(#S " fields do not match: '%d' vs '%d'", q1->S, q1->S);       \
      }                                                                            \
    } else if (q1->S < q2->S) {                                                    \
      if (mismatchIsError) {                                                       \
        BREAK_ERROR(#S " fields do not match");                                    \
      }                                                                            \
    } else                                                                         \
      retVal = 0;                                                                  \
    if (retVal)                                                                    \
      break;                                                                       \
  }
#define COMPARE_LONGLONG(S) COMPARE_INT(S)
#define COMPARE_STRING(S)                                                          \
  {                                                                                \
    if ((!q1->S) || (!q2->S)) {                                                    \
      if (q1->S != q2->S && mismatchIsError) {                                     \
        BREAK_ERRORV(#S " fields dot not match '%s' vs '%s'", q1->S, q2->S);       \
      }                                                                            \
    } else {                                                                       \
      if (strcmp(q1->S, q2->S)) {                                                  \
        if (mismatchIsError) {                                                     \
          BREAK_ERRORV( #S " fields do not match '%s' vs '%s'", q1->S, q2->S);     \
        }                                                                          \
      }                                                                            \
    }                                                                              \
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


int dump_answer_list(FILE *fp, struct answer_list *list) {
  int retVal = 0;

  do {
    if (!fp) {
      BREAK_ERROR("dump_next_questions(): invalid file pointer.");
    }

    if (!list) {
      fprintf(fp, "answers { <NULL> }\n");
      break;
    }

    fprintf(
      fp,
      "answers {\n"
      "  len: %zu\n"
      "  answers: [\n",
      list->len
    );

    for (size_t i = 0; i < list->len; i++) {
      fprintf(fp, "    %s%s\n", list->answers[i]->uid, (i < list->len - 1) ? ",": "");
    }

    fprintf(fp , "  ]\n}\n");
  } while (0);

  return retVal;
}

void free_answer_list(struct answer_list *list) {
  if (!list) {
    return;
  }
  for(size_t i = 0; i < list->len; i++) {
    free_answer(list->answers[i]);
  }
  list->len = 0;

  free(list);
}

/**
 * deserialise a sequence of answers
 */
struct answer_list *deserialise_answers(const char *body, enum answer_scope scope) {
  int retVal = 0;

  struct answer_list *list = NULL;
  do {
    if (!body) {
      BREAK_ERROR("body to parse is null");
    }
    if (!strlen(body)) {
      BREAK_ERROR("body to parse is empty");
    }

    list = calloc(1, sizeof(struct answer_list));
    if (!list) {
      BREAK_ERROR("error allocating memory for answer list");
    }

    int i = 0;
    char *sav;
    char *line = parse_line(body, '\n', &sav);

    while(line != NULL) {

      // initialise answer
      list->len++; // placed here for comlete free_answer_list on retVal > 0
      list->answers[i] = calloc(1, sizeof(struct answer));
      if (list->answers[i] == NULL) {
        BREAK_ERRORV("error allocating memory for answer in line %d", i);
        break;
      }

      // deserialise answer
      if (deserialise_answer(line, scope, list->answers[i])) {
        BREAK_ERRORV("failed to deserialise answer for line %d, starting with '%.20s'", i, line);
        break;
      }
      freez(line);
      line = NULL;

      LOG_INFOV("parsed answer '%s' from line %d", list->answers[i]->uid, i);

      // next
      i++;
      line = parse_line(NULL, '\n', &sav);

    } // while(line)

  } while(0);

  if (retVal) {
    free_answer_list(list);
    return NULL;
  }

  return list;
}
