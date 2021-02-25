
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "errorlog.h"
#include "serialisers.h"
#include "survey.h"

void assert_string_eq(char *left, char *right) {
  if (strcmp(left, right) == 0) {
     fprintf(stderr, "[PASS ] '%s'\n", left);
     return;
  }
  fprintf(stderr, "[ERROR] '%s' != '%s'!\n", left, right);
  exit(1);
}

void assert_answers_compare(struct answer *in, struct answer *out) {
  if(compare_answers(in, out, MISMATCH_IS_AN_ERROR)) {
     fprintf(stderr, "[ERROR] '%s' != '%s'!\n", in->uid, out->uid);
     fprintf(stderr, ">> in: \n");
     dump_answer(stderr, in);
     fprintf(stderr, "<< out: \n");
     dump_answer(stderr, out);

     free_answer(in);
     free_answer(out);
     exit(1);
  }

  fprintf(stderr, "[PASS ] answers match: '%s' == '%s'\n", in->uid, out->uid);
  free_answer(in);
  free_answer(out);
}

struct answer *create_answer(char *uid, int type, char*text, char *unit) {
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

struct question_serialiser_test qst[] = {

    // An empty string should not be accepted
    {"Empty string not accepted",
     SHOULD_FAIL,
     DIRECTION_DESERIALISE,
     "",
     {NULL}},

    // A simple valid record should be accepted.
    {"Simple serialised question",
     SHOULD_PASS,
     DIRECTION_SERIALISE | DIRECTION_DESERIALISE,
     "dummyuid:"
     "What is the answer to life, the universe and everything?:"
     "<div>What is the answer to life, the universe and everything?</div>:"
     "INT:0:42:0:100:0:1::",
     {"dummyuid", "What is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, 1, "", ""}},

    // Only valid question types should be accepted
    {"Illegal question type fails",
     SHOULD_FAIL,
     DIRECTION_DESERIALISE,
     "dummyuid:"
     "What is the answer to life, the universe and everything?:"
     "<div>What is the answer to life, the universe and everything?</div>:"
     "FISH:0:42:0:100:0:1::",
     {"dummyuid", "What is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, 1, "", ""}},

    {"Negative numbers are accepted",
     SHOULD_PASS,
     DIRECTION_DESERIALISE | DIRECTION_SERIALISE,
     "dummyuid:"
     "What is the answer to life, the universe and everything?:"
     "<div>What is the answer to life, the universe and everything?</div>:"
     "INT:0:42:0:-100:0:-1::",
     {"dummyuid", "What is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, -100, 0, -1, "", ""}},

    {"Minus sign can appear only at beginning of a number",
     SHOULD_FAIL,
     DIRECTION_DESERIALISE,
     "dummyuid:"
     "What is the answer to life, the universe and everything?:"
     "<div>What is the answer to life, the universe and everything?</div>:"
     "INT:0:42:0:10-0:0:-1::",
     {"dummyuid", "What is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, -1, "", ""}},

    {"Missing fields results in failure",
     SHOULD_FAIL,
     DIRECTION_DESERIALISE,
     "dummyuid:"
     "What is the answer to life, the universe and everything?:"
     "<div>What is the answer to life, the universe and everything?</div>:"
     "INT:0:42:0:10-0:0::",
     {"dummyuid", "What is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, -1, "", ""}},

    {"Missing fields results in failure",
     SHOULD_FAIL,
     DIRECTION_DESERIALISE,
     "dummyuid:"
     "What is the answer to life, the universe and everything?:"
     "<div>What is the answer to life, the universe and everything?</div>:"
     "INT:0:42:0:10-0:0::",
     {"dummyuid", "What is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, -1, "", ""}},

    {"Extra fields results in failure",
     SHOULD_FAIL,
     DIRECTION_DESERIALISE,
     "dummyuid:"
     "What is the answer to life, the universe and everything?:"
     "<div>What is the answer to life, the universe and everything?</div>:"
     "INT:0:42:0:10-0:0:-1:::-1",
     {"dummyuid", "What is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, -1, "", ""}},

    {"\\t escape is accepted in strings",
     SHOULD_PASS,
     DIRECTION_DESERIALISE | DIRECTION_SERIALISE,
     "dummyuid:"
     "\tWhat is the answer to life, the universe and everything?:"
     "<div>What is the answer to life, the universe and everything?</div>:"
     "INT:0:42:0:100:0:-1::",
     {"dummyuid", "\tWhat is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, -1, "", ""}},

    {"\\r escape is accepted in strings",
     SHOULD_PASS,
     DIRECTION_DESERIALISE | DIRECTION_SERIALISE,
     "dummyuid:"
     "\rWhat is the answer to life, the universe and everything?:"
     "<div>What is the answer to life, the universe and everything?</div>:"
     "INT:0:42:0:100:0:-1::",
     {"dummyuid", "\rWhat is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, -1, "", ""}},

    {"\\n escape is accepted in strings",
     SHOULD_PASS,
     DIRECTION_DESERIALISE | DIRECTION_SERIALISE,
     "dummyuid:"
     "\nWhat is the answer to life, the universe and everything?:"
     "<div>What is the answer to life, the universe and everything?</div>:"
     "INT:0:42:0:100:0:-1::",
     {"dummyuid", "\nWhat is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, -1, "", ""}},

    {"\\: escape is accepted in strings",
     SHOULD_PASS,
     DIRECTION_DESERIALISE | DIRECTION_SERIALISE,
     "dummyuid:"
     "\\:What is the answer to life, the universe and everything?:"
     "<div>What is the answer to life, the universe and everything?</div>:"
     "INT:0:42:0:100:0:-1::",
     {"dummyuid", ":What is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, -1, "", ""}},

    {"\\\\ escape is accepted in strings",
     SHOULD_PASS,
     DIRECTION_DESERIALISE | DIRECTION_SERIALISE,
     "dummyuid:"
     "\\\\What is the answer to life, the universe and everything?:"
     "<div>What is the answer to life, the universe and everything?</div>:"
     "INT:0:42:0:100:0:-1::",
     {"dummyuid", "\\What is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, -1, "", ""}},

    {"Multiple escape is accepted in strings",
     SHOULD_PASS,
     DIRECTION_DESERIALISE | DIRECTION_SERIALISE,
     "dummyuid:"
     "\\\\\r\n\t\\:What is the answer to life, the universe and everything?:"
     "<div>What is the answer to life, the universe and everything?</div>:"
     "INT:0:42:0:100:0:-1::",
     {"dummyuid",
      "\\\r\n\t:What is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_INT, 0, "42", 0, 100, 0, -1, "", ""}},

    {"numchoices should match number of colon separated items in choices field",
     SHOULD_PASS,
     DIRECTION_DESERIALISE | DIRECTION_SERIALISE,
     "dummyuid:"
     "\\\\\r\n\t\\:What is the answer to life, the universe and everything?:"
     "<div>What is the answer to life, the universe and everything?</div>:"
     "MULTICHOICE:0:42:0:100:0:1:this,that:",
     {"dummyuid",
      "\\\r\n\t:What is the answer to life, the universe and everything?",
      "<div>What is the answer to life, the universe and everything?</div>",
      QTYPE_MULTICHOICE, 0, "42", 0, 100, 0, 1, "this,that", ""}},

    {NULL, -1, -1, NULL, {NULL}}};

int main(int argc, char **argv) {
  int retVal = 0;

  do {

    setenv("SURVEY_HOME", ".", 0);

    int fail = 0;
    int pass = 0;
    int errors = 0;

    for (int i = 0; qst[i].name; i++) {
      fprintf(stderr, "[     ] %s", qst[i].name);
      fflush(stderr);

      clear_errors();

      if (qst[i].direction & DIRECTION_SERIALISE) {
        // XXX Implement
      }
      if (qst[i].direction & DIRECTION_DESERIALISE) {
        struct question d;
        bzero(&d, sizeof(struct question));
        int deserialise_result = deserialise_question(qst[i].serialised, &d);

        if (deserialise_result && qst[i].shouldPassP) {
          // Deserialisation failed when it should have succeeded.
          fprintf(stderr, "\r[FAIL \n  FAIL: serialised string triggered an "
                          "error during deserialisate\n");
          fprintf(stderr, "Internal error log:\n");
          dump_errors(stderr);
          fail++;
        } else if ((!deserialise_result) && (!qst[i].shouldPassP)) {
          // Deserialiation passed when it should have failed.
          fprintf(stderr, "\r[FAIL \n  FAIL: invalid serialised string did not "
                          "trigger an error during deserialisation\n");
          fprintf(stderr, "Internal error log:\n");
          dump_errors(stderr);
          fail++;
        } else if ((!deserialise_result) && qst[i].shouldPassP) {
          // Deserialised successfully, so make sure the field values
          // all match
          if (compare_questions(&d, &qst[i].question, MISMATCH_IS_AN_ERROR)) {
            fprintf(
                stderr,
                "\r[FAIL \n  FAIL: Original and serialised-then-deserialised "
                "question structures differ\n");
            fprintf(stderr, "Deserialised result");
            dump_question(stderr, &d);
            fprintf(stderr, "Expected result");
            dump_question(stderr, &qst[i].question);
            fprintf(stderr, "Internal error log:\n");
            dump_errors(stderr);
            fail++;
          } else {
            fprintf(stderr, "\r[PASS \n");
            pass++;
          }
        } else if ((deserialise_result) && (!qst[i].shouldPassP)) {
          fprintf(stderr, "\r[PASS \n");
          pass++;
        } else {
          fprintf(stderr,
                  "\r[ERROR\n  ERROR: deserialisation failed unexpectedly: "
                  "deserialise_result=%d, shouldPass=%d\n",
                  deserialise_result, qst[i].shouldPassP);
          errors++;
        }
      }
    }

    fprintf(stderr,
            "Summary: %d tests passed, %d failed and %d encountered internal "
            "errors.\n",
            pass, fail, errors);

     /* tests for #392 */
     fprintf(stderr, "\n --- escape_string() tests, #392 ---\n\n");

     {
       char str[1024];

       escape_string("hello", str, 1024);
       assert_string_eq(str, "hello");

       escape_string("::1(9000)", str, 1024);
       assert_string_eq(str, "\\:\\:1(9000)");

       escape_string("\\r\\n\\t:(9000)", str, 1024);
       assert_string_eq(str, "\\r\\n\\t\\:(9000)");

       escape_string("\t\r\n:(9000)", str, 1024);
       assert_string_eq(str, "\\t\\r\\n\\:(9000)");
     }

     fprintf(stderr, "\n --- answer serialisation tests, #392 ---\n\n");

     {
       char str[1024];
       int ret;

       struct answer *in = create_answer("uid", QTYPE_META, ":\n\r\b\tutext", "unit");
       ret = serialise_answer(in, str, 1024);
       assert(ret == 0);

       struct answer *out =  calloc(sizeof(struct answer), 1);
       ret = deserialise_answer(str, ANSWER_FIELDS_PROTECTED, out);
       assert(ret == 0);

       assert_answers_compare(in, out); // answers are freed inside
     }

     {
       char str[1024];
       int ret;

       struct answer *in = create_answer("text-is-json-array", QTYPE_TEXT, "[42, -42]", "unit");
       ret = serialise_answer(in, str, 1024);
       assert(ret == 0);

       struct answer *out =  calloc(sizeof(struct answer), 1);
       ret = deserialise_answer(str, ANSWER_FIELDS_PROTECTED, out);
       assert(ret == 0);

       assert_answers_compare(in, out); // answers are freed inside
     }

     {
       char str[1024];
       int ret;

       struct answer *in = create_answer("text-is-json-object", QTYPE_TEXT, "{ \"answer\": 42 }", "unit");
       ret = serialise_answer(in, str, 1024);
       assert(ret == 0);

       struct answer *out =  calloc(sizeof(struct answer), 1);
       ret = deserialise_answer(str, ANSWER_FIELDS_PROTECTED, out);
       assert(ret == 0);

       assert_answers_compare(in, out); // answers are freed inside
     }

  } while (0);

  return retVal;
}
