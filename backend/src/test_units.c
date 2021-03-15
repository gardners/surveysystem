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

    SECTION("serialiser_count_columns(), #413");

    {
      LOG_UNMUTE();
      int ret;

      ret = serialiser_count_columns("", 1024);
      ASSERT(ret == 1, "serialiser_count_columns('%s')", "");

      ret = serialiser_count_columns(NULL, 1024);
      ASSERT(ret == -1, "error: serialiser_count_columns(NULL)", "");

      ret = serialiser_count_columns("hello", 1024);
      ASSERT(ret == 1, "serialiser_count_columns('%s')", "hello");

      ret = serialiser_count_columns("hello", 2);
      ASSERT(ret == -1, "error: serialiser_count_columns('%s') out of bounds", "hello");

      ret = serialiser_count_columns(":", 1024);
      ASSERT(ret == 2, "serialiser_count_columns('%s')", ":");

      ret = serialiser_count_columns("\\:", 1024);
      ASSERT(ret == 1, "escape: serialiser_count_columns('%s')", "\\:");

      ret = serialiser_count_columns("\\\\:", 1024);
      ASSERT(ret == 1, "double escape: serialiser_count_columns('%s')", "\\\\:");

      ret = serialiser_count_columns(":\\:", 1024);
      ASSERT(ret == 2, "escape inner: serialiser_count_columns('%s')", ":\\:");

      ret = serialiser_count_columns("::1", 1024);
      ASSERT(ret == 3, "serialiser_count_columns('%s')", "::1");

      ret = serialiser_count_columns("::", 1024);
      ASSERT(ret == 3, "serialiser_count_columns('%s')", ":::");

      ret = serialiser_count_columns("1:1:1", 1024);
      ASSERT(ret == 3, "serialiser_count_columns('%s')", "1:1:1");

      ret = serialiser_count_columns("1:1:1", 2);
      ASSERT(ret == -1, "error serialiser_count_columns('%s') out of bounds", "1:1:1");

      ret = serialiser_count_columns("\n:1:1:1", 1024);
      ASSERT(ret == -1, "error starting endline serialiser_count_columns('%s') %d", "\\n:1:1:1", ret);

      ret = serialiser_count_columns(":1:1:1\n", 1024);
      ASSERT(ret == -1, "error ending endline serialiser_count_columns('%s') %d", ":1:1:1\\n", ret);

      ret = serialiser_count_columns("1:1:1\n1:1:1", 1024);
      ASSERT(ret == -1, "error inner endline serialiser_count_columns('%s') %d", "1:1:1\\n1:1:1", ret);

      ret = serialiser_count_columns("1:1:1\\n1:1:1", 1024);
      ASSERT(ret == 5, "escaped inner endline serialiser_count_columns('%s') %d", "1:1:1\\\\n1:1:1", ret);

      ret = serialiser_count_columns("uid:META:\\:\\n\r\b\tutext:0:0:0:0:0:0:0:unit:0:0", 1024);
      ASSERT(ret == ANSWER_SCOPE_FULL, "anwswer ANSWER_FIELDS_PROTECTED with escapes passing serialiser_count_columns('%s') %d", "uid:META:\\:\\n\\r\\b\\tutext:0:0:0:0:0:0:0:unit:0:0", ret);

      ret = serialiser_count_columns("uid:\\:\\n\r\b\tutext:0:0:0:0:0:0:0:", 1024);
      ASSERT(ret == ANSWER_SCOPE_PUBLIC, "anwswer ANSWER_FIELDS_PUBLIC with escapes passing serialiser_count_columns('%s') %d", "uid:META:\\:\\n\\r\\b\\tutext:0:0:0:0:0:0:0:unit:0:0", ret);
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

      struct answer *in = create_answer("serialise-public", QTYPE_TEXT, "something", "unit");
      in->stored = 123;
      ret = serialise_answer(in, ANSWER_SCOPE_PUBLIC, str, 1024);
      ASSERT(ret == 0, "answer '%s' serialised %d", in->uid, ret);
      ASSERT_STR_EQ(str, "serialise-public:something:0:0:0:0:0:0:0:unit", "");
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

    SECTION("sha1 tests: validate_string_sha1(), #237");

    {
      /* note: following tests rely on thest sha1_string('abc') */
      int ret;
      LOG_MUTE(); // supress error printing in sha1.c

      ret = validate_string_sha1("abc", "a9993e364706816aba3e25717850c26c9cd0d89d");
      ASSERT(ret == 0, "sha1_string('%s', '%s') is valid", "abc", "a9993e364706816aba3e25717850c26c9cd0d89d");

      ret = validate_string_sha1("abcd", "a9993e364706816aba3e25717850c26c9cd0d89d");
      ASSERT(ret != 0, "sha1_string('%s', '%s') is invalid (wrong src)", "abcd", "a9993e364706816aba3e25717850c26c9cd0d89d");

      ret = validate_string_sha1("abc", "99993e364706816aba3e25717850c26c9cd0d89d");
      ASSERT(ret != 0, "sha1_string('%s', '%s') is invalid (wrong shash)", "abcd", "99993e364706816aba3e25717850c26c9cd0d89d");

      ret = validate_string_sha1("abc", "99993e364706816aba3");
      ASSERT(ret != 0, "sha1_string('%s', '%s') is invalid (invalid hash)", "abcd", "d9");

      ret = validate_string_sha1(NULL, "a9993e364706816aba3e25717850c26c9cd0d89d");
      ASSERT(ret != 0, "sha1_string(NULL, '%s') is invalid", "a9993e364706816aba3e25717850c26c9cd0d89d");

      ret = validate_string_sha1("abc", NULL);
      ASSERT(ret != 0, "sha1_string('%s', NULL) is invalid", "abc");

      ret = validate_string_sha1("", "");
      ASSERT(ret != 0, "sha1_string('', '') is invalid", "");

      LOG_UNMUTE();
    }

  } while (0);

  DEBUG("\n-------------\nTESTS FINISHED\n-------------\n", "");
  return retVal;
}
