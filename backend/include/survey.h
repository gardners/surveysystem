#ifndef __SURVEY_H__
#define __SURVEY_H__

/* clang-format off */
/*
 * Disable code formatters in this section to avoid messing with the line breaks!
 * The make script does grep 'QTYPE_*' lines in order to generate 'question_types.h
 */
struct question {
  // Unique identifier
  char *uid;
  // Question text (as simple text)
  char *question_text;
  // HTML marked up version of question text, for use on HTML based display
  // systems.
  char *question_html;

  // Type of question
  int type;

  // the answer is set by the backend, a question is not defined,
  // answer cannot be deleted and must not be passed to frontend
  // the position of the answer data is flexible and depenends on the uid, multiple fields may be used
  // answers of this type might be overwritten multiple times
  // use in session header: uids start with '@' (i.e. '@user')
#define QTYPE_META                1
  // Answer is an integer, bounded by min_value and max_value
#define QTYPE_INT                 2
  // Answer is a fixed point value encoded as a 64-bit integer
  // and with decimal_places places after the decimal.
  // (we don't allow true floating point, as encoding differences can be a
  // pain, especially when compressing)
#define QTYPE_FIXEDPOINT          3
#define QTYPE_MULTICHOICE         4  // answer->text (comma separated): instruction for multi choice inputs (checkbox), answer is a single choice or comma separated list.
#define QTYPE_MULTISELECT         5  // answer->text (comma separated): instruction for multi choice inputs (select), answer is a single choice or comma separated list. (unit: *)
#define QTYPE_LATLON              6  // answer->lat & answer->lon: instruction for geographic coordinate ([-90 +90] & [-180 +180]). (unit: degrees)
#define QTYPE_DATETIME            7  // answer->time_begin: instruction for UNIX datetime (+-). (unit: seconds)
#define QTYPE_DAYTIME             8  // answer->time_begin: instruction for time of a generic day in seconds since midnight. (unit: seconds)
#define QTYPE_TIMERANGE           9  // answer->time_begin & answer->time_end: instruction for time range within a generic day. (unit: seconds)
#define QTYPE_UPLOAD              10
#define QTYPE_TEXT                11 // answer->text: instruction for generic text. (unit: *)
#define QTYPE_CHECKBOX            12 // answer->text: instruction for single html checkbox, requires two defined choices in the following order: [OFF-value, ON-value]. (unit: *)
#define QTYPE_HIDDEN              13 // answer->text: instruction for hidden input (pure textslide) answer is default value or default value. (unit: *)
#define QTYPE_TEXTAREA            14 // answer->text: instruction for textarea. (unit: *)
#define QTYPE_EMAIL               15 // answer->text: instruction for email input. (unit: *)
#define QTYPE_PASSWORD            16 // answer->text: instruction for (html) password input, this type can be used to mask any user input. (unit: *)
#define QTYPE_SINGLECHOICE        17 // answer->text: instruction for single choice inputs (checkbox, radios), answer is a single choice. (unit: *)
#define QTYPE_SINGLESELECT        18 // answer->text: instruction for single choice inputs (select), answer is a single choice. (unit: *)
#define QTYPE_FIXEDPOINT_SEQUENCE 19 // answer->text (comma separated): instruction an ascending sequence of FIXEDPOINT values, labels are defined in q.choices. (unit: *)
#define QTYPE_DAYTIME_SEQUENCE    20 // answer->text (comma separated): instruction an ascending sequence of DAYTIME values (comma separated), labels are defined in q.choices. (unit: seconds)
#define QTYPE_DATETIME_SEQUENCE   21 // answer->text (comma separated): instruction an ascending sequence of DATETIME values (comma separated), labels are defined in q.choices (unit: seconds)
#define QTYPE_DURATION24          22 // answer->value: instruction time period in seconds maximum 24 hours (86400 seconds), TODO enable min_value and max_value support, which would make this type redundant in favour of a more generic QTYPE_DURATION type.
#define QTYPE_DIALOG_DATA_CRAWLER 23 // answer->text: instruction for a dialog to give consent to accessing external data requires two defined choices in the following order: [DENIED, GRANTED], (unit: id of the data crawler module)
#define QTYPE_UUID                24
  // Formatting and other flags
  int flags;
  // Format integer input as times for questions like
  // "how long do you typically charge your phone?"
#define FLAG_Sec 1
#define FLAG_Min 2
#define FLAG_Hour 4
#define FLAG_Day 8
  // Convenient combinations of the above
#define FLAG_MinSec 3
#define FLAG_HourMin 6
#define FLAG_HourMinSec 7
#define FLAG_DayHourMin 13
#define FLAG_DayHourMinSec 15

  // Question gets automatically initialised
#define FLAG_AutoPopulate 16
  // Question is not visible for completion -- can only be completed
  // by auto population
#define FLAG_Hidden       32
  // Question gets given a random value in the valid range of responses
  // Only meaningful if AutoPopulate is also set
#define FLAG_RandomInitialValue 64
  // Try to populate automatically using a sensor.
  // Only meaningful for lat,lon, where GPS is used, and for date/time
  // questions, where the current date and time are acquired.
#define FLAG_Mandatory 128
  // Indicates if it is mandatory to have a (non-null?) answer to the question

  // Text rendering of default value.
  // Will be parsed for numeric answer types
  char *default_value;

  // Sets ranges for integer/fixed point answers
  // and allowed lengths of text answers
  long long min_value;
  long long max_value;
  // Number of decimal places for fixed point answers
  int decimal_places;

  // Number of choices for multi-choice/multi-select answers
  // (multi-choice encode as an integer, multi-select as string
  // of Y/N characters, or 0-9/N for "how strongly do you agree/disagree"
  // type questions
  int num_choices;
  // Colon separated list of choices for multiple choice/multiple select questions
  char *choices;
  // #72 unit field
  char *unit;
};
/* clang-format on */

/* clang-format off */
struct answer {

  /*
   * Answer values (writable by REST API)
   */

  // ID of question being answered
  char *uid;

  // #358 associated question type (writable by backend only, readable by backend and nextquestion controllers)
  int type;

  // Text response, including Y/N or 0-9/N response vectors for
  // multi-select and UUID
  // #212, swap text and value to align props with serialising order
  char *text;

  // Integer or fixed point response, also used for multi-choice
  long long value;

  // Fixed point latitude/longitude responses.
  long long lat,lon;

  // Time/date. There are two for time ranges
  // Units are milliseconds since 1800
  long long time_begin;
  long long time_end;
  // Timezone relative to GMT, in minutes
  // (places like South Australia have fractional hour time zones)
  int time_zone_delta;
  // Day Light Savings delta that was in force at the time
  // (typically either 0 or 60 minutes, but again, we won't assume so narrow
  // a view, as in some places and times DST has been 2 hours).
  int dst_delta;
  // #72 unit field
  char *unit;

  /*
   * Control fields (writable by backend only, readable by backend and nextquestion controllers)
   */

  // #186 flags
  int flags;

  // #162 answer storage timestamp
  long long stored;

#define ANSWER_DELETED 1
};
/* clang-format on */

enum answer_visibility {
  ANSWER_FIELDS_PUBLIC,
  ANSWER_FIELDS_PROTECTED,
};

// question and answer limits
// #363 introducing meta answers, decouple max values
#define MAX_QUESTIONS 8192
#define MAX_ANSWERS 8192

// #332 nextquestions data struct
#define MAX_NEXTQUESTIONS 1024
struct nextquestions {
    enum { STATUS_INFO, STATUS_WARN, STATUS_ERROR } status; // status flag, indcatiing how front end should handle message
    char *message;                                          // ad-hoc notifications, needs to be de-allocated
    struct question *next_questions[MAX_NEXTQUESTIONS];
    int question_count;
};

// #363 session meta
struct session_meta {
  char *user;
  char *group;
  char *authority;          // <ip>:<port> matched against env[SS_TRUSTED_MIDDLEWARE]
  enum {
    IDENDITY_CLI,           // shell
    IDENDITY_HTTP_PUBLIC,   // no auth, system is public
    IDENDITY_HTTP_BASIC,    // direct authentication via server (basic)
    IDENDITY_HTTP_DIGEST,   // direct authentication via server (digest)
    IDENDITY_HTTP_TRUSTED,  // trusted middleware handles authetication
    IDENDITY_UNKOWN,
  } provider;
};

// #379 session live cycle
enum session_state {
  SESSION_NULL, // init state, overwritten by load_session
  SESSION_NEW,
  SESSION_OPEN,
  SESSION_FINISHED,
  SESSION_CLOSED,
};

struct session {
  char *survey_id; // <survey name>/<hash>
  char *survey_description;
  char *session_id;

  // #184, add nextquestion provider mode flag
  unsigned int nextquestions_flag;
#define NEXTQUESTIONS_FLAG_GENERIC 1
#define NEXTQUESTIONS_FLAG_PYTHON 2

  // #363, update limit, offset for meta answers
  struct question *questions[MAX_QUESTIONS];
  struct answer *answers[MAX_ANSWERS];

  // #363, add offset for header answers
  int answer_offset;
  int answer_count;
  int question_count;

  // #379 session state, set on loading, updated during session actions, saved to session file if changed
  enum session_state state;
};

// #379 request actions (validated against session state)
enum actions {
  ACTION_TEST_ACCESS,
  ACTION_SESSION_NEW,
  ACTION_SESSION_DELETE,
  ACTION_SESSION_NEXTQUESTIONS,
  ACTION_SESSION_ADDANSWER,
  ACTION_SESSION_DELETEANSWER,
  ACTION_SESSION_ANALYSIS,
};

int generate_path(char *path_in, char *path_out, int max_len);
int generate_python_path(char *path_out, int max_len);

//#363
int is_given_answer(struct answer *a);
int is_system_answer(struct answer *a);

struct nextquestions *get_next_questions(struct session *s);
int get_analysis(struct session *s, const char **output);

int create_session(char *survey_id, char *session_id_out, struct session_meta *meta);
int delete_session(char *session_id);
struct session *load_session(char *session_id);
int save_session(struct session *s);
int session_add_answer(struct session *s, struct answer *a);
int session_delete_answer(struct session *s, struct answer *a,
                          int deleteFollowingP);
int session_delete_answers_by_question_uid(struct session *ses, char *uid,
                                           int deleteFollowingP);
int delete_session(char *session_id);
void free_session(struct session *s);
void free_question(struct question *q);
void free_answer(struct answer *a);

struct answer *copy_answer(struct answer *aa);
struct question *copy_question(struct question *qq);

int validate_session_id(char *session_id);
int validate_session_action(enum actions action, struct session *ses, char *msg, size_t sz); // #379
int session_add_datafile(char *session_id, char *filename_suffix, const char *data);
int lock_session(char *session_id);
int release_my_session_locks(void);

struct answer *session_get_header(char *uid, struct session *ses);

// #332 next_question struct
struct nextquestions *init_next_questions();
void free_next_questions(struct nextquestions *nq);

int dump_next_questions(FILE *f, struct nextquestions *nq);
int dump_session(FILE *f, struct session *ses);

// #363 session meta
void free_session_meta(struct session_meta *meta);
#endif
