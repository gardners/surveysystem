/**
 * Simple test runner for the survey system.
 * It runs tests that are each defined by a single file.
 * The test specification is as follows:
 *
 * @description <description> (required)
 * @skip! (optional, skip test)
 * @useproxy! (optional, start test with lighttpd for proxy auth)
 * @fcgienv_middleware <ip:port> (optional, set fastcgi env "SS_TRUSTED_MIDDLEWARE")
 *
 * definesurvey <name>
 * <questions in normal question format>
 * endofsurvey
 *
 * request <expected response code> <url path and query>
 * request proxy <expected response code> <url path and query> curlargs(`man curl`)
 *
 * extract_sessionid
 *
 * match <regex to search for in body of last response>
 *
 * nomatch <regex to search for in body of last response>
 *
 * verifysession
 * <expected set of answers in the session file. Can be empty>
 * endofsession
 *
 * session_add_answer <serialised answer>
 *
 * These commands can be used more than once, so that more complex activities can be scripted.
 *
 * Apart from running these scripts, all that it has to do to is to setup and cleanup the
 * directories for running the tests, and setup the config file for lighttpd for testing, and
 * actually stop and start lighttpd.
*/

#include <errno.h>
#include <fts.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <libgen.h>

#include "errorlog.h"
#include "survey.h"
#include "utils.h"

#define SERVER_PORT 8099
#define SURVEYFCGI_PORT 9009
#define LIGHTY_USER "www-data"
#define LIGHTY_GROUP "www-data"
#define LIGHTY_PIDFILE "/var/run/lighttpd.pid"

#define MAX_LINE 8192
#define MAX_BUFFER 65536

enum DiffResult {
  DIFF_MATCH,
  DIFF_MISMATCH,
  DIFF_MISMATCH_LENGTH,
  DIFF_MISMATCH_TOKEN,
};

// test configuration
struct Test {
    int skip;                      // (optional)skip! header directive

    char name[1024];               // test name (derived from test file name)
    char description[MAX_LINE];    // (required) first line @description directive

    char file[1024];               // local test file path (./tests/%s)
    char dir[1024];                // test roor dir (/tmp/%s)
    char log[1024];                // local testlog path (./testlogs/%s)

    char lighty_template[1024];    // (required) lighttpd.conf template
    char ligthy_userfile[1024];    // (optional) lighttpd user digest file
    char fcgienv_middleware[1024]; // (optional) @SS_TRUSTED_MIDDLEWARE %s directive, overwrites defaults

    int count;                     // number of scheduled tests
    int index;                     // current test
};

/**
 * initialise test config with default values
 */
struct Test create_test_config() {
  struct Test test = {
    .skip = 0,

    .name = "",
    .description = "",

    .file = "",
    .dir = "",

    .lighty_template =  "tests/config/lighttpd-default.conf.tpl",
    .ligthy_userfile = "",

    .count = 0,
    .index = 0,
  };

  snprintf(test.fcgienv_middleware, 1024, "127.0.0.1(%d)", SERVER_PORT);
  return test;
}

int tests = 0;

void init_lighttpd(struct Test *test);
void stop_lighttpd(int verbose);

char *py_traceback_func =
    "def cmodule_traceback(exc_type, exc_value, exc_tb):\n"
    "   import sys, traceback\n"
    "   lines = [];\n"
    "   lines = traceback.format_exception(exc_type, exc_value, exc_tb)\n"
    "   output = '\\n'.join(lines)\n"
    "   return output\n"
    "\n";

int fix_ownership(char *dir) {
  int retVal = 0;
  do {
    char cmd[MAX_LINE];
    snprintf(cmd, MAX_LINE, "sudo chown -R %s:%s %s\n", LIGHTY_USER, LIGHTY_GROUP,
             dir);
    system(cmd);
  } while (0);
  return retVal;
}

void require_file(char *path, int perm) {
  FILE *f = fopen(path, "w");
  if (!f) {
    fprintf(stderr, "require_file() fopen(%s) failed.", path);
    exit(-3);
  }
  fclose(f);

  if (chmod(path, perm)) {
    fprintf(stderr, "require_file() chmod(%s) failed.", path);
    exit(-3);
  }
}

void require_directory(char *path, int perm) {
  if (mkdir(path, perm)) {
    if (errno != EEXIST) {
      fprintf(stderr, "mkpath(%s, %o) failed.", path, perm);
      exit(-3);
    }
  }

  // bypassing umask
  if (chmod(path, perm)) {
    fprintf(stderr, "require_directory() chmod(%s) failed.", path);
    exit(-3);
  }
}

// From https://stackoverflow.com/questions/2256945/removing-a-non-empty-directory-programmatically-in-c-or-c
int recursive_delete(const char *dir) {
  int ret = 0;
  FTS *ftsp;
  FTSENT *curr;

  // Cast needed (in C) because fts_open() takes a "char * const *", instead
  // of a "const char * const *", which is only allowed in C++. fts_open()
  // does not modify the argument.
  char *files[] = {(char *)dir, NULL};

  // FTS_NOCHDIR  - Avoid changing cwd, which could cause unexpected behavior
  //                in multithreaded programs
  // FTS_PHYSICAL - Don't follow symlinks. Prevents deletion of files outside
  //                of the specified directory
  // FTS_XDEV     - Don't cross filesystem boundaries
  ftsp = fts_open(files, FTS_NOCHDIR | FTS_PHYSICAL | FTS_XDEV, NULL);
  if (!ftsp) {
    fprintf(stderr, "%s: fts_open failed: %s\n", dir, strerror(errno));
    ret = -1;
    goto finish;
  }

  while ((curr = fts_read(ftsp))) {
    switch (curr->fts_info) {
    case FTS_NS:
    case FTS_DNR:
    case FTS_ERR:
      if (curr->fts_errno != ENOENT) {
        fprintf(stderr,
                "recursive_delete('%s') encountered a problem: fts_read error: "
                "%s\n",
                curr->fts_accpath, strerror(curr->fts_errno));
      }
      break;

    case FTS_DC:
    case FTS_DOT:
    case FTS_NSOK:
      // Not reached unless FTS_LOGICAL, FTS_SEEDOT, or FTS_NOSTAT were
      // passed to fts_open()
      break;

    case FTS_D:
      // Do nothing. Need depth-first search, so directories are deleted
      // in FTS_DP
      break;

    case FTS_DP:
    case FTS_F:
    case FTS_SL:
    case FTS_SLNONE:
    case FTS_DEFAULT:
      if (remove(curr->fts_accpath) < 0) {
        fprintf(stderr, "%s: Failed to remove: %s\n", curr->fts_path,
                strerror(errno));
        ret = -1;
      }
      break;
    }
  }

finish:
  if (ftsp) {
    fts_close(ftsp);
  }

  return ret;
}

long long gettime_us() {
  long long retVal = -1;

  do {
    struct timeval nowtv;
    // If gettimeofday() fails or returns an invalid value, all else is lost!
    if (gettimeofday(&nowtv, NULL) == -1) {
      fprintf(stderr, "\nFATAL: gettimeofday returned -1");
      exit(-3);
    }

    if (nowtv.tv_sec < 0 || nowtv.tv_usec < 0 || nowtv.tv_usec >= 1000000) {
      fprintf(stderr, "\nFATAL: gettimeofday returned invalid value");
      exit(-3);
    }

    retVal = nowtv.tv_sec * 1000000LL + nowtv.tv_usec;
  } while (0);

  return retVal;
}

int dump_logs(char *dir, FILE *log) {

  int retVal = 0;

  do {
    fprintf(log, "============================================================="
                 "===========\n");
    fprintf(log, "Backend server logs follow.\n");
    fprintf(log, "============================================================="
                 "===========\n");

    {
      char breakage_log[16384];
      // XXX breakage.log exists outside of the logs directory. It should be moved in there.
      // XXX We don't ignore previous content of the breakage log.  We should do that.
      snprintf(breakage_log, 16384, "%s/../breakage.log", dir);
      FILE *f = fopen(breakage_log, "r");
      if (f) {
        fprintf(log, "--------- %s ----------\n", breakage_log);
        char line[4096];
        line[0] = 0;
        fgets(line, 4096, f);

        while (line[0]) {
          fprintf(log, "%s", line);
          line[0] = 0;
          fgets(line, 4096, f);
        }
        fclose(f);
      } else {
        fprintf(log,
                "WARNING: Could not open breakage log file '%s' for reading.\n",
                breakage_log);
      }
    }
    fprintf(log, "============================================================="
                 "===========\n");

    int ret = 0;
    FTS *ftsp;
    FTSENT *curr;

    // Cast needed (in C) because fts_open() takes a "char * const *", instead
    // of a "const char * const *", which is only allowed in C++. fts_open()
    // does not modify the argument.
    char *files[] = {(char *)dir, NULL};

    // FTS_NOCHDIR  - Avoid changing cwd, which could cause unexpected behavior
    //                in multithreaded programs
    // FTS_PHYSICAL - Don't follow symlinks. Prevents deletion of files outside
    //                of the specified directory
    // FTS_XDEV     - Don't cross filesystem boundaries
    ftsp = fts_open(files, FTS_NOCHDIR | FTS_PHYSICAL | FTS_XDEV, NULL);
    if (!ftsp) {
      fprintf(log, "%s: fts_open failed: %s\n", dir, strerror(errno));
      ret = -1;
      goto finish;
    }

    while ((curr = fts_read(ftsp))) {
      switch (curr->fts_info) {
      case FTS_NS:
      case FTS_DNR:
      case FTS_ERR:
        fprintf(log, "%s: fts_read error: %s\n", curr->fts_accpath,
                strerror(curr->fts_errno));
        break;

      case FTS_F:
      case FTS_SL: {
        FILE *in = fopen(curr->fts_accpath, "r");

        if (!in) {
          fprintf(log, "ERROR: Could not read log file '%s'\n",
                  curr->fts_accpath);
        } else {
          fprintf(log, "--------- %s ----------\n", curr->fts_accpath);
          char line[MAX_LINE];
          line[0] = 0;
          fgets(line, MAX_LINE, in);

          while (line[0]) {
            fprintf(log, "%s", line);
            line[0] = 0;
            fgets(line, MAX_LINE, in);
          }
          fclose(in);
        }

      } break;
      }
    }

  finish:
    if (ftsp) {
      fts_close(ftsp);
    }

  } while (0);

  return retVal;
}

void print_test_config(FILE *in, struct Test *test) {
  if(!test) {
    fprintf(in, "struct *test is NULL\n");
    return;
  }

  fprintf(
    in,
    "struct *test\n"
    "    |_ skip: %d\n"
    "    |_ name: '%s'\n"
    "    |_ description: '%s'\n"
    "    |_ file: '%s'\n"
    "    |_ dir: '%s'\n"
    "    |_ log: '%s'\n"
    "    |_ lighty_template: '%s'\n"
    "    |_ ligthy_userfile: '%s'\n"
    "    |_ fcgienv_middleware: '%s'\n"
    "    |_ count: %d\n"
    "    |_ index: %d\n",
    test->skip,
    test->name,
    test->description,
    test->file,
    test->dir,
    test->log,
    test->lighty_template,
    test->ligthy_userfile,
    test->fcgienv_middleware,
    test->count,
    test->index
  );
}

/**
 * parses a request directive line for defined patterns and creates a curl command
 */
int parse_request(char *line, char *out, int *expected_http_status, char *last_sessionid, char *dir, FILE *log) {
    int retVal = 0;

    do {
        char tmp[MAX_BUFFER];

        char args[MAX_BUFFER];
        char url[MAX_BUFFER];
        char data[MAX_BUFFER] = {'\0'};
        char method[MAX_BUFFER] = {'\0'};
        char curl_args[MAX_BUFFER]= {'\0'};
        char url_sub[MAX_BUFFER];

        // flags
        int done = 0;
        int proxy =0;

        // parse out arguments
        if (sscanf(line, "request %[^\r\n]", args) != 1) {
            fprintf(log, "FATAL: invalid directive '%s'", line);
            retVal = -1;
            break;
        }

        // check if proxy
        // example: request proxy 200 addanswer curlargs(--user name:password) POST "sessionid=$SESSION&answer=question1:Hello+World:0:0:0:0:0:0:0:"
        if (sscanf(args, "proxy %[^\r\n]", tmp) == 1) {
            proxy = 1;
            strncpy(args, tmp, MAX_BUFFER);
            tmp[0] = 0;
        }

        // with code + url + curlargs + method + data
        // example: request 200 addanswer curlargs(--user name:password) POST "sessionid=$SESSION&answer=question1:Hello+World:0:0:0:0:0:0:0:"
        if(!done && sscanf(args, "%d %s curlargs(%[^)]) %s %s", expected_http_status, url, curl_args, method, data) == 5) {
            done = 1;
        }

        // with code + url + curlargs
        // example: request 200 newsession curlargs(--user name:password)
        if(!done && sscanf(args, "%d %s curlargs(%[^)])", expected_http_status, url, curl_args) == 3) {
            done = 1;
        }

        // with code + url + optional( method + data)
        // example: request 200 newsession?surveyid=mysurvey
        if(!done && sscanf(args, "%d %s %s %s", expected_http_status, url, method, data) == 4) {
            done = 1;
        }

        int o = 0;
        for (int i = 0; url[i]; i++) {

          if (url[i] != '$') {
            url_sub[o++] = url[i];
          } else {

            // $$ substitutes for $
            if (url[i + 1] == '\"') {
              // Escape quotes
              url_sub[o++] = '\\';
              url_sub[o++] = '\"';
            } else if (url[i + 1] == '$') {
              url_sub[o++] = '$';
            } else if (!strncmp("$SESSION", &url[i], 8)) {
              snprintf(&url_sub[o], MAX_BUFFER - o, "%s", last_sessionid);
              o = strlen(url_sub);
              i += 7;
            } else {
              fprintf(log, "FATAL: Unknown $ substitution in URL");
              retVal = -1;
            } // endif

          } // endif

        } // endfor

        if(retVal) {
            break;
        }

        url_sub[o] = 0;

        if (strlen(data)) {
          strncpy(tmp, data, MAX_BUFFER);
          snprintf(data, MAX_BUFFER, " -d %s", tmp);
        }

        if (strlen(method)) {
          strncpy(tmp, method, MAX_BUFFER);
          snprintf(method, MAX_BUFFER, " -X %s", tmp);
        }

        if (strlen(curl_args)) {
          strncpy(tmp, curl_args, MAX_BUFFER);
          snprintf(curl_args, MAX_BUFFER, " %s", tmp);
        }

        // build curl cmd
        snprintf(
          out, MAX_BUFFER,
          "curl%s%s%s -s -w \"HTTPRESULT=%%{http_code}\" -o "
          "%s/request.out \"http://localhost:%d/%s/%s\" > "
          "%s/request.code",
          curl_args,
          method,
          data,
          dir,
          SERVER_PORT,
          "surveyapi",
          url_sub,
          dir
        );
    } while(0);

    return retVal;
}

/**
 * creates a copy of a given session and opens it for inspection
 */
FILE *open_session_file_copy(char *session_id, struct Test *test) {

  // Build path to session file
  char session_file[1024];

  snprintf(session_file, 1024, "%s/sessions/%c%c%c%c/%s", test->dir,
            session_id[0], session_id[1], session_id[2],
            session_id[3], session_id);

  char tmp[MAX_LINE];
  snprintf(tmp, MAX_LINE, "%s/session.cpy", test->dir);
  unlink(tmp);

  snprintf(tmp, MAX_LINE, "sudo cp %s %s/session.cpy", session_file, test->dir);
  if(system(tmp)) {
    fprintf(stderr, "Command failed: '%s'\n", tmp);
    return NULL;
  }

  // Check that the file exists
  snprintf(tmp, MAX_LINE, "%s/session.cpy", test->dir);
  FILE *copy = fopen(tmp, "r");
  if (!copy) {
    fprintf(stderr, "Could not open session file '%s': %s\n", tmp, strerror(errno));
    return NULL;
  }

  return copy;
}

/**
 * Replace pattern with string.
 */
void replace_str(char *str, char *pattern, char *replacement, size_t sz) {
    char *pos;
    char tmp[sz];
    int index = 0;
    int plen;

    plen = strlen(pattern);

    while ((pos = strstr(str, pattern)) != NULL) {
        strcpy(tmp, str);
        index = pos - str;
        str[index] = '\0';
        strcat(str, replacement);

        // cat str with remainder
        strcat(str, tmp + index + plen);
    }
}

/**
 * Replace pattern with int.
 */
void replace_int(char *str, char *pattern, int replacement, size_t sz) {
  char tmp[sz];
  snprintf(tmp, sz, "%d", replacement);
  replace_str(str, pattern, tmp, sz);
}

int define_session(FILE *in, char *session_id, struct Test *test) {
  char session_dir[1024];
  char session_file[1024];
  char line[MAX_LINE];

  // copen session file
  snprintf(session_dir, 1024, "%s/sessions/%c%c%c%c", test->dir,
            session_id[0], session_id[1], session_id[2],
            session_id[3]);

  mkdir(session_dir, 0755);

  if (chmod(session_dir, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH)) {
    fprintf(stderr, "\nERROR: chmod() failed for new session directory %s", session_dir);
    return -1;
  }

  snprintf(session_file, 1024, "%s/%s", session_dir, session_id);

  FILE *s = fopen(session_file, "w");
  if (!s) {
    fprintf(stderr,
            "\rERROR: Could not create survey session file '%s'                  "
            "                                             \n",
            session_file);
    return -1;
  }

  // now read definition into session
  line[0] = 0;
  fgets(line, MAX_LINE, in);

  while (line[0]) {
    trim_crlf(line);

    replace_int(line, "<UTIME>", (int)time(0), MAX_LINE);
    replace_str(line, "<FCGIENV_MIDDLEWARE>", test->fcgienv_middleware, MAX_LINE);
    replace_int(line, "<IDENDITY_CLI>", IDENDITY_CLI, MAX_LINE);
    replace_int(line, "<IDENDITY_HTTP_PUBLIC>", IDENDITY_HTTP_PUBLIC, MAX_LINE);
    replace_int(line, "<IDENDITY_HTTP_BASIC>", IDENDITY_HTTP_BASIC, MAX_LINE);
    replace_int(line, "<IDENDITY_HTTP_DIGEST>", IDENDITY_HTTP_DIGEST, MAX_LINE);
    replace_int(line, "<IDENDITY_HTTP_TRUSTED>", IDENDITY_HTTP_TRUSTED, MAX_LINE);
    replace_int(line, "<IDENDITY_UNKOWN>", IDENDITY_UNKOWN, MAX_LINE);

    if (!strcmp(line, "endofsession")) {
      break;
    }

    fprintf(s, "%s\n", line);
    line[0] = 0;
    fgets(line, MAX_LINE, in);
  }

  fclose(s);
  return 0;
}

/**
 * Compares survey session line with a comparsion string
 * - the number of delimitors (Note, this includes escaped delimiters in fields, i.e "\:")
 * - string matches between delimitors
 * - custom validation via <KEYWORDS>
 * Note that this function is not very practical in terms of deserializing lines due to ignoring escaped delimiters
 *
 * Return values:
 *    0: OK, columns match
 *   -1: text compare error
 *   -2: session line too short (column count)
 *   -3: session line too long (column count)
 *   -4: session field <UTIME> is invalid (must be within the past hour from now)
 */
enum DiffResult compare_session_line(char *session_line, char *comparison_line, struct Test *test, int *row_count) {

  char left_line[MAX_LINE];
  char right_line[MAX_LINE];

  char *left;
  char *right;
  char *left_ptr;
  char *right_ptr;

  (*row_count) = 0;
  int pass;

  // do not mutate arg *strings
  strncpy(left_line, session_line, MAX_LINE);
  strncpy(right_line, comparison_line, MAX_LINE);

  left = strtok_r(left_line, ":", &left_ptr);
  right = strtok_r(right_line, ":", &right_ptr);

  while (right) {
    pass = 0;

    // match number of columns: session line too short
    if (!left) {
      return DIFF_MISMATCH_LENGTH;
    }

    // validate <UTIME> keyword (unix timestamp)
    if (!strcmp(right, "<UTIME>")) {
      int now = (int)time(NULL);
      int then = atoi(left);

      if (now <= 0 || now < then || now - then > 8600) {
        return DIFF_MISMATCH_TOKEN;
      }
      pass++;
    }

    // validate <IDENDITY_CLI>
    if (!strcmp(right, "<IDENDITY_CLI>")) {
      int val = atoi(left);

      if (val != IDENDITY_CLI) {
        return DIFF_MISMATCH_TOKEN;
      }
      pass++;
    }

    // validate <IDENDITY_HTTP_PUBLIC>
    if (!strcmp(right, "<IDENDITY_HTTP_PUBLIC>")) {
      int val = atoi(left);

      if (val != IDENDITY_HTTP_PUBLIC) {
        return DIFF_MISMATCH_TOKEN;
      }
      pass++;
    }

    // validate <IDENDITY_HTTP_BASIC>
    if (!strcmp(right, "<IDENDITY_HTTP_BASIC>")) {
      int val = atoi(left);

      if (val != IDENDITY_HTTP_BASIC) {
        return DIFF_MISMATCH_TOKEN;
      }
      pass++;
    }

    // validate <IDENDITY_HTTP_DIGEST>
    if (!strcmp(right, "<IDENDITY_HTTP_DIGEST>")) {
      int val = atoi(left);

      if (val != IDENDITY_HTTP_DIGEST) {
        return DIFF_MISMATCH_TOKEN;
      }
      pass++;
    }

    // validate <IDENDITY_HTTP_TRUSTED>
    if (!strcmp(right, "<IDENDITY_HTTP_TRUSTED>")) {
      int val = atoi(left);

      if (val != IDENDITY_HTTP_TRUSTED) {
        return DIFF_MISMATCH_TOKEN;
      }
      pass++;
    }

    // validate <IDENDITY_UNKOWN>
    if (!strcmp(right, "<IDENDITY_UNKOWN>")) {
      int val = atoi(left);

      if (val != IDENDITY_UNKOWN) {
        return DIFF_MISMATCH_TOKEN;
      }
      pass++;
    }

    // validate <AUTHORITY_NONE>, default authority: remote_ip(remote_port)
    if (!strcmp(right, "<AUTHORITY_NONE>")) {
      char tmp[16];
      snprintf(tmp, 16, "127.0.0.1(%d)", SERVER_PORT);
      if (strcmp(left, tmp)) {
        return DIFF_MISMATCH_TOKEN;
      }
      pass++;
    }

    // validate <UTIME> keyword (unix timestamp)
    if (!strcmp(right, "<FCGIENV_MIDDLEWARE>")) {
      if (strcmp(left, test->fcgienv_middleware)) {
        return DIFF_MISMATCH_TOKEN;
      }
      pass++;
    }

    // compare column text
    if(!pass) {
      if (strcmp(left, right)) {
        return DIFF_MISMATCH;
      }
    }

    left = strtok_r(NULL, ":", &left_ptr);
    right = strtok_r(NULL, ":", &right_ptr);
    (*row_count)++;
  }

  // match number of columns: session line too long
  if (left) {
    return DIFF_MISMATCH_LENGTH;
  }

  return DIFF_MATCH;
}

/**
 * All file pointers are left open and have to be closed externally
 */
int compare_session(FILE *sess, int skip_s, FILE *comp, int skip_c, struct Test *test, FILE *log, long long start_time) {
  int retVal = 0;
  double tdelta;

  // Now go through reading lines from here and from the session file
  char session_line[MAX_LINE];
  char comparison_line[MAX_LINE];

  int c_count = 0; // comparsion line count
  int s_count = 0; // session line count
  int diff = DIFF_MATCH; // diff result

  char *s;
  char *c;

  // wind both files forward
  while (c_count < skip_c) {
    c = fgets(comparison_line, MAX_LINE, comp);
    c_count++;
  }

  while (s_count < skip_s) {
    s = fgets(session_line, MAX_LINE, sess);
    s_count++;
  }

  while (1) {
    // reset both containers, we need this for checks below
    session_line[0] = 0;
    comparison_line[0] = 0;

    tdelta = gettime_us() - start_time;
    tdelta /= 1000;

    // read left and right
    s = fgets(session_line, MAX_LINE, sess);
    c = fgets(comparison_line, MAX_LINE, comp);

    s_count++;
    c_count++;

    trim_crlf(comparison_line);
    trim_crlf(session_line);

    // End of session file before end of comparison list
    // End of comparison list before end of session file

    fprintf(log, "<<< %s\n", comparison_line);
    fprintf(log, ">>> %s\n", session_line);

    // end comparasion
    if (!strcmp(comparison_line, "endofsession")) {
      fprintf(log, "<<<<<< END OF EXPECTED SESSION DATA\n");
      if (s != NULL) {
        fprintf(log,"  [SESSION_TOO_LONG] comparsion 'endofsession' at line %d, but session file has content at line %d: '%s'\n", c_count, s_count, session_line);
      }
      break;
    }

    // no empty lines allowed
    if (!strlen(comparison_line)) {
      fprintf(log,"  [EMPTY_LINE_COMPARSION] line %d of comparsion is empty.\n", c_count);
      retVal++;
      // TODO check if eof session
      break;
    }

    if (!strlen(session_line)) {
      if (s == NULL) {
        fprintf(log,"  [SESSION_TOO_SHORT] comparsion content at line %d, but session file ended before: '%s'\n", c_count,comparison_line);
      } else {
        fprintf(log,"  [EMPTY_LINE_SESSION] line %d of session file is empty.\n", s_count);
      }
      retVal++;
      // TODO check if eof session
      break;
    }

    // compare session line, we are relying on the string terminations set above
    int row_count = 0;
    diff = compare_session_line(session_line, comparison_line, test, &row_count);
    // fprintf(stderr, "\n%d: line %d(%d), '%s' => '%s'", diff, s_count, c_count, session_line, comparison_line);

    switch (diff) {
      case DIFF_MATCH:
        // ok, do nothing
        break;

      case DIFF_MISMATCH:
        fprintf(log,
          "  [DIFF_MISMATCH] compare_session_line() field mismatch (session line %d row %d, comparsion line %d, code %d)\n", s_count, row_count, c_count, diff);
        break;

      case DIFF_MISMATCH_LENGTH:
        fprintf(log,
          "  [DIFF_MISMATCH_LENGTH] compare_session_line() session line number of fields don't match (session line %d row %d, comparsion line %d, code %d)\n", s_count, row_count, c_count, diff);
        break;

      case DIFF_MISMATCH_TOKEN:
        fprintf(log,
          "  [DIFF_MISMATCH_TOKEN] compare_session_line() <TOKEN> ivalidation failed (session line %d row %d, comparsion line %d, code %d)\n", s_count, row_count, c_count, diff);
        break;

      default:
        fprintf(log,
          "  [UNKWOWN MISMATCH] compare_session_line() unknown return (session line %d row %d, comparsion line %d, code %d)\n", s_count, row_count, c_count, diff);
    } // endswitch

    // register error state
    if (diff > DIFF_MATCH) {
      retVal++;
    }
  } // end while 1

  tdelta = gettime_us() - start_time;
  tdelta /= 1000;

  if (retVal) {
    fprintf(log, "T+%4.3fms : FAIL : verifysession with %d errors.\n", tdelta, retVal);
    return -1;
  }

  fprintf(log, "T+%4.3fms : OK : session file matches comparsion: %d session lines read, %d lines compared.\n", tdelta, s_count, c_count);
  return 0;
}


FILE *load_test_header(char *test_file, struct Test *test) {
    FILE *fp = fopen(test_file, "r");
    if (!fp) {
      fprintf(stderr, "\nCould not open test file '%s' for reading\n", test_file);
      fclose(fp);
      return NULL;
    }

    // Get name of test file without path
    char *name = strrchr(test_file, '/');
    name++;
    strncpy(test->name, name, sizeof(test->name));

    // fist line is description
    char line[MAX_LINE];
    fgets(line, MAX_LINE, fp);
    if (sscanf(line, "description %[^\r\n]", test->description) != 1) {
      fprintf(stderr, "\nCould not parse description line of test.\n");
      fclose(fp);
      return NULL;
    }

    while(fgets(line, MAX_LINE, fp) != NULL) {
        // end of config header
        if (line[0] == '\n') {
          break;
        }

        if (strncmp(line, "@skip!", 6) == 0) {
            test->skip = 1;
        }
    }

    if (feof(fp)) {
      fprintf(stderr, "Error: File '%s' reached EOF before header parsing finished. Did you forget to separate the header with an empty line?\n", test_file);
      fclose(fp);
      return NULL;
    }

    return fp;
}

void init_parse_test_config(int test_count, int test_index, char *test_file, struct Test *test) {
    FILE *fp = fopen(test_file, "r");
    if (!fp) {
      fprintf(stderr, "\nCould not open test file '%s' for reading\n", test_file);
      fclose(fp);
      exit(-3);
    }

    char tmp[1024];

    // set test->file
    test->count = test_count;
    test->index = test_index;
    strncpy(test->file, test_file, 1024);

    // set test->name
    char *name = strrchr(test_file, '/');
    name++;
    strncpy(test->name, name, 1024);

    // set test->dir
    snprintf(test->dir, 1024, "/tmp/surveytestrunner.%s", name);

    // set test->log
    snprintf(test->log, 1024, "testlog/%s.log", name);

    // fist line is description
    char line[MAX_LINE];
    fgets(line, MAX_LINE, fp);
    if (sscanf(line, "@description %[^\r\n]", test->description) != 1) {
      fprintf(stderr, "\nCould not parse description line of test->\n");
      exit(-3);
    }

    while(fgets(line, MAX_LINE, fp) != NULL) {

      if (line[0] != '@') {
        break; // end of config header
      }

      if (strncmp(line, "@skip!", 6) == 0) {
          test->skip = 1;
      }

      if (strncmp(line, "@useproxy!", 10) == 0) {
          strncpy(test->lighty_template, "tests/config/lighttpd-proxy.conf.tpl", 1024);
          strncpy(test->ligthy_userfile, "tests/config/lighttpd.user", 1024);
      }

      if (sscanf(line, "@fcgienv_middleware %[^\r\n]", tmp) == 1) {
        strncpy(test->fcgienv_middleware, tmp, 1024);
      }

    }

    if (feof(fp)) {
      fprintf(stderr, "Error: File '%s' reached EOF before header parsing finished. Did you forget to separate the header with an empty line?\n", test_file);
      fclose(fp);
      exit(-3);
    }

    if(fp) {
      fclose(fp);
    }
}

void init_test_filesystem(struct Test *test) {

  // remove old test records
  if (!access(test->dir, F_OK)) {
    if(recursive_delete(test->dir)) {
      fprintf(stderr, "FATAL: cannot remove old test dir '%s'\n", test->dir);
      exit(-3);
    }
  }

  // create test root
  require_directory(test->dir, 0755);

  // Make surveys, sessions and lock directories
  // Also make sure the survey directory is writable when running tests
  // (this is so accesstest will succeed.  For production, this can be
  // avoided by making sure to use the commandline tool to create a single
  // session in a survey after the survey has been modified, to make sure
  // all the necessary hash files get created.)
  char path[2048];

  snprintf(path, 2048, "%s/surveys", test->dir);
  require_directory(path, 0777);

  snprintf(path, 2048, "%s/sessions", test->dir);
  require_directory(path, 0777);

  snprintf(path, 2048, "%s/logs", test->dir);
  require_directory(path, 0777);

  snprintf(path, 2048, "%s/locks", test->dir);
  require_directory(path, 0777);

  snprintf(path, 2048, "%s/lighttpd-access.log", test->dir);
  require_file(path, 0777);

  snprintf(path, 2048, "%s/lighttpd-error.log", test->dir);
  require_file(path, 0777);

  snprintf(path, 2048, "%s/breakage.log", test->dir);
  require_file(path, 0777);

  // point python dir ALWAYS to test dir
  snprintf(path, 2048, "%s/python", test->dir);
  require_directory(path, 0775);

  // create docroot for lighttpd, we do not use it, but it is required by lighttpd
  snprintf(path, 2048, "%s/www", test->dir);
  require_directory(path, 0775);
}

int run_test(struct Test *test) {
  // #198 flush errors accumulated by previous tests
  clear_errors();

  int retVal = 0;
  FILE *in = NULL;
  FILE *log = NULL;
  char line[MAX_LINE];
  char log_dir[1024];

  int response_line_count = 0;
  char response_lines[100][MAX_LINE];
  char last_sessionid[100] = "";

  do {
    // skip test if flag was set
    if (test->skip) {
      goto skip;
    }

    snprintf(log_dir, 1024,"%s/logs", test->dir);

    in = fopen(test->file, "r");
    if (!in) {
      fprintf(stderr, "\n[Fatal]: Loading test header for '%s' failed.\n", test->file);
      exit(-3);
    }

    log = fopen(test->log, "w");
    if (!log) {
      fprintf(stderr,
              "\rFATAL: Could not create test log file '%s' for test '%s': %s  "
              "                                  \n",
              test->log, test->file, strerror(errno));
      goto error;
    }

    fprintf(log, "--------- test config ----------\n");
    print_test_config(log, test);
    fprintf(log, "\n");

    // starting test
    fprintf(stderr, "\033[39m[    ]  \033[37m%s : %s\033[39m", test->name, test->description);
    fflush(stderr);

    time_t now = time(0);
    char *ctime_str = ctime(&now);
    fprintf(log, "Started running test at %s", ctime_str);
    long long start_time = gettime_us();

    char surveyname[1024] = "";
    char custom_sessionid[1024] = "";
    char glob[MAX_BUFFER];
    double tdelta;

    // Variables for FOR NEXT loops
    char var[1024];
    char arg[1024]; // another temp storage for scanf
    int first, last;
    int for_count = 0;
    char for_var[10][16];
    int for_prevval[10];
    int for_last[10];
    int for_step[10];
    off_t for_seek_pos[10];

    // Now iterate through test script
    line[0] = 0;
    fgets(line, MAX_LINE, in);

    while (line[0]) {

      int len = strlen(line);
      // Trim CR/LF from the end of the line
      while (len && (line[len - 1] < ' ')) {
        line[--len] = 0;
      }

      tdelta = gettime_us() - start_time;
      tdelta /= 1000;
      if (line[0] && line[0] != '@' && line[0] != '#') {
        fprintf(log, "T+%4.3fms : Executing directive '%s'\n", tdelta, line);
      }

      ////
      // keywords: START
      ////

      if (sscanf(line, "definesurvey %[^\r\n]", surveyname) == 1) {

        ////
        // keyword: "definesurvey"
        ////

        // Read survey definition and create survey file
        char survey_file[1024];

        // Trim trailing white space from survey name to avoid annoying mismatches
        while (surveyname[0] && surveyname[strlen(surveyname) - 1] == ' ') {
          surveyname[strlen(surveyname) - 1] = 0;
        }

        snprintf(survey_file, 1024, "%s/surveys/%s", test->dir, surveyname);
        mkdir(survey_file, 0777);
        if (chmod(survey_file, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP |
                                   S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH)) {
          fprintf(stderr, "\nERROR: chmod() failed for new survey directory %s",
                  survey_file);
          goto error;
        }

        snprintf(survey_file, 1024, "%s/surveys/%s/current", test->dir, surveyname);
        FILE *s = fopen(survey_file, "w");
        if (!s) {
          fprintf(stderr,
                  "\rERROR: Could not create survey file '%s'                  "
                  "                                             \n",
                  survey_file);
          goto error;
        }

        line[0] = 0;
        fgets(line, MAX_LINE, in);

        while (line[0]) {
          int len = strlen(line);
          // Trim CR/LF from the end of the line
          while (len && (line[len - 1] < ' ')) {
            line[--len] = 0;
          }

          if (!strcmp(line, "endofsurvey")) {
            break;
          }
          fprintf(s, "%s\n", line);
          line[0] = 0;
          fgets(line, MAX_LINE, in);
        }

        fclose(s);
      } else if (sscanf(line, "definesession %[^\r\n]", custom_sessionid) == 1) {

        ////
        // keyword: "definesession"
        ////

        // register session id
        trim_crlf(custom_sessionid);
        strcpy(last_sessionid, custom_sessionid);

        if(define_session(in, custom_sessionid, test)) {
          fprintf(stderr, "\nERROR: define_session() failed for custom sessionid %s", custom_sessionid);
          goto error;
        }

      } else if (sscanf(line, "definesession %[^\r\n]", custom_sessionid) == 1) {

      }else if (!strcmp(line, "python")) {

        ////
        // keyword: "python"
        ////

        // create python directory
        char python_dir[1024];
        snprintf(python_dir, 1024, "%s/python", test->dir);
        mkdir(python_dir, 0755);

        if (chmod(python_dir, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP |
                                  S_IROTH | S_IXOTH)) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log,
                  "T+%4.3fms : ERROR : Could not set permissions on python "
                  "directory '%s'\n",
                  tdelta, python_dir);
          goto error;
        }

        // create python package (__ini.py__)
        char python_ini[2048];
        snprintf(python_ini, 2048, "%s/__init__.py", python_dir);

        FILE *s = fopen(python_ini, "w");
        if (!s) {
          fprintf(log,
                  "T+%4.3fms : ERROR : Could not create python file '%s'\n",
                  tdelta, python_ini);
          goto error;
        }
        fclose(s);

        if (chmod(python_ini, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP |
                                  S_IROTH | S_IXOTH)) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log,
                  "T+%4.3fms : ERROR : Could not set permissions on python "
                  "file '%s'\n",
                  tdelta, python_ini);
          goto error;
        }

        // create python module (nextquestion.py)
        char python_module[2048];
        snprintf(python_module, 2048, "%s/nextquestion.py", python_dir);
        s = fopen(python_module, "w");
        if (!s) {
          fprintf(log,
                  "T+%4.3fms : ERROR : Could not create python file '%s'\n",
                  tdelta, python_module);
          goto error;
        }

        fprintf(s, "%s\n", py_traceback_func);

        // write python content from testfile
        line[0] = 0;
        fgets(line, MAX_LINE, in);
        while (line[0]) {
          int len = strlen(line);
          // Trim CR/LF from the end of the line
          while (len && (line[len - 1] < ' ')) {
            line[--len] = 0;
          }

          if (!strcmp(line, "endofpython")) {
            break;
          }
          fprintf(s, "%s\n", line);

          line[0] = 0;
          fgets(line, MAX_LINE, in);
        } // endwhile
        fclose(s);

        if (chmod(python_module, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP |
                                     S_IXGRP | S_IROTH | S_IXOTH)) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log,
                  "T+%4.3fms : ERROR : Could not set permissions on python "
                  "file '%s'\n",
                  tdelta, python_module);
          goto error;
        }

        // Compile the python
        char cmd[1024];
        snprintf(cmd, 1024, "python3.7 -m compileall %s 2>&1 >>%s",
                 python_module, test->log);
        int compile_result = system(cmd);

        if (compile_result) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log,
                  "T+%4.3fms : FATAL: Failed to compile python module. Does "
                  "the python have errors?\n",
                  tdelta);
          goto fatal;
        }

        // Then restart, to clear out any old python code we had loaded before
        tdelta = gettime_us() - start_time;
        tdelta /= 1000;
        fprintf(log,
                "T+%4.3fms : INFO : Restarting backend to clear loaded python "
                "code\n",
                tdelta);

        // #361, removed extra call to configure_and_start_lighttpd()
        //  give filesystem and python fs time to cope with newly created python file,
        //  we had random ModuleNotFoundErrors on tests with both no python and python
        sleep(1);

        tdelta = gettime_us() - start_time;
        tdelta /= 1000;
        fprintf(log, "T+%4.3fms : INFO : Backend restart complete\n", tdelta);

      } else if (!strcmp(line, "extract_sessionid")) {

        ////
        // keyword: "extract_sessionid"
        ////

        if (response_line_count != 1) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log,
                  "T+%4.3fms : FAIL : Could not parse session ID: Last "
                  "response contained %d lines, instead of exactly 1.\n",
                  tdelta, response_line_count);
          goto fail;
        }

        if (validate_session_id(response_lines[0])) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log,
                  "T+%4.3fms : FAIL : Could not parse session ID: "
                  "validate_session_id() reported failure.\n",
                  tdelta);
          goto fail;
        }

        // Remember session ID for other directives
        strcpy(last_sessionid, response_lines[0]);
        tdelta = gettime_us() - start_time;
        tdelta /= 1000;
        fprintf(log, "T+%4.3fms : Session ID is '%s'\n", tdelta,
                last_sessionid);

      } else if (sscanf(line, "match_string %[^\r\n]", glob) == 1) {

        ////
        // keyword: "match_string"
        ////

        // Check that the response contains the supplied pattern
        int matches = 0;
        for (int i = 0; i < response_line_count; i++) {
          if (strstr(response_lines[i], glob)) {
            matches++;
          }
        }

        if (!matches) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log, "T+%4.3fms : FAIL : No match for literal string.\n",
                  tdelta);
          goto fail;
        }

      } else if (sscanf(line, "nomatch_string %[^\r\n]", glob) == 1) {

        ////
        // keyword: "extract_sessionid"
        ////

        // Check that the response contains the supplied pattern
        int matches = 0;
        for (int i = 0; i < response_line_count; i++) {
          if (strstr(response_lines[i], glob)) {
            matches++;
          }
        }

        if (matches) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log,
                  "T+%4.3fms : FAIL : There are matches for literal string.\n",
                  tdelta);
          goto fail;
        }

      } else if (sscanf(line, "for %s = %d to %d", var, &first, &last) == 3) {

        ////
        // keyword: "for $ = $ to $"
        ////

        if (for_count > 10) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log, "T+%4.3fms : FATAL: Too many FOR statements\n", tdelta);
          goto fatal;
        }

        if (strlen(var) > 15) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(
              log,
              "T+%4.3fms : FATAL: Variable name too long in FOR statement\n",
              tdelta);
          goto fatal;
        }

        strcpy(for_var[for_count], var);
        for_prevval[for_count] = first;
        for_last[for_count] = last;
        for_step[for_count] = 1;
        for_seek_pos[for_count++] = ftello(in);

      } else if (!strcmp(line, "next")) {

        ////
        // keyword: "next"
        ////

        if (!for_count) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log, "T+%4.3fms : FATAL: NEXT without FOR\n", tdelta);
          goto fatal;
        }

        if (for_prevval[for_count - 1] == for_last[for_count - 1]) {
          // No need to loop back
          for_count--;
        } else {
          for_prevval[for_count - 1] += for_step[for_count - 1];

          if (fseeko(in, for_seek_pos[for_count - 1], SEEK_SET)) {
            tdelta = gettime_us() - start_time;
            tdelta /= 1000;
            fprintf(log,
                    "T+%4.3fms : ERROR : Could not seek to top of FOR %s loop "
                    "at offset %lld\n",
                    tdelta, for_var[for_count - 1],
                    (long long)for_seek_pos[for_count - 1]);
            goto error;
          }
        }

      } else if (sscanf(line, "match %[^\r\n]", glob) == 1) {

        ////
        // keyword: "match" (pattern)
        ////

        // Check that the response contains the supplied pattern
        regex_t regex;
        int matches = 0;
        int error_code = regcomp(&regex, glob, REG_EXTENDED | REG_NOSUB);

        if (error_code) {
          char err[MAX_LINE] = "";
          regerror(error_code, &regex, err, MAX_LINE);
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(
              log,
              "T+%4.3fms : FATAL: Could not compile regular expression: %s\n",
              tdelta, err);
          goto fatal;
        }

        for (int i = 0; i < response_line_count; i++) {
          if (REG_NOMATCH != regexec(&regex, response_lines[i], 0, NULL, 0)) {
            matches++;
          }
        }

        if (!matches) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log, "T+%4.3fms : FAIL: No match for regular expression.\n",
                  tdelta);
          goto fail;
        }

      } else if (sscanf(line, "nomatch %[^\r\n]", glob) == 1) {

        ////
        // keyword: "nomatch" (pattern)
        ////

        // Check that the response contains the supplied pattern
        regex_t regex;
        int matches = 0;
        int error_code = regcomp(&regex, glob, REG_EXTENDED | REG_NOSUB);

        if (error_code) {
          char err[MAX_LINE] = "";
          regerror(error_code, &regex, err, MAX_LINE);
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(
              log,
              "T+%4.3fms : FATAL: Could not compile regular expression: %s\n",
              tdelta, err);
          goto fatal;
        }

        for (int i = 0; i < response_line_count; i++) {
          if (REG_NOMATCH != regexec(&regex, response_lines[i], 0, NULL, 0)) {
            matches++;
          }
        }

        if (matches) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log,
                  "T+%4.3fms : FAIL: There is a match to the regular "
                  "expression.\n",
                  tdelta);
          goto fail;
        }

      } else if (strncmp("request", line, strlen("request")) == 0) {

        ////
        // keyword: "request"
        ////

        // Exeucte curl call. If it is a newsession command, then remember the session ID
        // We also log the returned data from the request, so that we can look at that
        // as well, if required.
        char cmd[MAX_BUFFER];
        char tmp[MAX_BUFFER];
        int expected_http_status = 0;

        tdelta = gettime_us() - start_time;
        tdelta /= 1000;

        // Delete any old version of files laying around
        snprintf(tmp, MAX_BUFFER, "%s/request.out", test->dir);
        unlink(tmp);
        if (!access(tmp, F_OK)) {
          fprintf(log, "T+%4.3fms : FATAL: Could not unlink file '%s'", tdelta,
                  tmp);
          goto fatal;
        }

        snprintf(tmp, MAX_BUFFER, "%s/request.code", test->dir);
        unlink(tmp);
        if (!access(tmp, F_OK)) {
          fprintf(log, "T+%4.3fms : FATAL: Could not unlink file '%s'", tdelta,
                  tmp);
          goto fatal;
        }

        if (parse_request(line, cmd, &expected_http_status, last_sessionid, test->dir, log)) {
          fprintf(log, "T+%4.3fms : FATAL: Could not parse args in declaration \"%s\"'", tdelta,
                  line);
          goto fatal;
        }
        fprintf(log, "T+%4.3fms : HTTP API request command: '%s'\n", tdelta,
                cmd);

        int shell_result = system(cmd);
        if (shell_result) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log,
                  "T+%4.3fms : HTTP API request command returned with non-zero "
                  "status %d.\n",
                  tdelta, shell_result);
        }

        // handle  response
        int httpcode = -1;
        tdelta = gettime_us() - start_time;
        tdelta /= 1000;
        fprintf(log, "T+%4.3fms : HTTP API request command completed.\n",
                tdelta);
        FILE *rc;

        snprintf(cmd, MAX_BUFFER, "%s/request.out", test->dir);
        rc = fopen(cmd, "r");

        if (!rc) {

          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log,
                  "T+%4.3fms : NOTE : Could not open '%s/request.out'. No "
                  "response from web page?\n",
                  tdelta, test->dir);

        } else {

          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log, "T+%4.3fms : HTTP request response body:\n", tdelta);
          response_line_count = 0;
          line[0] = 0;
          fgets(line, MAX_LINE, rc);

          while (line[0]) {
            int len = strlen(line);
            // Trim CR/LF from the end of the line
            while (len && (line[len - 1] < ' ')) {
              line[--len] = 0;
            }
            fprintf(log, "::: %s\n", line);

            // Keep the lines returned from the request in case we want to probe them
            if (response_line_count < 100) {
              line[8191] = 0;
              strcpy(response_lines[response_line_count++], line);
            }

            line[0] = 0;
            fgets(line, MAX_LINE, rc);
          }
          fclose(rc);

        } // endif !rc

        snprintf(cmd, MAX_BUFFER, "%s/request.code", test->dir);
        rc = fopen(cmd, "r");

        if (!rc) {

          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log,
                  "T+%4.3fms : FATAL: Could not open '%s/request.code' to "
                  "retrieve HTTP response code.\n",
                  tdelta, test->dir);
          goto fatal;

        } else {

          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log, "T+%4.3fms : HTTP request.code response:\n", tdelta);
          line[0] = 0;
          fgets(line, 1024, rc);
          while (line[0]) {
            int len = strlen(line);
            // Trim CR/LF from the end of the line
            while (len && (line[len - 1] < ' '))
              line[--len] = 0;

            sscanf(line, "HTTPRESULT=%d", &httpcode);

            fprintf(log, "=== %s\n", line);
            line[0] = 0;
            fgets(line, 1024, rc);
          }
          fclose(rc);

        } // endif !rc

        tdelta = gettime_us() - start_time;
        tdelta /= 1000;
        fprintf(log, "T+%4.3fms : HTTP response code %d\n", tdelta, httpcode);

        if (httpcode == -1) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log,
                  "T+%4.3fms : FATAL: Could not find HTTP response code in "
                  "request.code file.\n",
                  tdelta);
          goto fatal;
        }

        if (httpcode != expected_http_status) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log,
                  "T+%4.3fms : ERROR : Expected HTTP response code %d, but got "
                  "%d.\n",
                  tdelta, expected_http_status, httpcode);
          goto fail;
        }

      } else if (sscanf(line, "session_add_answer %[^\r\n]", arg) == 1) {

        ////
        // keyword: "session_add_answer"
        ////

       if (validate_session_id(last_sessionid)) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log,
            "T+%4.3fms : FATAL: (session_add_answer) No session ID has been captured. Use "
            "extract_sessionid following request directive.\n",
            tdelta);
          goto fatal;
        }

        trim_crlf(arg);
        if (arg[0] == 0) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log,
            "T+%4.3fms : FATAL: (session_add_answer) no answer defined.\n",
            tdelta);
          goto fatal;
        }

        // replace <UTIME> with timestamp
        char parsed[1024];
        char *hit = strstr(arg, "<UTIME>");
        if (hit) {
          size_t pos = strlen(arg) - strlen("<UTIME>");
          arg[pos] = 0;
          snprintf(parsed, 1024, "%s%d" , arg, (int)time(NULL));
        } else{
          strncpy(parsed, arg, 1024);
        }

        // TODO we could invoke backends: deserialise_answer() here for validation

        // Build path to session file
        char path[1024];
        snprintf(path, 1024, "%s/sessions/%c%c%c%c/%s", test->dir,
                  last_sessionid[0], last_sessionid[1], last_sessionid[2],
                  last_sessionid[3], last_sessionid);

        FILE *fp = fopen(path, "a");
        if (!fp) {
          fprintf(stderr, "Could not open session file '%s': %s\n", path, strerror(errno));
          goto fatal;
        }

        fprintf(fp, "%s\n", parsed);
        fclose(fp);
        arg[0] = 0;

      } else if (strncmp("verify_session", line, strlen("verify_session")) == 0) {
        int skip_headers = 0;

        ////
        // keyword: "verify_session"
        ////

        if (validate_session_id(last_sessionid)) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log,
            "T+%4.3fms : FATAL: (verify_session) No session ID has been captured. Use "
            "extract_sessionid following request directive.\n",
            tdelta);
          goto fatal;
        }

        FILE *s = open_session_file_copy(last_sessionid, test);
        if(!s) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log, "T+%4.3fms : Create copy of session file failed: '%s/%s.cpy'.\n", tdelta, test->dir, last_sessionid);
          goto fatal;
        }

        // parse out optional args
        char arg[1024];
        if (sscanf(line, "verify_session(%s)", arg) == 1) {
          arg[strlen(arg) - 1] = 0; // replase closing ')'
          if(!strcmp(arg, "skip_headers")) {
            skip_headers = 1;
          }
        }

        // TODO for now(!) we always ignore the header line with the hashed session id
        // read first line (survey sha) and ignore it
        int skip_s = 1;
        int skip_c = 1;

        // skip n first session lines (headers)
        if (skip_headers) {
          char tmp[MAX_LINE];
          int count = 0;
          while (fgets(tmp, MAX_LINE, s)) {
            count++;
            if (count <= skip_s) {
              continue;
            }
            if(tmp[0] != '@') {
              count--;
              break;
            }
          } //end while
          skip_s = count;
          rewind(s);
        }

        fprintf(log, "- skip_headers flag %s, skipping first %d lines of session file\n", (skip_headers) ? "set" : "not set", skip_s);
        fprintf(log, "<<<<<< START OF EXPECTED SESSION DATA\n");
        fprintf(log, ">>>>>> START OF SESSION FILE\n");

        int ret = compare_session(s, skip_s, in, skip_c, test, log, start_time);
        if (s) {
          fclose(s);
        }

        if (ret) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log, "T+%4.3fms : compare session failed: '%s'.\n", tdelta, last_sessionid);
          goto fail;
        }

        ////
        // keywords: End
        ////

      } else if (line[0] == 0) {
        // Ignore blank lines
      } else if (line[0] == '@') {
        // Ignore header lines
      } else if (line[0] == '#') {
        // print special comments starting with "#!"
        if (line[1] == '!') {
          char *l = line;
          l += 2;
          fprintf(log, "%s\n", l);
        }
        // Ignore default comments
      } else {
        fprintf(
            log,
            "T+%4.3fms : FATAL: Test script '%s' has unknown directive '%s'\n",
            tdelta, test->file, line);
        goto fatal;
      }

      line[0] = 0;
      fgets(line, 1024, in);

    } // endwhile line[0]

    //    pass:

    fix_ownership(test->dir);
    if (log) {
      dump_logs(log_dir, log);
    }

    fprintf(stderr, "\r\033[39m[\033[32mPASS\033[39m] \033[37m(%d/%d)\033[39m %s : %s\n", test->index, test->count, test->name, test->description);
    fflush(stderr);
    break;

  skip:

    fix_ownership(test->dir);

    fprintf(stderr, "\r\033[90m[\033[33mSKIP\033[39m] \033[37m(%d/%d)\033[39m %s : %s\n", test->index, test->count, test->name, test->description);
    fflush(stderr);
    break;

  fail:

    fix_ownership(test->dir);
    if (log) {
      dump_logs(log_dir, log);
    }

    fprintf(stderr, "\r\033[39m[\033[31mFAIL\033[39m] \033[37m(%d/%d)\033[39m %s : %s\n", test->index, test->count, test->name, test->description);
    fflush(stderr);
    retVal = 1;
    break;

  error:

    fix_ownership(test->dir);
    if (log) {
      dump_logs(log_dir, log);
    }

    fprintf(stderr, "\r\033[39m[\033[31;1;5mERROR\033[39;0m] \033[37m(%d/%d)\033[39m %s : %s\n", test->index, test->count, test->name, test->description);
    fflush(stderr);
    retVal = 2;
    break;

  fatal:

    fix_ownership(test->dir);
    if (log) {
      dump_logs(log_dir, log);
    }

    fprintf(stderr, "\r\033[39m[\033[31;1;5mDIED\033[39;0m] \033[37m(%d/%d)\033[39m %s : %s\n", test->index, test->count, test->name, test->description);
    fflush(stderr);
    retVal = 3;
    break;

  } while (0);

  if (in) {
    fclose(in);
  }

  if (log) {
    fclose(log);
  }

  return retVal;
}

void init_lighttpd(struct Test *test) {
  char conf_path[1024];
  char userfile_path[1024] = { '\0' };
  char cmd[2048];

  // kill open ports
  stop_lighttpd(test->count == 1);

  if (test->count == 1) {
    fprintf(stderr, "Pulling configuration together...\n");
  }

  // path to conf
  snprintf(conf_path, 1024, "%s/lighttpd.conf", test->dir);

  // path to user file
  snprintf(userfile_path, 1024, "%s/lighttpd.user", test->dir);

  // copy lighttpd.conf from template
  snprintf(cmd, 2048, "sudo cp %s %s", test->lighty_template, conf_path);
  if (system(cmd)) {
    fprintf(stderr, "system() call to copy lighttpd template failed");
    exit(-3);
  }

  if (strlen(test->ligthy_userfile)) {
    // copy userfile
    snprintf(cmd, 2048, "sudo cp %s %s", test->ligthy_userfile, userfile_path);
    if (system(cmd)) {
      fprintf(stderr, "system() call to copy lighttpd user file failed");
      exit(-3);
    }
  }

  snprintf(cmd, 16384,
    // vars
    "sed -i                              \\"
    "-e 's|{BASE_DIR}|%s|g'              \\"
    "-e 's|{PID_FILE}|%s|g'              \\"
    "-e 's|{LIGHTY_USER}|%s|g'           \\"
    "-e 's|{LIGHTY_GROUP}|%s|g'          \\"
    "-e 's|{SERVER_PORT}|%d|g'           \\"
    "-e 's|{FCGIENV_MIDDLEWARE}|%s|g' \\"
    "-e 's|{SURVEYFCGI_PORT}|%d|g'       \\"
    "-e 's|{DIGEST_USERFILE}|%s|g'       \\"
    // path
    "%s",
    // vars
    test->dir,
    LIGHTY_PIDFILE,
    LIGHTY_USER,
    LIGHTY_GROUP,
    SERVER_PORT,
    test->fcgienv_middleware,
    SURVEYFCGI_PORT,
    userfile_path,
    // path
    conf_path
  );

  if (system(cmd)) {
    fprintf(stderr, "replacing template string in lighttpd.conf failed: %s",
                conf_path);
    exit(-3);
  }

  // #333 remove creating temp config in /etc/lighttpd,
  // sysymlink /etc/lighttpd/conf-enabled into test dir (required by mod_fcgi)
  char sym_path[1024];
  snprintf(sym_path, 1024, "%s/conf-enabled", test->dir);
  if (access(sym_path, F_OK)) {
    if(symlink("/etc/lighttpd/conf-enabled", sym_path)) {
      fprintf(stderr, "symlinking '/etc/lighttpd/conf-enabled' => '%s' failed, error: %s",
                  sym_path, strerror(errno));
      exit(-3);
    }
  }

  snprintf(cmd, 2048, "sudo cp surveyfcgi %s/surveyfcgi", test->dir);
  if (!tests) {
    fprintf(stderr, "Running '%s'\n", cmd);
  }

  if (system(cmd)) {
    fprintf(stderr, "system() call to copy surveyfcgi failed: %s",
                strerror(errno));
    exit(-3);
  }

  snprintf(cmd, 2048, "sudo lighttpd -f %s", conf_path);
  if (test->count == 1) {
    fprintf(stderr, "Running '%s'\n", cmd);
  }

  if (system(cmd)) {
    fprintf(stderr, "system() call to start lighttpd failed: %s", strerror(errno));
    exit(-3);
  }

  snprintf(
      cmd, 2048,
      "curl -s -o /dev/null -f http://localhost:%d/surveyapi/fastcgitest",
      SERVER_PORT);

  if (test->count == 1) {
    fprintf(stderr, "Running '%s'\n", cmd);
  }

  int v = 0;
  while ((v = system(cmd)) != 0) {
    if (test->count == 1) {
      fprintf(stderr, "curl returned code %d\n    - command: '%s'\n", v, cmd);
    }
    char log_dir[1024];
    // This should be /logs, but it doesn't exist yet, and we really only need it for the
    // ../ to get to breakage.log
    snprintf(log_dir, 1024, "%s/sessions", test->dir);
    if (test->count == 1) {
      dump_logs(log_dir, stderr);
    }
    sleep(2);
    continue;
  }

  if (test->count == 1) {
    fprintf(stderr, "lighttpd is now responding to requests.\n");
  }

  if (test->count == 1) {
    fprintf(stderr, "All done.\n");
  }
}

/**
 * #333, kill connections synchronous
 * requires sh: ./scripts/killport
 */
void stop_lighttpd(int verbose) {

    char cmd[2048];
    char out[2048];
    FILE *fp;
    int status;

    snprintf(cmd, 2048, "sudo ./scripts/killport %d 2>&1", SERVER_PORT);
    fp = popen(cmd, "r");
    if (fp == NULL) {
      fprintf(stderr, "FAIL\nsystem(popen) call to stop lighttpd failed ('%s')\n", cmd);
      exit(-3);
    }

    while (fgets(out, sizeof(out), fp) != NULL) {
      if (verbose) {
        fprintf(stderr, "%s\n", out);
      }
    }

    status = pclose(fp);
    if (status) {
      fprintf(stderr, "stopping lighttpd on port '%d' failed (exit code: %d)\n", SERVER_PORT, WEXITSTATUS(status));
      exit(-3);
    }

    ////

    snprintf(cmd, 2048, "sudo ./scripts/killport %d 2>&1", SURVEYFCGI_PORT);
    fp = popen(cmd, "r");
    if (fp == NULL) {
      fprintf(stderr, "FAIL\nsystem(popen) call to stop lighttpd failed ('%s')\n", cmd);
      exit(-3);
    }

    while (fgets(out, sizeof(out), fp) != NULL) {
      if (verbose) {
        fprintf(stderr, "%s", out);
      }
    }

    status = pclose(fp);
    if (status) {
      fprintf(stderr, "stopping lighttpd on port '%d' failed (exit code: %d)\n", SURVEYFCGI_PORT, WEXITSTATUS(status));
      exit(-3);
    }

    sleep(1);
}

/**
 * #333, get current pat to executable (for setting SURVEY_HOME)
 */
int get_current_path(int argc, char **argv, char *path_out, size_t max_len) {
  int retVal = 0;
  do {

    if (!path_out) {
        retVal = -1;
        break;
    }

    char prog_name[max_len];
    strncpy(prog_name, argv[0], max_len);
    char *dir = dirname(prog_name);

    // argv is absolute path
    if(argv[0][0] == '/') {
        strncpy(path_out, dir, max_len);
        break;
    }

    // argv is relative path
    char cwd[1024];
    if (getcwd(cwd, max_len) == NULL) {
        fprintf(stderr, "getcwd failed");
        retVal = -1;
        break;
    }
    if (snprintf(dir, max_len, "%s%s", dir, cwd) < 0) {
        fprintf(stderr, "snprintf failed");
        retVal = -1;
        break;
    }
    if (realpath(dir, path_out) == NULL) {
        fprintf(stderr, "realpath failed");
        retVal = -1;
        break;
    }

  } while (0);
  return retVal;
}

int main(int argc, char **argv) {
  // #333, we are using some backend functions in our test code who rely on generate_path,
  // for this to work we need to set SURVEY_HOME for this executable,
  // this is indepenent to fastcgi SURVEY_HOME
  char current_path[1024];
  if (get_current_path(argc, argv, current_path, 1024)) {
    fprintf(stderr, "Compiling current path failed");
    exit(-1);
  }

  char varname[1024];
  if (snprintf(varname, 1024, "SURVEY_HOME=%s", current_path) < 0) {
    fprintf(stderr, "compiling env var failed (1)");
    exit(-1);
  }
  if(putenv(varname)) {
    fprintf(stderr, "putenv SURVEY_HOME failed");
    exit(-1);
  }

  // Make sure we have a test log directory
  require_directory("testlog", 0755);

  int passes = 0;
  int fails = 0;
  int errors = 0;
  int fatals = 0;

  for (int i = 1; i < argc; i++) {

    // handle only files with extension ".test", This allows us to store supporting configs in folder
    char *e = strrchr(argv[i], '.');
    if (!e || strlen(e) != 5 || strcmp(".test", e)) {
        continue;
    }

    struct Test test = create_test_config();
    init_parse_test_config(argc - 1, i, argv[i], &test);
    // print_test_config(stderr, &test);
    init_test_filesystem(&test);
    init_lighttpd(&test);

    switch (run_test(&test)) {
    case 0:
      passes++;
      break;

    case 1:
      fails++;
      break;

    case 2:
      errors++;
      break;

    case 3:
      fatals++;
      break;
    }

    tests++;
  }

  // Clean up after ourselves
  fprintf(stderr, "Cleaning up...\n");

  stop_lighttpd(argc == 2);

  fprintf(stderr,
          "Summary: %d/%d tests passed (%d failed, %d errors, %d fatalities "
          "during tests)\n",
          passes, tests, fails, errors, fatals);
  return 0;
}
