#ifndef __TEST_H__
#define __TEST_H__

#define TEST_MAX_LINE 8192
#define TEST_MAX_BUFFER 65536
#define TEST_MAX_LINE_COUNT 100

// test configuration
struct Test {
    int skip;                      // (optional)skip! header directive

    char name[1024];               // test name (derived from test file name)
    char description[TEST_MAX_LINE];    // (required) first line @description directive

    char file[1024];               // local test file path (./tests/%s)
    char dir[1024];                // test root dir (/tmp/%s)
    char log[1024];                // local testlog path (./testlogs/%s)

    char lighty_template[1024];    // (required) lighttpd.conf template
    char ligthy_userfile[1024];    // (optional) lighttpd user digest file
    char fcgienv_middleware[1024]; // (optional) @SS_TRUSTED_MIDDLEWARE %s directive, overwrites defaults

    int count;                     // number of scheduled tests
    int index;                     // current test
};

enum DiffResult {
  DIFF_MATCH,
  DIFF_MISMATCH,
  DIFF_MISMATCH_LENGTH,
  DIFF_MISMATCH_TOKEN,
};

// http
struct HttpResponse {
    int status;
    char contentType[1024];
    char eTag[1024];
    char lines[TEST_MAX_LINE_COUNT][TEST_MAX_LINE];
    int line_count;
};

////
// logs
////

int test_dump_logs(char *dir, FILE *log);

////
// lighttpd
////

void test_start_lighttpd(struct Test *test, char *pid_file, char *user, char *group, int server_port, int fcgi_port);
void test_stop_lighttpd(int verbose, int server_port, int fcgi_port);

////
// file system
////

int test_get_current_path(int argc, char **argv, char *path_out, size_t max_len);
int test_set_ownership(char *dir, char *user, char *group);
void test_require_file(char *path, int perm);
void test_require_directory(char *path, int perm);
int test_recursive_delete(const char *dir);

////
// time
////

long long test_gettime_us();
double test_time_delta(long long start_time);

////
// struct test
////

struct Test test_config_create(int server_port);
void test_config_print(FILE *in, struct Test *test);
FILE *test_load_test_file_header(char *test_file, struct Test *test);
void test_load_test_file(int test_count, int test_index, char *test_file, struct Test *test);

/////
// sessions
/////

int test_count_sessions(char *path);
int test_copy_session(char *session_id, char *targ, struct Test *test);


////
// test file parsing
////

int test_compile_session_definition(FILE *in, char *session_id, struct Test *test);
int test_compare_session(FILE *sess, int skip_s, FILE *comp, int skip_c, struct Test *test, int server_port, FILE *log, long long start_time);

////
// http
////

int test_parse_http_response(FILE *fp, struct HttpResponse *resp);

#endif
