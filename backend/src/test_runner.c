/*
  Simple test runner for the survey system.
  It runs tests that are each defined by a single file.
  The test specification is as follows:

  description <description>
  definesurvey <name>
  <questions in normal question format>
  endofsurvey
  request <expected response code> <url path and query>
  extract_sessionid
  match <regex to search for in body of last response>
  nomatch <regex to search for in body of last response>
  verifysession
  <expected set of answers in the session file. Can be empty>
  endofsession

  These commands can be used more than once, so that more complex activities can be scripted.

  Apart from running these scripts, all that it has to do to is to setup and cleanup the
  directories for running the tests, and setup the config file for lighttpd for testing, and
  actually stop and start lighttpd.

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

#define SERVER_PORT 8099
#define AUTH_PROXY_PORT 8199
#define SURVEYFCGI_PORT 9009
#define LIGHTY_USER "www-data"
#define LIGHTY_GROUP "www-data"
#define LIGHTY_PIDFILE "/var/run/lighttpd.pid"

// test configuration
struct Test {
    int skip;

    char name[1024];
    char description[8192];

    char file[1024];
    char dir[1024];
    char log[1024];

    char lighty_template[1024];
    char ligthy_userfile[1024];

    int count;
    int index;
};

struct Test create_test_config() {
  struct Test test = {
    .skip = 0,

    .name = "",
    .description = "",

    .file = "",
    .dir = "",

    .lighty_template =  "tests/config/lighttpd-default.conf.tpl",
    .ligthy_userfile = "tests/config/lighttpd.user",

    .count = 0,
    .index = 0,
  };
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
    char cmd[8192];
    snprintf(cmd, 8192, "sudo chown -R %s:%s %s\n", LIGHTY_USER, LIGHTY_GROUP,
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
          char line[8192];
          line[0] = 0;
          fgets(line, 8192, in);

          while (line[0]) {
            fprintf(log, "%s", line);
            line[0] = 0;
            fgets(line, 8192, in);
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
        char tmp[65536];

        char args[65536];
        char url[65536];
        char data[65536] = {'\0'};
        char method[65536] = {'\0'};
        char curl_args[65536]= {'\0'};
        char url_sub[65536];

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
            strncpy(args, tmp, 65536);
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
              snprintf(&url_sub[o], 65535 - o, "%s", last_sessionid);
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
          strncpy(tmp, data, 65536);
          snprintf(data, 65536, " -d %s", tmp);
        }

        if (strlen(method)) {
          strncpy(tmp, method, 65536);
          snprintf(method, 65536, " -X %s", tmp);
        }

        if (strlen(curl_args)) {
          strncpy(tmp, curl_args, 65536);
          snprintf(curl_args, 65536, " %s", tmp);
        }

        // build curl cmd
        snprintf(
          out, 65536,
          "curl%s%s%s -s -w \"HTTPRESULT=%%{http_code}\" -o "
          "%s/request.out \"http://localhost:%d/%s/%s\" > "
          "%s/request.code",
          curl_args,
          method,
          data,
          dir,
          (proxy) ? AUTH_PROXY_PORT: SERVER_PORT,
          "surveyapi",
          url_sub,
          dir
        );
    } while(0);

    return retVal;
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
int compare_session_line(char *session_line, char *comparison_line) {
  // do not mutate arg strings
  char *left_line = strdup(session_line);
  char *right_line = strdup(comparison_line);

  char *left_ptr;
  char *right_ptr;

  char *left = strtok_r(left_line, ":", &left_ptr);
  char *right = strtok_r(right_line, ":", &right_ptr);

  while (right) {

    // match number of columns: session line too short
    if (!left) {
      return -2;
    }

    // validate <UTIME> keyword (unix timestamp)
    if (!strcmp(right, "<UTIME>")) {
      int now = (int)time(NULL);
      int then = atoi(left);

      if (now <= 0 || now < then || now - then > 8600) {
        return -4;
      }

      return 0;
    }

    // compare column text
    if (strcmp(left, right)) {
      return -1;
    }

    left = strtok_r(NULL, ":", &left_ptr);
    right = strtok_r(NULL, ":", &right_ptr);
  }

  // match number of columns: session line too long
  if (left) {
    return -3;
  }

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
    char line[8192];
    fgets(line, 8192, fp);
    if (sscanf(line, "description %[^\r\n]", test->description) != 1) {
      fprintf(stderr, "\nCould not parse description line of test.\n");
      fclose(fp);
      return NULL;
    }

    while(fgets(line, 8192, fp) != NULL) {
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
    char line[8192];
    fgets(line, 8192, fp);
    if (sscanf(line, "@description %[^\r\n]", test->description) != 1) {
      fprintf(stderr, "\nCould not parse description line of test->\n");
      exit(-3);
    }

    while(fgets(line, 8192, fp) != NULL) {
        // end of config header
        if (line[0] != '@') {
          break;
        }

        if (strncmp(line, "@skip!", 6) == 0) {
            test->skip = 1;
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
  char line[8192];
  char log_dir[1024];

  int response_line_count = 0;
  char response_lines[100][8192];
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

    char surveyname[8192] = "";
    char glob[65536];
    double tdelta;

    // Variables for FOR NEXT loops
    char var[8192];
    int first, last;
    int for_count = 0;
    char for_var[10][16];
    int for_prevval[10];
    int for_last[10];
    int for_step[10];
    off_t for_seek_pos[10];

    // Now iterate through test script
    line[0] = 0;
    fgets(line, 8192, in);

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
        char survey_file[8192];

        // Trim trailing white space from survey name to avoid annoying mismatches
        while (surveyname[0] && surveyname[strlen(surveyname) - 1] == ' ') {
          surveyname[strlen(surveyname) - 1] = 0;
        }

        snprintf(survey_file, 8192, "%s/surveys/%s", test->dir, surveyname);
        mkdir(survey_file, 0777);
        if (chmod(survey_file, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP |
                                   S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH)) {
          fprintf(stderr, "\nERROR: chmod() failed for new survey directory %s",
                  survey_file);
          goto error;
        }

        snprintf(survey_file, 8192, "%s/surveys/%s/current", test->dir, surveyname);
        FILE *s = fopen(survey_file, "w");
        if (!s) {
          fprintf(stderr,
                  "\rERROR: Could not create survey file '%s'                  "
                  "                                             \n",
                  survey_file);
          goto error;
        }

        line[0] = 0;
        fgets(line, 8192, in);

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
          fgets(line, 8192, in);
        }

        fclose(s);

      } else if (!strcmp(line, "python")) {

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
        fgets(line, 8192, in);
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
          fgets(line, 8192, in);
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
          char err[8192] = "";
          regerror(error_code, &regex, err, 8192);
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
          char err[8192] = "";
          regerror(error_code, &regex, err, 8192);
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
        char cmd[65536];
        char tmp[65536];
        int expected_http_status = 0;

        tdelta = gettime_us() - start_time;
        tdelta /= 1000;

        // Delete any old version of files laying around
        snprintf(tmp, 65536, "%s/request.out", test->dir);
        unlink(tmp);
        if (!access(tmp, F_OK)) {
          fprintf(log, "T+%4.3fms : FATAL: Could not unlink file '%s'", tdelta,
                  tmp);
          goto fatal;
        }

        snprintf(tmp, 65536, "%s/request.code", test->dir);
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

        snprintf(cmd, 65536, "%s/request.out", test->dir);
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
          fgets(line, 8192, rc);

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
            fgets(line, 8192, rc);
          }
          fclose(rc);

        } // endif !rc

        snprintf(cmd, 65536, "%s/request.code", test->dir);
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

      } else if (!strcmp(line, "verify_session")) {

        ////
        // keyword: "verify_session"
        ////

        if (validate_session_id(last_sessionid)) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log,
                  "T+%4.3fms : FATAL: No session ID has been captured. Use "
                  "extract_sessionid following request directive.\n",
                  tdelta);
          goto fatal;
        }

        // Build path to session file
        char session_file[8192];
        snprintf(session_file, 1024, "%s/sessions/%c%c%c%c/%s", test->dir,
                 last_sessionid[0], last_sessionid[1], last_sessionid[2],
                 last_sessionid[3], last_sessionid);

        char cmd[8192];
        snprintf(cmd, 8192, "%s/session.log", test->dir);
        unlink(cmd);

        snprintf(cmd, 8192, "sudo cp %s %s/session.log", session_file, test->dir);
        system(cmd);

        tdelta = gettime_us() - start_time;
        tdelta /= 1000;
        fprintf(log, "T+%4.3fms : Examining contents of session file '%s'.\n",
                tdelta, session_file);

        // Check that the file exists
        snprintf(cmd, 8192, "%s/session.log", test->dir);
        FILE *s = fopen(cmd, "r");
        if (!s) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log, "T+%4.3fms : FAIL : Could not open session file: %s\n",
                  tdelta, strerror(errno));
          goto fail;
        }

        // Now go through reading lines from here and from the session file
        char session_line[8192];
        char comparison_line[8192];

        fprintf(log, "<<<<<< START OF EXPECTED SESSION DATA\n");
        fprintf(log, ">>>>>> START OF SESSION FILE\n");

        session_line[0] = 0;
        fgets(session_line, 8192, s);
        comparison_line[0] = 0;
        fgets(comparison_line, 8192, in);

        int verify_errors = 0;
        int verify_line = 0;

        while (1) {

          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          if (comparison_line[0] && strcmp(comparison_line, "endofsession")) {
            int len = strlen(comparison_line);
            while (len && (comparison_line[len - 1] == '\r' ||
                           comparison_line[len - 1] == '\n')) {
              comparison_line[--len] = 0;
            }
            fprintf(log, "<<< %s\n", comparison_line);
          } else {
            fprintf(log, "<<<<<< END OF EXPECTED SESSION DATA\n");
          }

          if (session_line[0]) {
            int len = strlen(session_line);
            while (len && (session_line[len - 1] == '\r' ||
                           session_line[len - 1] == '\n')) {
              session_line[--len] = 0;
            }
            fprintf(log, ">>> %s\n", session_line);
          } else {
            fprintf(log, ">>>>>> END OF SESSION FILE\n");
          }

          // compare session line, we are relying on the string terminations set above
          if (verify_line >
              0) { // TODO for now(!) we ignore the header line with the hashed session id

            if ((session_line[0] && comparison_line[0]) &&
                strcmp(comparison_line, "endofsession")) {
              verify_errors =
                  compare_session_line(session_line, comparison_line);

              switch (verify_errors) {
              case 0:
                // ok, do nothing
                break;

              case -1:
                fprintf(log,
                        "  MISMATCH : compare_session_line() field mismatch "
                        "(line %d, code %d): \"%s\" != \"%s\"\n",
                        verify_line, verify_errors, session_line,
                        comparison_line);
                break;

              case -2:
                fprintf(
                    log,
                    "  MISMATCH : compare_session_line() session line too "
                    "short (columns) (line %d, code %d): \"%s\" != \"%s\"\n",
                    verify_line, verify_errors, session_line, comparison_line);
                break;

              case -3:
                fprintf(log,
                        "  MISMATCH : compare_session_line() session line too "
                        "long (columns) (line %d, code %d): \"%s\" != \"%s\"\n",
                        verify_line, verify_errors, session_line,
                        comparison_line);
                break;

              case -4:
                fprintf(log,
                        "  MISMATCH : compare_session_line() <UTIME> invalid "
                        "(line %d, code %d): \"%s\" != \"%s\"\n",
                        verify_line, verify_errors, session_line,
                        comparison_line);
                break;

              default:
                fprintf(log,
                        "  MISMATCH : compare_session_line() unknown return "
                        "code (line %d, code %d): \"%s\" != \"%s\"\n",
                        verify_line, verify_errors, session_line,
                        comparison_line);
              } // endswitch
            }

          } // endif verify_line

          if (session_line[0] && (!strcmp("endofsession", comparison_line))) {
            // End of comparison list before end of session file
            tdelta = gettime_us() - start_time;
            tdelta /= 1000;
            fprintf(log,
                    "T+%4.3fms : FAIL : 1 Session log file contains more lines "
                    "than expected.\n",
                    tdelta);
            goto fail;
          }

          if ((!session_line[0]) && (strcmp("endofsession", comparison_line))) {
            // End of session file before end of comparison list
            tdelta = gettime_us() - start_time;
            tdelta /= 1000;
            fprintf(log,
                    "T+%4.3fms : FAIL : 2 Session log file contains less lines "
                    "than expected.\n",
                    tdelta);
            goto fail;
          }

          if (!strcmp("endofsession", comparison_line)) {
            break;
          }

          verify_line++;
          session_line[0] = 0;
          fgets(session_line, 8192, s);
          comparison_line[0] = 0;
          fgets(comparison_line, 8192, in);

        } // end while 1

        fclose(s);

        // fail if session contents mismatch
        if (verify_errors) {
          fprintf(log,
                  "T+%4.3fms : FAIL : verifysession: %d lines in session file "
                  "do not match!.\n",
                  tdelta, verify_errors);
          goto fail;
        } else {
          fprintf(
              log,
              "T+%4.3fms : verifysession: session file matches comparasion.\n",
              tdelta);
        }

        ////
        // keywords: End
        ////

      } else if (line[0] == 0) {
        // Ignore blank lines
      } else if (line[0] == '#') {
        // Ignore header lines
      } else if (line[0] == '@') {
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

    fprintf(stderr, "\r\033[39m[\033[32mPASS\033[39m]  %s : %s\n", test->name, test->description);
    fflush(stderr);
    break;

  skip:

    fix_ownership(test->dir);

    fprintf(stderr, "\r\033[90m[\033[33mSKIP\033[39m]  %s : %s\n", test->name, test->description);
    fflush(stderr);
    break;

  fail:

    fix_ownership(test->dir);
    if (log) {
      dump_logs(log_dir, log);
    }

    fprintf(stderr, "\r\033[39m[\033[31mFAIL\033[39m]  %s : %s\n", test->name, test->description);
    fflush(stderr);
    retVal = 1;
    break;

  error:

    fix_ownership(test->dir);
    if (log) {
      dump_logs(log_dir, log);
    }

    fprintf(stderr, "\r\033[39m[\033[31;1;5mERROR\033[39;0m]  %s : %s\n", test->name, test->description);
    fflush(stderr);
    retVal = 2;
    break;

  fatal:

    fix_ownership(test->dir);
    if (log) {
      dump_logs(log_dir, log);
    }

    fprintf(stderr, "\r\033[39m[\033[31;1;5mDIED\033[39;0m]  %s : %s\n", test->name, test->description);
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
  char userfile_path[1024];
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

  // copy userfile
  snprintf(cmd, 2048, "sudo cp %s %s", test->ligthy_userfile, userfile_path);
  if (system(cmd)) {
    fprintf(stderr, "system() call to copy lighttpd user file failed");
    exit(-3);
  }

  snprintf(cmd, 16384,
    // vars
    "sed -i                         \\"
    "-e 's|{BASE_DIR}|%s|g'         \\"
    "-e 's|{PID_FILE}|%s|g'         \\"
    "-e 's|{LIGHTY_USER}|%s|g'      \\"
    "-e 's|{LIGHTY_GROUP}|%s|g'     \\"
    "-e 's|{SERVER_PORT}|%d|g'      \\"
    "-e 's|{AUTH_PROXY_PORT}|%d|g'  \\"
    "-e 's|{SURVEYFCGI_PORT}|%d|g'  \\"
    "-e 's|{DIGEST_USERFILE}|%s|g'  \\"
    // path
    "%s",
    // vars
    test->dir,
    LIGHTY_PIDFILE,
    LIGHTY_USER,
    LIGHTY_GROUP,
    SERVER_PORT,
    AUTH_PROXY_PORT,
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
      fprintf(stderr, "cmd returned code %d\n", v);
    }
    char log_dir[8192];
    // This should be /logs, but it doesn't exist yet, and we really only need it for the
    // ../ to get to breakage.log
    snprintf(log_dir, 8192, "%s/sessions", test->dir);
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
        fprintf(stderr, "* NO TEST:%d: %s\n", i, argv[i]);
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
