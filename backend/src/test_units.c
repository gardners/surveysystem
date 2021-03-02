#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "errorlog.h"
#include "serialisers.h"
#include "survey.h"

struct answer* create_answer(char *uid, int type, char *text, char *unit) {
  struct answer *ans = calloc(sizeof(struct answer), 1);
  if (!ans) {
    return NULL;
  }

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
  FORMAT_FAIL, FORMAT_PASS, FORMAT_SECTION, FORMAT_NONE
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
  }\
  assert(expr);\
} while (0)

#define ASSERT_STR_EQ(left, right, message)\
do {\
   ASSERT(strcmp(left, right) == 0, "'%s' == '%s' %s", left, right, message); \
} while (0)

#define PASS(message, ...)\
do {\
  test_message(FORMAT_PASS, message, __VA_ARGS__);\
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

    SECTION("answer serialisation tests, #392");

    {
      char str[1024];
      char uid[] = "uid";
      int ret;

      struct answer *in = create_answer(uid, QTYPE_META, ":\n\r\b\tutext", "unit");
      ret = serialise_answer(in, str, 1024);
      ASSERT(ret == 0, "answer '%s' serialised", uid);

      struct answer *out = calloc(sizeof(struct answer), 1);
      ret = deserialise_answer(str, ANSWER_FIELDS_PROTECTED, out);
      ASSERT(ret == 0, "answer '%s' deserialised", uid);

      assert_answers_compare(in, out);  // answers are freed in func
    }

    {
      char str[1024];
      int ret;
      char uid[] = "text-is-json-array";

      struct answer *in = create_answer(uid, QTYPE_TEXT, "[42, -42]", "unit");
      ret = serialise_answer(in, str, 1024);
      ASSERT(ret == 0, "answer '%s' serialised", uid);

      struct answer *out = calloc(sizeof(struct answer), 1);
      ret = deserialise_answer(str, ANSWER_FIELDS_PROTECTED, out);
      ASSERT(ret == 0, "answer '%s' deserialised", uid);

      assert_answers_compare(in, out);  // answers are freed in func
    }

    {
      char str[1024];
      int ret;
      char uid[] = "text-is-json-object";

      struct answer *in = create_answer(uid, QTYPE_TEXT, "{ \"answer\": 42 }", "unit");
      ret = serialise_answer(in, str, 1024);
      ASSERT(ret == 0, "answer '%s' serialised", uid);

      struct answer *out = calloc(sizeof(struct answer), 1);
      ret = deserialise_answer(str, ANSWER_FIELDS_PROTECTED, out);
      ASSERT(ret == 0, "answer '%s' deserialised", uid);

      assert_answers_compare(in, out);  // answers are freed in func
    }

  } while (0);

  return retVal;
}
