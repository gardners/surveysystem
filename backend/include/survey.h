
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
  // Answer is an integer, bounded by min_value and max_value
#define QTYPE_INT         1
  // Answer is a fixed point value encoded as a 64-bit integer
  // and with decimal_places places after the decimal.
  // (we don't allow true floating point, as encoding differences can be a
  // pain, especially when compressing)
#define QTYPE_FIXEDPOINT  2
#define QTYPE_MULTICHOICE 3
#define QTYPE_MULTISELECT 4
#define QTYPE_LATLON      5
#define QTYPE_DATETIME    6
#define QTYPE_TIMERANGE   7
#define QTYPE_UPLOAD      8
#define QTYPE_TEXT        9
#define QTYPE_UUID        10
  
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
};

struct answer {
  // ID of question being answered
  char *uid;
  
  // Integer or fixed point response, also used for multi-choice
  long long value;

  // Text response, including Y/N or 0-9/N response vectors for
  // multi-select and UUID
  char *text;
  
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
  
};

#define MAX_QUESTIONS 8192
struct session {
  char *survey_id;   // <survey name>/<hash>
  char *survey_description;
  char *session_id;
  struct question *questions[MAX_QUESTIONS];
  struct answer *answers[MAX_QUESTIONS];
  int answer_count;
  int question_count;
};


int generate_path(char *path_in,char *path_out,int max_len);
int get_next_questions(struct session *s,
		       struct question *next_questions_out[],int max_next_questions,int *next_question_count_out);
int create_session(char *survey_id,char *session_id_out);
int delete_session(char *session_id);
struct session *load_session(char *session_id);
int save_session(struct session *s);
int session_add_answer(struct session *s,struct answer *a);
int session_delete_answer(struct session *s,struct answer *a);
int session_delete_answers_by_question_uid(struct session *ses,char *uid);
int delete_session(char *session_id);
void free_session(struct session *s);
void free_question(struct question *q);
void free_answer(struct answer *a);
