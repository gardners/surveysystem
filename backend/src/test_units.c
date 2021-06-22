#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "errorlog.h"
#include "serialisers.h"
#include "survey.h"
#include "sha1.h"
#include "question_types.h"

struct answer* create_answer(char *uid, int type, char *text, char *unit) {
  struct answer *ans = calloc(sizeof(struct answer), 1);
  assert(ans != NULL); // exit

  ans->uid = strdup(uid);
  ans->type = type;

  if (text) {
    ans->text = strdup(text);
  }
  if (unit) {
    ans->unit = strdup(unit);
  }

  return ans;
}
#define MAX_TEST_BUFFER 2048

enum {
  FORMAT_FAIL, FORMAT_PASS, FORMAT_SKIP, FORMAT_SECTION, FORMAT_NONE
} format;

void test_message(int format, const char *message, ...) {
  char msg[MAX_TEST_BUFFER];
  va_list argp;
  va_start(argp, message);
  vsnprintf(msg, MAX_TEST_BUFFER, message, argp);
  va_end(argp);

  switch (format) {
    case FORMAT_FAIL:
      fprintf(stderr, "\e[31m[FAIL]\e[0m %s\n", msg);
      break;
    case FORMAT_PASS:
      fprintf(stderr, "\e[32m[PASS]\e[0m %s\n", msg);
      break;
    case FORMAT_SKIP:
      fprintf(stderr, "\e[37m[SKIP]\e[0m %s\n", msg);
      break;
    case FORMAT_SECTION:
      fprintf(stderr, "\n\e[33m --- %s --- \e[0m\n\n", msg);
      break;
    default:
      fprintf(stderr, "%s\n", msg);
  }
}

#define ASSERT(expr, message, ...)\
do {\
  if (expr) {\
    test_message(FORMAT_PASS, message, __VA_ARGS__);\
  } else {\
    test_message(FORMAT_FAIL, message, __VA_ARGS__);\
    fprintf(stderr, "\e[37m"); dump_errors(stdout); fprintf(stderr, "\e[0m"); \
  }\
  assert(expr);\
  clear_errors(); \
} while (0)

#define ASSERT_STR_EQ(left, right, message)\
do {\
   ASSERT(strcmp(left, right) == 0, "'%s' == '%s' %s", left, right, message); \
} while (0)

#define PASS(message, ...)\
do {\
  test_message(FORMAT_PASS, message, __VA_ARGS__);\
} while (0)

#define SKIP(message, ...)\
do {\
  test_message(FORMAT_SKIP, message, __VA_ARGS__);\
} while (0)

#define FAIL(message, ...)\
do {\
  test_message(FORMAT_FAIL, message, __VA_ARGS__);\
} while (0)

#define DEBUG(message, ...)\
do {\
  test_message(FORMAT_NONE, message, __VA_ARGS__);\
} while (0)

#define SECTION(message)\
do {\
  test_message(FORMAT_SECTION, message);\
} while (0)

void assert_answers_compare(struct answer *in, struct answer *out) {
  if (compare_answers(in, out, MISMATCH_IS_AN_ERROR)) {
    FAIL("'%s' != '%s'!", in->uid, out->uid);
    DEBUG(">> in:", "");
    dump_answer(stderr, in);
    DEBUG("<< out:", "");
    dump_answer(stderr, out);

    free_answer(in);
    free_answer(out);
    exit(1);
  }

  PASS("answers match: '%s' == '%s'\n", in->uid, out->uid);
  free_answer(in);
  free_answer(out);
}

struct question_serialiser_test {
  char *name;
  int shouldPassP;
  #define DIRECTION_SERIALISE 1
  #define DIRECTION_DESERIALISE 2
  int direction;
  char *serialised;
  struct question question;
};

#define SHOULD_FAIL 0
#define SHOULD_PASS 1

struct question_serialiser_test qst[] = {    // An empty string should not be accepted
  {
    "Empty string not accepted",
    SHOULD_FAIL,
    DIRECTION_DESERIALISE,
    "",
    { NULL }
  },
     // A simple valid record should be accepted.
  {
    "Simple serialised question",
    SHOULD_PASS,
    DIRECTION_SERIALISE | DIRECTION_DESERIALISE,
    "dummyuid:"
    "What is the answer to life, the universe and everything?:"
    "<div>What is the answer to life, the universe and everything?</div>:"
    "INT:0:42:0:100:0:1::",
    {
      "dummyuid", "What is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, 1, "", "" }
  },
     // Only valid question types should be accepted
  {
    "Illegal question type fails",
    SHOULD_FAIL,
    DIRECTION_DESERIALISE,
    "dummyuid:"
    "What is the answer to life, the universe and everything?:"
    "<div>What is the answer to life, the universe and everything?</div>:"
    "FISH:0:42:0:100:0:1::",
    {
      "dummyuid", "What is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, 1, "", "" }
  },

  {
    "Negative numbers are accepted",
    SHOULD_PASS,
    DIRECTION_DESERIALISE | DIRECTION_SERIALISE,
    "dummyuid:"
    "What is the answer to life, the universe and everything?:"
    "<div>What is the answer to life, the universe and everything?</div>:"
    "INT:0:42:0:-100:0:-1::",
    {
      "dummyuid", "What is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, -100, 0, -1, "", "" }
  },

  {
    "Minus sign can appear only at beginning of a number",
    SHOULD_FAIL,
    DIRECTION_DESERIALISE,
    "dummyuid:"
    "What is the answer to life, the universe and everything?:"
    "<div>What is the answer to life, the universe and everything?</div>:"
    "INT:0:42:0:10-0:0:-1::",
    {
      "dummyuid", "What is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, -1, "", "" }
  },

  {
    "Missing fields results in failure",
    SHOULD_FAIL,
    DIRECTION_DESERIALISE,
    "dummyuid:"
    "What is the answer to life, the universe and everything?:"
    "<div>What is the answer to life, the universe and everything?</div>:"
    "INT:0:42:0:10-0:0::",
    {
      "dummyuid", "What is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, -1, "", "" }
  },

  {
    "Missing fields results in failure",
    SHOULD_FAIL,
    DIRECTION_DESERIALISE,
    "dummyuid:"
    "What is the answer to life, the universe and everything?:"
    "<div>What is the answer to life, the universe and everything?</div>:"
    "INT:0:42:0:10-0:0::",
    {
      "dummyuid", "What is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, -1, "", "" }
  },

  {
    "Extra fields results in failure",
    SHOULD_FAIL,
    DIRECTION_DESERIALISE,
    "dummyuid:"
    "What is the answer to life, the universe and everything?:"
    "<div>What is the answer to life, the universe and everything?</div>:"
    "INT:0:42:0:10-0:0:-1:::-1",
    {
      "dummyuid", "What is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, -1, "", "" }
  },

  {
    "\\t escape is accepted in strings",
    SHOULD_PASS,
    DIRECTION_DESERIALISE | DIRECTION_SERIALISE,
    "dummyuid:"
    "\tWhat is the answer to life, the universe and everything?:"
    "<div>What is the answer to life, the universe and everything?</div>:"
    "INT:0:42:0:100:0:-1::",
    {
      "dummyuid", "\tWhat is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, -1, "", "" }
  },

  {
    "\\r escape is accepted in strings",
    SHOULD_PASS,
    DIRECTION_DESERIALISE | DIRECTION_SERIALISE,
    "dummyuid:"
    "\rWhat is the answer to life, the universe and everything?:"
    "<div>What is the answer to life, the universe and everything?</div>:"
    "INT:0:42:0:100:0:-1::",
    {
      "dummyuid", "\rWhat is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, -1, "", "" }
  },

  {
    "\\n escape is accepted in strings",
    SHOULD_PASS,
    DIRECTION_DESERIALISE | DIRECTION_SERIALISE,
    "dummyuid:"
    "\nWhat is the answer to life, the universe and everything?:"
    "<div>What is the answer to life, the universe and everything?</div>:"
    "INT:0:42:0:100:0:-1::",
    {
      "dummyuid", "\nWhat is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, -1, "", "" }
  },

  {
    "\\: escape is accepted in strings",
    SHOULD_PASS,
    DIRECTION_DESERIALISE | DIRECTION_SERIALISE,
    "dummyuid:"
    "\\:What is the answer to life, the universe and everything?:"
    "<div>What is the answer to life, the universe and everything?</div>:"
    "INT:0:42:0:100:0:-1::",
    {
      "dummyuid", ":What is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, -1, "", "" }
  },

  {
    "\\\\ escape is accepted in strings",
    SHOULD_PASS,
    DIRECTION_DESERIALISE | DIRECTION_SERIALISE,
    "dummyuid:"
    "\\\\What is the answer to life, the universe and everything?:"
    "<div>What is the answer to life, the universe and everything?</div>:"
    "INT:0:42:0:100:0:-1::",
    {
      "dummyuid", "\\What is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, -1, "", "" }
  },

  {
    "Multiple escape is accepted in strings",
    SHOULD_PASS,
    DIRECTION_DESERIALISE | DIRECTION_SERIALISE,
    "dummyuid:"
    "\\\\\r\n\t\\:What is the answer to life, the universe and everything?:"
    "<div>What is the answer to life, the universe and everything?</div>:"
    "INT:0:42:0:100:0:-1::",
    {
      "dummyuid",
      "\\\r\n\t:What is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, -1, "", "" }
  },

  {
    "numchoices should match number of colon separated items in choices field",
    SHOULD_PASS,
    DIRECTION_DESERIALISE | DIRECTION_SERIALISE,
    "dummyuid:"
    "\\\\\r\n\t\\:What is the answer to life, the universe and everything?:"
    "<div>What is the answer to life, the universe and everything?</div>:"
    "MULTICHOICE:0:42:0:100:0:1:this,that:",
    {
      "dummyuid",
      "\\\r\n\t:What is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_MULTICHOICE, 0, "42", 0, 100, 0, 1, "this,that", "" }
  },

  {
    NULL, -1, -1, NULL, { NULL }
  }
};

int main(int argc, char **argv) {
  int retVal = 0;

  do {

    setenv("SURVEY_HOME", ".", 0);
    setenv("SS_LOG_FILE", "./logs/test_units.log", 0);

    SECTION("question serialisation tests");

    int fail = 0;
    int pass = 0;
    int errors = 0;

    for (int i = 0; qst[i].name; i++) {

      clear_errors();

      if (qst[i].direction &DIRECTION_SERIALISE) {
          // XXX Implement
      }
      if (qst[i].direction &DIRECTION_DESERIALISE) {
        struct question d;
        bzero(&d, sizeof(struct question));
        int deserialise_result = deserialise_question(qst[i].serialised, &d);

        if (deserialise_result && qst[i].shouldPassP) {
          // Deserialisation failed when it should have succeeded.
          FAIL("'%s': serialised string triggered an error during deserialisation", qst[i].name);
          DEBUG("Internal error log:", "");
          dump_errors(stderr);
          fail++;
        } else if ((!deserialise_result) && (!qst[i].shouldPassP)) {
          // Deserialiation passed when it should have failed.
          FAIL("'%s': invalid serialised string did not trigger an error during deserialisation", qst[i].name);
          DEBUG("Internal error log:", "");
          dump_errors(stderr);
          fail++;
        } else if ((!deserialise_result) && qst[i].shouldPassP) {
          // Deserialised successfully, so make sure the field values
          // all match
          if (compare_questions(&d, &qst[i].question, MISMATCH_IS_AN_ERROR)) {
            FAIL("'%s': original and serialised-then-deserialised question structures differ", qst[i].name);
            DEBUG("Deserialised result", "");
            dump_question(stderr, &d);
            DEBUG("Expected result", "");
            dump_question(stderr, &qst[i].question);
            DEBUG("Internal error log:", "");
            dump_errors(stderr);
            fail++;
          } else {
            PASS("'%s'", qst[i].name);
            pass++;
          }
        } else if ((deserialise_result) && (!qst[i].shouldPassP)) {
          PASS("'%s'", qst[i].name);
          pass++;
        } else {
          FAIL("'%s' deserialisation failed unexpectedly: deserialise_result=%d, shouldPass=%d\n", qst[i].name, deserialise_result, qst[i].shouldPassP);
          errors++;
        }
      }
    }

    DEBUG("\nSummary: %d tests passed, %d failed and %d encountered internal errors.\n", pass, fail, errors);

    /*tests for #392 */
    SECTION("escape_string() tests, issue #392");

    {
      char str[1024];

      escape_string("hello", str, 1024);
      ASSERT_STR_EQ(str, "hello", "escape_string()");

      escape_string("::1(9000)", str, 1024);
      ASSERT_STR_EQ(str, "\\:\\:1(9000)", "escape_string()");

      escape_string("\\r\\n\\t:(9000)", str, 1024);
      ASSERT_STR_EQ(str, "\\r\\n\\t\\:(9000)", "escape_string()");

      escape_string("\t\r\n:(9000)", str, 1024);
      ASSERT_STR_EQ(str, "\\t\\r\\n\\:(9000)", "escape_string()");
    }

    /*
     * tests for #451
     * includes testing boundary types ("start of list", "end of list"):
     * see question_types.h  question_type_names[1+NUM_QUESTION_TYPES+1];
     */

    SECTION("deserialise_question_type() tests, issue #451");

    {
      int index = -99;
      int invalid = 0;
      char *name = question_type_names[invalid];

      int ret = deserialise_question_type(name, &index);
      ASSERT(ret == -1, "(type '%s', question_type_names[%d]) returns -1 (out of bounds)", name, invalid);
      ASSERT(index == -99, "(type '%s') index written (%d > 0)", name, index);
    }

    {
      int index = -99;
      int first = 1;
      char *name = question_type_names[first];

      int ret = deserialise_question_type(name, &index);
      ASSERT(ret == 0, "first type ('%s'): retval == 0", name);
      ASSERT(index == first, "first type ('%s'): index written (%d == %d)", name, index, first);
      ASSERT_STR_EQ(question_type_names[index], name, "");
    }

    {
      int index = -99;
      int last = NUM_QUESTION_TYPES;
      char *name = question_type_names[last];

      int ret = deserialise_question_type(name, &index);
      ASSERT(ret == 0, "last type ('%s'): retval == 0", name);
      ASSERT(index == last, "flast type ('%s'): index written (%d == %d)", name, index, last);
      ASSERT_STR_EQ(question_type_names[index], name, "");
    }

    {
      int index = -99;
      int invalid = NUM_QUESTION_TYPES + 1;
      char *name = question_type_names[invalid];

      int ret = deserialise_question_type(name, &index);
      ASSERT(ret == -1, "(type '%s', question_type_names[%d]) returns -1 (out of bounds)", name, invalid);
      ASSERT(index == -99, "(type '%s') index written (%d > 0)", name, index);
    }

    {
      int index = -99;
      char *name = "UNKNOWN";

      int ret = deserialise_question_type(name, &index);
      ASSERT(ret == -1, "(type '%s') returns -1", name);
      ASSERT(index == -99, "(type '%s') index written (%d > 0)", name, index);
    }

    /*
     * tests for #451
     * includes testing boundary types ("start of list", "end of list"):
     * see question_types.h  question_type_names[1+NUM_QUESTION_TYPES+1];
     */

    SECTION("serialise_question_type() tests, issue #451");

    {
      char out [256];

      {
        out[0] = 0;
        int invalid = 0;
        int ret = serialise_question_type(invalid, out, 256);

        ASSERT(ret == -1, "(type '%s', question_type_names[%d]) returns -1 (out of bounds)", question_type_names[invalid], invalid);
        ASSERT(out[0] == 0, "(type '%s') nothing written into 'out'", question_type_names[invalid]);
      }

      {
        int first = 1;
        int ret = serialise_question_type(first, out, 256);

        ASSERT(ret > 0, " (first type: %d) returns > 0", first);
        ASSERT_STR_EQ(out, question_type_names[first], "");
        ASSERT(ret == strlen(out), "(first type: %d) returns strlen(name)", first);
      }

      {
        int last = NUM_QUESTION_TYPES;
        int ret = serialise_question_type(last, out, 256);

        ASSERT(ret > 0, " (last type: %d) returns > 0", last);
        ASSERT_STR_EQ(out, question_type_names[last], "");
        ASSERT(ret == strlen(out), "(last type: %d) returns strlen(name)", last);
      }

      {
        out[0] = 0;
        int invalid = NUM_QUESTION_TYPES + 1;
        int ret = serialise_question_type(invalid, out, 256);

        ASSERT(ret == -1, "(type '%s', question_type_names[%d]) returns -1 (out of bounds)", question_type_names[invalid], invalid);
        ASSERT(out[0] == 0, "(type '%s') nothing written into 'out'", question_type_names[invalid]);
      }

    }

    SECTION("serialiser_count_columns(':', ), #413");

    {
      LOG_UNMUTE();
      int ret;

      ret = serialiser_count_columns(':', "", 1024);
      ASSERT(ret == 1, "serialiser_count_columns(':', '%s')", "");

      ret = serialiser_count_columns(':', NULL, 1024);
      ASSERT(ret == -1, "error: serialiser_count_columns(':', NULL)", "");

      ret = serialiser_count_columns(':', "hello", 1024);
      ASSERT(ret == 1, "serialiser_count_columns(':', '%s')", "hello");

      ret = serialiser_count_columns(':', "hello", 2);
      ASSERT(ret == -1, "error: serialiser_count_columns(':', '%s') out of bounds", "hello");

      ret = serialiser_count_columns(':', ":", 1024);
      ASSERT(ret == 2, "serialiser_count_columns(':', '%s')", ":");

      ret = serialiser_count_columns(':', "\\:", 1024);
      ASSERT(ret == 1, "escape: serialiser_count_columns(':', '%s')", "\\:");

      ret = serialiser_count_columns(':', "\\\\:", 1024);
      ASSERT(ret == 1, "double escape: serialiser_count_columns(':', '%s')", "\\\\:");

      ret = serialiser_count_columns(':', ":\\:", 1024);
      ASSERT(ret == 2, "escape inner: serialiser_count_columns(':', '%s')", ":\\:");

      ret = serialiser_count_columns(':', "::1", 1024);
      ASSERT(ret == 3, "serialiser_count_columns(':', '%s')", "::1");

      ret = serialiser_count_columns(':', "::", 1024);
      ASSERT(ret == 3, "serialiser_count_columns(':', '%s')", ":::");

      ret = serialiser_count_columns(':', "1:1:1", 1024);
      ASSERT(ret == 3, "serialiser_count_columns(':', '%s')", "1:1:1");

      ret = serialiser_count_columns(':', "1:1:1", 2);
      ASSERT(ret == -1, "error serialiser_count_columns(':', '%s') out of bounds", "1:1:1");

      ret = serialiser_count_columns(':', "\n:1:1:1", 1024);
      ASSERT(ret == -1, "error starting endline serialiser_count_columns(':', '%s') %d", "\\n:1:1:1", ret);

      ret = serialiser_count_columns(':', ":1:1:1\n", 1024);
      ASSERT(ret == -1, "error ending endline serialiser_count_columns(':', '%s') %d", ":1:1:1\\n", ret);

      ret = serialiser_count_columns(':', "1:1:1\n1:1:1", 1024);
      ASSERT(ret == -1, "error inner endline serialiser_count_columns(':', '%s') %d", "1:1:1\\n1:1:1", ret);

      ret = serialiser_count_columns(':', "1:1:1\\n1:1:1", 1024);
      ASSERT(ret == 5, "escaped inner endline serialiser_count_columns(':', '%s') %d", "1:1:1\\\\n1:1:1", ret);

      ret = serialiser_count_columns(':', "uid:META:\\:\\n\r\b\tutext:0:0:0:0:0:0:0:unit:0:0", 1024);
      ASSERT(ret == ANSWER_SCOPE_FULL, "answer ANSWER_FIELDS_PROTECTED with escapes passing serialiser_count_columns(':', '%s') %d", "uid:META:\\:\\n\\r\\b\\tutext:0:0:0:0:0:0:0:unit:0:0", ret);

      // #448 remove 'unit' from public answer
      ret = serialiser_count_columns(':', "uid:\\:\\n\r\b\tutext:0:0:0:0:0:0:0", 1024);
      ASSERT(ret == ANSWER_SCOPE_PUBLIC, "answer ANSWER_FIELDS_PUBLIC with escapes passing serialiser_count_columns(':', '%s') %d", "uid:META:\\:\\n\\r\\b\\tutext:0:0:0:0:0:0:0:unit:0:0", ret);
    }

    ////
    // answer serialisation - a large number of question type => value related tests are also located in test_runner tests/*
    ////

    SECTION("answer deserialisation width tests, #413");

    LOG_MUTE();

    {
      int ret;
      char str[] = "correct-scope:TEXT:Answer 1:0:0:0:0:0:0:0::0:123";

      struct answer *out = calloc(sizeof(struct answer), 1);
      ret = deserialise_answer(str, ANSWER_SCOPE_FULL, out);
      ASSERT(ret == 0, "answer '%s' deserialised %d", "correct-scope", ret);
      free_answer(out);
    }

    {
      int ret;
      char str[] = "incorrect-scope-public:TEXT:Answer 1:0:0:0:0:0:0:0::0:123";

      struct answer *out = calloc(sizeof(struct answer), 1);
      ret = deserialise_answer(str, ANSWER_SCOPE_PUBLIC, out); // should be ANSWER_SCOPE_FULL
      ASSERT(ret == -1, "answer '%s' deserialised %d", "incorrect-scope-public", ret);
      free_answer(out);
    }

    {
      int ret;
      char str[] = "incorrect-scope-checksum:TEXT:Answer 1:0:0:0:0:0:0:0::0:123";

      struct answer *out = calloc(sizeof(struct answer), 1);
      ret = deserialise_answer(str, ANSWER_SCOPE_CHECKSUM, out); // should be ANSWER_SCOPE_FULL
      ASSERT(ret == -1, "answer '%s' deserialised %d", "incorrect-scope-checksum", ret);
      free_answer(out);
    }

    {
      int ret;
      char str[] = "too-short:TEXT";

      struct answer *out = calloc(sizeof(struct answer), 1);
      ret = deserialise_answer(str, ANSWER_SCOPE_CHECKSUM, out);
      ASSERT(ret == -1, "answer '%s' deserialised %d", "too-short", ret);
      free_answer(out);
    }

    {
      int ret;
      char str[] = "";

      struct answer *out = calloc(sizeof(struct answer), 1);
      ret = deserialise_answer(str, ANSWER_SCOPE_CHECKSUM, out);
      ASSERT(ret == -1, "answer '%s' deserialised %d", "empty string", ret);
      free_answer(out);
    }

    {
      int ret;
      struct answer *out = calloc(sizeof(struct answer), 1);
      ret = deserialise_answer(NULL, ANSWER_SCOPE_CHECKSUM, out);dump_errors(stderr);
      ASSERT(ret == -1, "answer '%s' deserialised %d", "*in == NULL", ret);
      free_answer(out);
    }

    {
      int ret;
      char str[] = "too-long:TEXT:Answer 1:0:0:0:0:0:0:0::0:123:"; // one separator added at end
      struct answer *out = calloc(sizeof(struct answer), 1);
      ret = deserialise_answer(str, ANSWER_SCOPE_CHECKSUM, out);dump_errors(stderr);
      ASSERT(ret == -1, "answer '%s' deserialised %d", "too-long", ret);
      free_answer(out);
    }

    LOG_UNMUTE();

    ////
    // answer serialisation - a large number of question type => value related tests are also located in test_runner tests/*
    ////

    SECTION("answer serialisation width tests, #413, #268");

    LOG_MUTE();

    {
      char str[1024];
      int ret;

      struct answer *in = create_answer("serialise-full", QTYPE_TEXT, "something", "unit");
      in->stored = 123;
      ret = serialise_answer(in, ANSWER_SCOPE_FULL, str, 1024);
      ASSERT(ret == 0, "answer '%s' serialised %d", in->uid, ret);
      ASSERT_STR_EQ(str, "serialise-full:TEXT:something:0:0:0:0:0:0:0:unit:0:123", "");
    }

    {
      char str[1024];
      int ret;

      struct answer *in = create_answer("serialise-checksum", QTYPE_TEXT, "something", "unit");
      in->stored = 123;
      ret = serialise_answer(in, ANSWER_SCOPE_CHECKSUM, str, 1024);
      ASSERT(ret == 0, "answer '%s' serialised %d", in->uid, ret);
      ASSERT_STR_EQ(str, "serialise-checksum:TEXT:something:0:0:0:0:0:0:0:unit:0", "");
    }

    {
      char str[1024];
      int ret;
      // #448 remove 'unit' from public answer
      struct answer *in = create_answer("serialise-public", QTYPE_TEXT, "something", "unit");
      in->stored = 123;
      ret = serialise_answer(in, ANSWER_SCOPE_PUBLIC, str, 1024);
      ASSERT(ret == 0, "answer '%s' serialised %d", in->uid, ret);
      ASSERT_STR_EQ(str, "serialise-public:something:0:0:0:0:0:0:0", "");
    }

    LOG_UNMUTE();

    SECTION("answer serialisation tests, #392");

    {
      char str[1024];
      char uid[] = "uid";
      int ret;

      struct answer *in = create_answer(uid, QTYPE_META, ":\n\r\b\tutext", "unit");
      ret = serialise_answer(in, ANSWER_SCOPE_FULL,str, 1024);
      ASSERT(ret == 0, "answer '%s' serialised", uid);

      struct answer *out = calloc(sizeof(struct answer), 1);
      ret = deserialise_answer(str, ANSWER_SCOPE_FULL, out);
      ASSERT(ret == 0, "answer '%s' deserialised", uid);

      assert_answers_compare(in, out);  // answers are freed in func
    }

    {
      char str[1024];
      int ret;
      char uid[] = "text-is-json-array";

      struct answer *in = create_answer(uid, QTYPE_TEXT, "[42, -42]", "unit");
      ret = serialise_answer(in, ANSWER_SCOPE_FULL, str, 1024);
      ASSERT(ret == 0, "answer '%s' serialised", uid);

      struct answer *out = calloc(sizeof(struct answer), 1);
      ret = deserialise_answer(str, ANSWER_SCOPE_FULL, out);
      ASSERT(ret == 0, "answer '%s' deserialised", uid);

      assert_answers_compare(in, out);  // answers are freed in func
    }

    {
      char str[1024];
      int ret;
      char uid[] = "text-is-json-object";

      struct answer *in = create_answer(uid, QTYPE_TEXT, "{ \"answer\": 42 }", "unit");
      ret = serialise_answer(in, ANSWER_SCOPE_FULL, str, 1024);
      ASSERT(ret == 0, "answer '%s' serialised", uid);

      struct answer *out = calloc(sizeof(struct answer), 1);
      ret = deserialise_answer(str, ANSWER_SCOPE_FULL, out);
      ASSERT(ret == 0, "answer '%s' deserialised", uid);

      assert_answers_compare(in, out);  // answers are freed in func
    }

    SECTION("answer serialisation (NULL pointer for text), #421");
    {
      char str[1024];
      int ret;

      struct answer *a = calloc(sizeof(struct answer), 1);
      a->type = QTYPE_TEXT;

      // precondtions (silent)
      assert(a->uid == NULL); // !!
      assert(a->text == NULL);
      assert(a->unit == NULL);

      ret = serialise_answer(a, ANSWER_SCOPE_FULL, str, 1024);
      ASSERT(ret == 0, "answer did serialise", "");
      ASSERT_STR_EQ(str, ":TEXT::0:0:0:0:0:0:0::0:0", "(no uid!)");
      free_answer(a);
    }

    SECTION("sha1 tests: sha1_string(), #268, #237");

    {
      char hash[HASHSTRING_LENGTH];
      int ret;
      LOG_MUTE(); // supress error printing in sha1.c

      // test stolen from sha1.c:main()
      ret = sha1_string("abc", hash);
      ASSERT(ret == 0, "sha1_string('abc') passes", "");
      ret = strcmp(hash, "a9993e364706816aba3e25717850c26c9cd0d89d");
      ASSERT(ret == 0, "sha1_string('abc'): '%s' == '%s'", hash, "a9993e364706816aba3e25717850c26c9cd0d89d");

      // stolen from sha1.c:main()
      ret = sha1_string("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", hash);
      ASSERT(ret == 0, "sha1_string('abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq') passes", "");
      ret = strcmp(hash, "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
      ASSERT(ret == 0, "sha1_string('abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq'): '%s' == '%s'", hash, "84983e441c3bd26ebaae4aa1f95129e5e54670f1");

      ret = sha1_string("", hash);
      ASSERT(ret != 0, "sha1_string('', hash) returns error", "");

      ret = sha1_string(NULL, hash);
      ASSERT(ret != 0, "sha1_string(NULL, hash) returns error", "");

      ret = sha1_string("abc", NULL);
      ASSERT(ret != 0, "sha1_string('abc', NULL) returns error", "");

      LOG_UNMUTE();
    }

    SECTION("sha1 tests: sha1_hash(), #237");

    {
      /* note: following tests rely on thest sha1_string('abc') */
      int ret;
      sha1nfo info;
      char hash[256];

      LOG_MUTE(); // supress error printing in sha1.c

      sha1_init(&info);
      sha1_write(&info, "abc", 3);
      ret = sha1_hash(&info, hash);
      ASSERT(ret == 0, "sha1_hash('%s', '%s') is valid", "abc", "a9993e364706816aba3e25717850c26c9cd0d89d");
      ret = strcmp(hash, "a9993e364706816aba3e25717850c26c9cd0d89d");
      ASSERT(ret == 0, "sha1_hash('abc'): '%s' == '%s'", hash, "a9993e364706816aba3e25717850c26c9cd0d89d");

      sha1_init(&info);
      // example of <session_id><state> pattern
      sha1_write(&info, "e3bbd48d-0000-0000-78b1-35c785bf3e861", 37);
      ret = sha1_hash(&info, hash);
      ASSERT(ret == 0, "sha1_hash('%s', '%s') is valid", "e3bbd48d-0000-0000-78b1-35c785bf3e861", "79504b0a11b93721911e2547a60ae53e3982f179");
      ret = strcmp(hash, "79504b0a11b93721911e2547a60ae53e3982f179");
      ASSERT(ret == 0, "sha1_hash('abc'): '%s' == '%s'", hash, "79504b0a11b93721911e2547a60ae53e3982f179");

      LOG_UNMUTE();
    }

    SECTION("sha1 tests: sha1_validate_string(), #237");

    {
      /* note: following tests rely on thest sha1_string('abc') */
      int ret;
      LOG_MUTE(); // supress error printing in sha1.c

      ret = sha1_validate_string("abc", "a9993e364706816aba3e25717850c26c9cd0d89d");
      ASSERT(ret == 0, "sha1_string('%s', '%s') is valid", "abc", "a9993e364706816aba3e25717850c26c9cd0d89d");

      ret = sha1_validate_string("abcd", "a9993e364706816aba3e25717850c26c9cd0d89d");
      ASSERT(ret != 0, "sha1_string('%s', '%s') is invalid (wrong src)", "abcd", "a9993e364706816aba3e25717850c26c9cd0d89d");

      ret = sha1_validate_string("abc", "99993e364706816aba3e25717850c26c9cd0d89d");
      ASSERT(ret != 0, "sha1_string('%s', '%s') is invalid (wrong shash)", "abcd", "99993e364706816aba3e25717850c26c9cd0d89d");

      ret = sha1_validate_string("abc", "99993e364706816aba3");
      ASSERT(ret != 0, "sha1_string('%s', '%s') is invalid (invalid hash)", "abcd", "d9");

      ret = sha1_validate_string(NULL, "a9993e364706816aba3e25717850c26c9cd0d89d");
      ASSERT(ret != 0, "sha1_string(NULL, '%s') is invalid", "a9993e364706816aba3e25717850c26c9cd0d89d");

      ret = sha1_validate_string("abc", NULL);
      ASSERT(ret != 0, "sha1_string('%s', NULL) is invalid", "abc");

      ret = sha1_validate_string("", "");
      ASSERT(ret != 0, "sha1_string('', '') is invalid", "");

      LOG_UNMUTE();
    }

    SECTION("sha1_validate_string_hashlike(), #268");

    {
      int ret;
      LOG_MUTE(); // supress error printing in sha1.c

      ret = sha1_validate_string_hashlike(NULL);
      ASSERT(ret < 0, "FAIL: string has is '%s'", "NULL");

      ret = sha1_validate_string_hashlike("1");
      ASSERT(ret < 0, "FAIL: string too short '%s'", "1");

      ret = sha1_validate_string_hashlike("a9993e364706816aba3e25717850c26c9cd0d89d");
      ASSERT(ret == 0, "PASS: string '%s'", "a9993e364706816aba3e25717850c26c9cd0d89d");

      ret = sha1_validate_string_hashlike("9993e364706816aba3e25717850c26c9cd0d89d");
      ASSERT(ret < 0, "PASS: too short '%s'", "9993e364706816aba3e25717850c26c9cd0d89d");

      ret = sha1_validate_string_hashlike("ta9993e364706816aba3e25717850c26c9cd0d89d");
      ASSERT(ret < 0, "FAIL: too long '%s'", "t9993e364706816aba3e25717850c26c9cd0d89d");

      ret = sha1_validate_string_hashlike("+9993e364706816aba3e25717850c26c9cd0d89d");
      ASSERT(ret < 0, "FAIL: contains invalid characters '%s'", "+9993e364706816aba3e25717850c26c9cd0d89d");

      LOG_UNMUTE();
    }

    SECTION("sessions answer_set_value_raw(), #442");

    {
      int ret;
      struct answer *a;

      // args

      ret = answer_set_value_raw(NULL, NULL);
      ASSERT(ret < 0, "%s, FAIL: NULL, NULL", "args");

      a = create_answer("args", QTYPE_TEXT, NULL, NULL);
      ret = answer_set_value_raw(a, NULL);
      ASSERT(ret < 0, "%s, FAIL: answer, NULL", "args");
      free_answer(a);

      a = create_answer("args", -1, NULL, NULL);
      ret = answer_set_value_raw(a, "overwrite");
      ASSERT(ret < 0, "%s, FAIL: unknown question type", "args");
      free_answer(a);

      // QTYPE_META system answers

      a = create_answer("meta", QTYPE_META, NULL, NULL);
      ret = answer_set_value_raw(a, "something");
      ASSERT(ret < 0, "%s, FAIL: (system answer)", "QTYPE_META");
      free_answer(a);

      a = create_answer("@meta", QTYPE_TEXT, NULL, NULL);
      ret = answer_set_value_raw(a, "something");
      ASSERT(ret < 0, "%s, FAIL: uid starts with '@' (system answer)", "SYSTEM PREFIX");
      free_answer(a);

      // QTYPE_INT

      a = create_answer("int", QTYPE_INT, NULL, NULL);
      ret = answer_set_value_raw(a, "2");
      ASSERT(ret == 0, "%s, PASS", "QTYPE_INT");
      ASSERT(a->value == 2, "%s, PASS: value", "QTYPE_INT");
      free_answer(a);

      a = create_answer("int", QTYPE_INT, NULL, NULL);
      ret = answer_set_value_raw(a, "invalid");
      ASSERT(ret < 0, "%s, FAIL: string value", "QTYPE_INT");
      free_answer(a);

      a = create_answer("int", QTYPE_INT, NULL, NULL);
      ret = answer_set_value_raw(a, "1.2");
      ASSERT(ret < 0, "%s, FAIL: invalid value '1.2'", "QTYPE_INT");
      free_answer(a);

      // QTYPE_FIXEDPOINT
      SKIP("%s is currently an alias of %s, see #424 for planned changes", "QTYPE_FIXEDPOINT", "QTYPE_INT");

      // QTYPE_MULTICHOICE

      a = create_answer("multichoice", QTYPE_MULTICHOICE, NULL, NULL);
      ret = answer_set_value_raw(a, "hello");
      ASSERT(ret == 0, "%s, PASS: single", "QTYPE_MULTICHOICE");
      ASSERT_STR_EQ(a->text, "hello", "QTYPE_MULTICHOICE, PASS: value");
      free_answer(a);

      a = create_answer("multichoice", QTYPE_MULTICHOICE, NULL, NULL);
      ret = answer_set_value_raw(a, "");
      ASSERT(ret == 0, "%s, PASS: empty", "QTYPE_MULTICHOICE");
      ASSERT_STR_EQ(a->text, "", "QTYPE_MULTICHOICE, PASS: empty");
      free_answer(a);

      a = create_answer("multichoice", QTYPE_MULTICHOICE, NULL, NULL);
      ret = answer_set_value_raw(a, "hello,world");
      ASSERT(ret == 0, "%s, PASS: multiple", "QTYPE_MULTICHOICE");
      ASSERT_STR_EQ(a->text, "hello,world", "QTYPE_MULTICHOICE, PASS: multiple values");
      free_answer(a);

      a = create_answer("multichoice", QTYPE_MULTICHOICE, NULL, NULL);
      ret = answer_set_value_raw(a, "hello,");
      ASSERT(ret == 0, "%s, PASS: trailing comma", "QTYPE_MULTICHOICE");
      ASSERT_STR_EQ(a->text, "hello,", "QTYPE_MULTICHOICE, PASS: trailing comma");
      free_answer(a);

      a = create_answer("multichoice", QTYPE_MULTICHOICE, NULL, NULL);
      a->text = strdup("has value");
      ret = answer_set_value_raw(a, "overwrite");
      ASSERT(ret < 0, "%s, FAIL: overwrite existing a->text ", "QTYPE_MULTICHOICE");
      free_answer(a);

      // QTYPE_MULTISELECT

      SKIP("%s is an alias of %s", "QTYPE_MULTISELECT", "QTYPE_MULTICHOICE");

      // QTYPE_LATLON

      a = create_answer("latlon", QTYPE_LATLON, NULL, NULL);
      ret = answer_set_value_raw(a, "1,2");
      ASSERT(ret == 0, "%s, PASS: single", "QTYPE_LATLON");
      ASSERT((a->lat == 1 && a->lon == 2), "hello", "QTYPE_LATLON, PASS: values");
      free_answer(a);

      a = create_answer("latlon", QTYPE_LATLON, NULL, NULL);
      ret = answer_set_value_raw(a, "");
      ASSERT(ret < 0, "%s, FAIL: empty", "QTYPE_LATLON");
      free_answer(a);

      a = create_answer("latlon", QTYPE_LATLON, NULL, NULL);
      ret = answer_set_value_raw(a, "1,");
      ASSERT(ret < 0, "%s, FAIL: incomplete", "QTYPE_LATLON");
      free_answer(a);

      a = create_answer("latlon", QTYPE_LATLON, NULL, NULL);
      ret = answer_set_value_raw(a, "1,2,3");
      ASSERT(ret < 0, "%s, FAIL: value column overflow", "QTYPE_LATLON");
      free_answer(a);

      a = create_answer("latlon", QTYPE_LATLON, NULL, NULL);
      ret = answer_set_value_raw(a, "hello,1");
      ASSERT(ret < 0, "%s, FAIL: invalid column 1", "QTYPE_LATLON");
      free_answer(a);

      a = create_answer("latlon", QTYPE_LATLON, NULL, NULL);
      ret = answer_set_value_raw(a, "1,world");
      ASSERT(ret < 0, "%s, FAIL: invalid column 2", "QTYPE_LATLON");
      free_answer(a);

      // QTYPE_DATETIME

      a = create_answer("datetime", QTYPE_DATETIME, NULL, NULL);
      ret = answer_set_value_raw(a, "2");
      ASSERT(ret == 0, "%s, PASS", "QTYPE_DATETIME");
      ASSERT(a->time_begin == 2, "%s, PASS: value", "QTYPE_DATETIME");
      free_answer(a);

      a = create_answer("datetime", QTYPE_DATETIME, NULL, NULL);
      ret = answer_set_value_raw(a, "-1");
      ASSERT(ret == 0, "%s, PASS", "QTYPE_DATETIME negative");
      ASSERT(a->time_begin == -1, "%s, PASS: negative value", "QTYPE_DATETIME");
      free_answer(a);

      a = create_answer("datetime", QTYPE_DATETIME, NULL, NULL);
      ret = answer_set_value_raw(a, "invalid");
      ASSERT(ret < 0, "%s, FAIL: string value", "QTYPE_DATETIME");
      free_answer(a);

      a = create_answer("datetime", QTYPE_DATETIME, NULL, NULL);
      ret = answer_set_value_raw(a, "1.2");
      ASSERT(ret < 0, "%s, FAIL: invalid value '1.2'", "QTYPE_DATETIME");
      free_answer(a);

      // QTYPE_DATETIME

      a = create_answer("datetime", QTYPE_DATETIME, NULL, NULL);
      ret = answer_set_value_raw(a, "2");
      ASSERT(ret == 0, "%s, PASS", "QTYPE_DATETIME");
      ASSERT(a->time_begin == 2, "%s, PASS: value", "QTYPE_DATETIME");
      free_answer(a);

      a = create_answer("datetime", QTYPE_DATETIME, NULL, NULL);
      ret = answer_set_value_raw(a, "-1"); // TODO allow? future validator
      ASSERT(ret == 0, "%s, PASS", "QTYPE_DATETIME negative");
      ASSERT(a->time_begin == -1, "%s, PASS: negative value", "QTYPE_DATETIME");
      free_answer(a);

      a = create_answer("datetime", QTYPE_DATETIME, NULL, NULL);
      ret = answer_set_value_raw(a, "invalid");
      ASSERT(ret < 0, "%s, FAIL: string value", "QTYPE_DATETIME");
      free_answer(a);

      a = create_answer("datetime", QTYPE_DATETIME, NULL, NULL);
      ret = answer_set_value_raw(a, "1.2");
      ASSERT(ret < 0, "%s, FAIL: invalid value '1.2'", "QTYPE_DATETIME");
      free_answer(a);

      // QTYPE_DATETIME

      SKIP("%s is currently an alias of %s, see #414 for planned changes (negative values)", "QTYPE_DATETIME", "QTYPE_DATETIME");

      // QTYPE_TIMERANGE

      a = create_answer("timerange", QTYPE_TIMERANGE, NULL, NULL);
      ret = answer_set_value_raw(a, "1,2");
      ASSERT(ret == 0, "%s, PASS: single", "QTYPE_TIMERANGE");
      ASSERT((a->time_begin == 1 && a->time_end == 2), "hello", "QTYPE_TIMERANGE, PASS: values");
      free_answer(a);

      a = create_answer("timerange", QTYPE_TIMERANGE, NULL, NULL);
      ret = answer_set_value_raw(a, "");
      ASSERT(ret < 0, "%s, FAIL: empty", "QTYPE_TIMERANGE");
      free_answer(a);

      a = create_answer("timerange", QTYPE_TIMERANGE, NULL, NULL);
      ret = answer_set_value_raw(a, "1,");
      ASSERT(ret < 0, "%s, FAIL: incomplete", "QTYPE_TIMERANGE");
      free_answer(a);

      a = create_answer("timerange", QTYPE_TIMERANGE, NULL, NULL);
      ret = answer_set_value_raw(a, "1,2,3");
      ASSERT(ret < 0, "%s, FAIL: value column overflow", "QTYPE_TIMERANGE");
      free_answer(a);

      a = create_answer("timerange", QTYPE_TIMERANGE, NULL, NULL);
      ret = answer_set_value_raw(a, "hello,1");
      ASSERT(ret < 0, "%s, FAIL: invalid column 1", "QTYPE_TIMERANGE");
      free_answer(a);

      a = create_answer("timerange", QTYPE_TIMERANGE, NULL, NULL);
      ret = answer_set_value_raw(a, "1,world");
      ASSERT(ret < 0, "%s, FAIL: invalid column 2", "QTYPE_TIMERANGE");
      free_answer(a);

      // QTYPE_UPLOAD

      SKIP("%s is currently not implemented", "QTYPE_UPLOAD");

      // QTYPE_TEXT

      a = create_answer("text", QTYPE_TEXT, NULL, NULL);
      ret = answer_set_value_raw(a, "hello");
      ASSERT(ret == 0, "%s, PASS", "QTYPE_TEXT");
      ASSERT_STR_EQ(a->text, "hello", "QTYPE_TEXT, PASS: value");
      free_answer(a);

      a = create_answer("text", QTYPE_TEXT, NULL, NULL);
      ret = answer_set_value_raw(a, "");
      ASSERT(ret == 0, "%s, PASS: empty", "QTYPE_TEXT");
      ASSERT_STR_EQ(a->text, "", "QTYPE_TEXT, PASS: empty");
      free_answer(a);

      a = create_answer("text", QTYPE_TEXT, NULL, NULL);
      a->text = strdup("has value");
      ret = answer_set_value_raw(a, "overwrite");
      ASSERT(ret < 0, "%s, FAIL: overwrite existing a->text ", "QTYPE_TEXT");
      free_answer(a);

      // QTYPE_CHECKBOX

      SKIP("%s is an alias of %s", "QTYPE_CHECKBOX", "QTYPE_TEXT");

      // QTYPE_HIDDEN

      SKIP("%s is an alias of %s", "QTYPE_HIDDEN", "QTYPE_TEXT");

      // QTYPE_TEXTAREA

      SKIP("%s is an alias of %s", "QTYPE_TEXTAREA", "QTYPE_TEXT");

      // QTYPE_EMAIL

      SKIP("%s is an alias of %s", "QTYPE_EMAIL", "QTYPE_TEXT");

      //  QTYPE_SINGLECHOICE

      SKIP("%s is an alias of %s", "QTYPE_SINGLECHOICE", "QTYPE_TEXT");

      //  QTYPE_QTYPE_SINGLESELECT

      SKIP("%s is an alias of %s", "QTYPE_SINGLESELECT", "QTYPE_TEXT");

      // QTYPE_FIXEDPOINT_SEQUENCE

      SKIP("%s is currently an alias of %s, see #414 for planned changes (validate type)", "QTYPE_FIXEDPOINT_SEQUENCE", "QTYPE_TEXT");

      // QTYPE_DAYTIME_SEQUENCE

      SKIP("%s is currently an alias of %s, see #414 for planned changes (validate type)", "QTYPE_DAYTIME_SEQUENCE", "QTYPE_TEXT");

      // QTYPE_DATETIME_SEQUENCE

      SKIP("%s is currently an alias of %s, see #414 for planned changes (validate type)", "QTYPE_DATETIME_SEQUENCE", "QTYPE_TEXT");

      // QTYPE_DURATION24

      SKIP("%s is currently an alias of %s, see #414 for planned changes (min,max)", "QTYPE_DURATION24", "QTYPE_INT");

      // QTYPE_DIALOG_DATA_CRAWLER

      SKIP("%s is an alias of %s", "QTYPE_DURATION24", "QTYPE_TEXT");

      // QTYPE_SHA1_HASH

      SKIP("%s is currently an alias of %s, see #414 for planned changes (length)", "QTYPE_SHA1_HASH", "QTYPE_TEXT");

      // QTYPE_UUID

      SKIP("%s is an alias of %s", "QTYPE_UUID", "QTYPE_TEXT");

    }

    ////
    // copy question
    ////

    SECTION("copy question default value overwrite, #463, #213");

    {
      struct question *src = calloc(sizeof(struct question), 1);
      deserialise_question("question1:Question 1::TEXT:0:DEFAULT:-1:-1:0:0::", src);
      struct question *cpy = copy_question(src, NULL);

      ASSERT_STR_EQ(src->default_value, "DEFAULT", "source qn default value");
      ASSERT_STR_EQ(cpy->default_value, "DEFAULT", "copy qn default value == src (NULL POINTER)");
      free_question(src);
      free_question(cpy);
    }

    {
      struct question *src = calloc(sizeof(struct question), 1);
      deserialise_question("question1:Question 1::TEXT:0:DEFAULT:-1:-1:0:0::", src);
      struct question *cpy = copy_question(src, "");

      ASSERT_STR_EQ(src->default_value, "DEFAULT", "source qn default value");
      ASSERT_STR_EQ(cpy->default_value, "DEFAULT", "copy qn default value == src (EMPTY STRING)");
      free_question(src);
      free_question(cpy);
    }

    {
      struct question *src = calloc(sizeof(struct question), 1);
      deserialise_question("question1:Question 1::TEXT:0:DEFAULT:-1:-1:0:0::", src);
      struct question *cpy = copy_question(src, "OVERWRITE");

      ASSERT_STR_EQ(src->default_value, "DEFAULT", "source qn default value");
      ASSERT_STR_EQ(cpy->default_value, "OVERWRITE", "copy qn default value != src (OVERWRITE)");
      free_question(src);
      free_question(cpy);
    }

  } while (0);

  DEBUG("\n-------------\nTESTS FINISHED\n-------------\n", "");
  return retVal;
}
