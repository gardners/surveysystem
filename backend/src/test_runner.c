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

#include "errorlog.h"
#include "survey.h"

#define HTTP_PORT 8099
#define SURVEYFCGI_PORT 9009
#define LIGHTY_USER "www-data"
#define LIGHTY_GROUP "www-data"

char test_dir[1024];
int tests = 0;

int configure_and_start_lighttpd(char *test_dir, int silent_flag);
int stop_lighttpd();

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

void require_test_directory(char *sub_dir, int perm) {
  char tmp[2048];
  snprintf(tmp, 2048, "%s/%s", test_dir, sub_dir);
  if (mkdir(tmp, perm)) {
    if (errno != EEXIST) {
      fprintf(stderr, "mkdir(%s, %o) failed.", tmp, perm);
      exit(-3);
    }
  }
  // Now make sessions directory writeable by all users
  if (chmod(tmp, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP |
                     S_IROTH | S_IWOTH | S_IXOTH)) {
    fprintf(stderr, "require_test_directory() chmod(%s) failed.", tmp);
    exit(-3);
  }
}

void require_test_file(char *file_name, int perm) {
  char tmp[2048];
  snprintf(tmp, 2048, "%s/%s", test_dir, file_name);

  FILE *f = fopen(tmp, "w");
  if (!f) {
    fprintf(stderr, "require_test_file() fopen(%s) failed.", tmp);
    exit(-3);
  }
  fclose(f);

  // Now make sessions directory writeable by all users
  if (chmod(tmp, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP |
                     S_IROTH | S_IWOTH | S_IXOTH)) {
    fprintf(stderr, "require_test_file() chmod(%s) failed.", tmp);
    exit(-3);
  }
}

void require_directory(char *dir, int perm) {
  if (mkdir(dir, perm)) {
    if (errno != EEXIST) {
      fprintf(stderr, "mkdir(%s, %o) failed.", dir, perm);
      exit(-3);
    }
  }
}

// From https://stackoverflow.com/questions/2256945/removing-a-non-empty-directory-programmatically-in-c-or-c
int recursive_delete(const char *dir) {
  int ret = 0;
  FTS *ftsp = NULL;
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
    FTS *ftsp = NULL;
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

int run_test(char *dir, char *test_file) {
  // #198 flush errors accumulated by previous tests
  clear_errors();

  int retVal = 0;
  FILE *in = NULL;
  FILE *log = NULL;
  char line[8192];

  int response_line_count = 0;
  char response_lines[100][8192];
  char last_sessionid[100] = "";

  do {

    // Erase log files from previous tests, see issue #198
    char log_path[8192];
    snprintf(log_path, 8192, "%s/logs", test_dir);
    recursive_delete(log_path);
    if (mkdir(log_path, 0777)) {
      perror("mkdir() failed for test/logs directory");
      exit(-3);
    }
    // Now make sessions directory writeable by all users
    if (chmod(log_path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP |
                            S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH)) {
      perror("chmod() failed for test/logs directory");
      exit(-3);
    }

    // Get name of test file without path
    char *test_name = test_file;
    for (int i = 0; test_file[i]; i++) {
      if (test_file[i] == '/')
        test_name = &test_file[i + 1];
    }

    in = fopen(test_file, "r");
    if (!in) {
      fprintf(stderr, "\nCould not open test file '%s' for reading", test_file);
      perror("fopen");
      retVal = -1;
      break;
    }

    // Read first line of test for description
    line[0] = 0;
    fgets(line, 8192, in);
    if (!line[0]) {
      fprintf(stderr, "\nFirst line of test definition must be description "
                      "<description text>\n");
      retVal = -1;
      break;
    }

    char description[8192];
    if (sscanf(line, "skip! description %[^\r\n]", description) == 1) {
      fprintf(stderr, "\r\033[90m[\033[33mSKIP\033[39m]  %s : %s\n", test_name,
              description);
      fflush(stderr);
      break;
    }

    if (sscanf(line, "description %[^\r\n]", description) != 1) {
      fprintf(stderr, "\nCould not parse description line of test.\n");
      retVal = -1;
      break;
    }

    fprintf(stderr, "\033[39m[    ]  \033[37m%s : %s\033[39m", test_name,
            description);
    fflush(stderr);

    char testlog[1024];
    snprintf(testlog, 1024, "testlog/%s.log", test_name);

    log = fopen(testlog, "w");

    if (!log) {
      fprintf(stderr,
              "\rFATAL: Could not create test log file '%s' for test '%s': %s  "
              "                                  \n",
              testlog, test_file, strerror(errno));
      goto error;
    }

    time_t now = time(0);
    char *ctime_str = ctime(&now);
    fprintf(log, "Started running test at %s", ctime_str);
    long long start_time = gettime_us();

    char surveyname[8192] = "";
    int expected_result = 200;
    char url[65536];
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
      if (line[0] && line[0] != '#') {
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

        snprintf(survey_file, 8192, "%s/surveys/%s", dir, surveyname);
        mkdir(survey_file, 0777);
        if (chmod(survey_file, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP |
                                   S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH)) {
          fprintf(stderr, "\nERROR: chmod() failed for new survey directory %s",
                  survey_file);
          goto error;
        }

        snprintf(survey_file, 8192, "%s/surveys/%s/current", dir, surveyname);
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
        snprintf(python_dir, 1024, "%s/python", dir);
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
                 python_module, testlog);
        int compile_result = system(cmd);

        if (compile_result) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log,
                  "T+%4.3fms : FATAL : Failed to compile python module. Does "
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

        if (configure_and_start_lighttpd(test_dir, 1)) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log, "T+%4.3fms : FATAL : Backend restart failed\n", tdelta);
          goto fatal;
        }

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
          fprintf(log, "T+%4.3fms : FATAL : Too many FOR statements\n", tdelta);
          goto fatal;
        }

        if (strlen(var) > 15) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(
              log,
              "T+%4.3fms : FATAL : Variable name too long in FOR statement\n",
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
          fprintf(log, "T+%4.3fms : FATAL : NEXT without FOR\n", tdelta);
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
              "T+%4.3fms : FATAL : Could not compile regular expression: %s\n",
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
          fprintf(log, "T+%4.3fms : FAIL : No match for regular expression.\n",
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
              "T+%4.3fms : FATAL : Could not compile regular expression: %s\n",
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
                  "T+%4.3fms : FAIL : There is a match to the regular "
                  "expression.\n",
                  tdelta);
          goto fail;
        }

      } else if (sscanf(line, "request %d %[^\r\n]", &expected_result, url) ==
                 2) {

        ////
        // keyword: "request"
        ////

        // Exeucte wget call. If it is a newsession command, then remember the session ID
        // We also log the returned data from the request, so that we can look at that
        // as well, if required.
        char cmd[65536];
        char url_sub[65536];

        // adds extended request options (request method and request data)
        char data[65536] = {'\0'};
        char method[65536] = "GET";
        sscanf(line, "request %d %s %s %s", &expected_result, url, method,
               data);

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
              fprintf(log, "T+%4.3fms : FATAL : Unknown $ substitution in URL",
                      tdelta);
              goto fatal;
            } // endif

          } // endif

        } // endfor
        url_sub[o] = 0;

        // Delete any old version of files laying around
        snprintf(cmd, 65536, "%s/request.out", dir);
        unlink(cmd);
        if (!access(cmd, F_OK)) {
          fprintf(log, "T+%4.3fms : FATAL : Could not unlink file '%s'", tdelta,
                  cmd);
          goto fatal;
        }

        snprintf(cmd, 65536, "%s/request.code", dir);
        unlink(cmd);
        if (!access(cmd, F_OK)) {
          fprintf(log, "T+%4.3fms : FATAL : Could not unlink file '%s'", tdelta,
                  cmd);
          goto fatal;
        }

        // build and execute curl cod
        snprintf(cmd, 65536,
                 "curl -X %s -s -w \"HTTPRESULT=%%{http_code}\" %s%s -o "
                 "%s/request.out \"http://localhost:%d/surveyapi/%s\" > "
                 "%s/request.code",
                 method, ((data[0] == '\0') ? "" : "-d "), data, dir, HTTP_PORT,
                 url_sub, dir);

        tdelta = gettime_us() - start_time;
        tdelta /= 1000;
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
        FILE *rc = NULL;

        snprintf(cmd, 65536, "%s/request.out", dir);
        rc = fopen(cmd, "r");

        if (!rc) {

          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log,
                  "T+%4.3fms : NOTE : Could not open '%s/request.out'. No "
                  "response from web page?\n",
                  tdelta, dir);

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

        snprintf(cmd, 65536, "%s/request.code", dir);
        rc = fopen(cmd, "r");

        if (!rc) {

          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log,
                  "T+%4.3fms : FATAL : Could not open '%s/request.code' to "
                  "retrieve HTTP response code.\n",
                  tdelta, dir);
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
                  "T+%4.3fms : FATAL : Could not find HTTP response code in "
                  "request.code file.\n",
                  tdelta);
          goto fatal;
        }

        if (httpcode != expected_result) {
          tdelta = gettime_us() - start_time;
          tdelta /= 1000;
          fprintf(log,
                  "T+%4.3fms : ERROR : Expected HTTP response code %d, but got "
                  "%d.\n",
                  tdelta, expected_result, httpcode);
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
                  "T+%4.3fms : FATAL : No session ID has been captured. Use "
                  "extract_sessionid following request directive.\n",
                  tdelta);
          goto fatal;
        }

        // Build path to session file
        char session_file[8192];
        snprintf(session_file, 1024, "%s/sessions/%c%c%c%c/%s", test_dir,
                 last_sessionid[0], last_sessionid[1], last_sessionid[2],
                 last_sessionid[3], last_sessionid);

        char cmd[8192];
        snprintf(cmd, 8192, "%s/session.log", dir);
        unlink(cmd);

        snprintf(cmd, 8192, "sudo cp %s %s/session.log", session_file, dir);
        system(cmd);

        tdelta = gettime_us() - start_time;
        tdelta /= 1000;
        fprintf(log, "T+%4.3fms : Examining contents of session file '%s'.\n",
                tdelta, session_file);

        // Check that the file exists
        snprintf(cmd, 8192, "%s/session.log", dir);
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
        // print special comments starting with "#!"
        if (line[1] != 0 && line[1] == '!') {
          char *l = line;
          l += 2;
          fprintf(log, "%s\n", l);
        }
        // Ignore default comments
      } else {
        fprintf(
            log,
            "T+%4.3fms : FATAL : Test script '%s' has unknown directive '%s'\n",
            tdelta, test_file, line);
        goto fatal;
      }

      line[0] = 0;
      fgets(line, 1024, in);

    } // endwhile line[0]

    //    pass:

    fix_ownership(test_dir);
    if (log) {
      dump_logs(log_path, log);
    }

    fprintf(stderr, "\r\033[39m[\033[32mPASS\033[39m]  %s : %s\n", test_name,
            description);
    fflush(stderr);
    break;

  fail:

    fix_ownership(test_dir);
    if (log) {
      dump_logs(log_path, log);
    }

    fprintf(stderr, "\r\033[39m[\033[31mFAIL\033[39m]  %s : %s\n", test_name,
            description);
    fflush(stderr);
    retVal = 1;
    break;

  error:

    fix_ownership(test_dir);
    if (log) {
      dump_logs(log_path, log);
    }

    fprintf(stderr, "\r\033[39m[\033[31;1;5mERROR\033[39;0m]  %s : %s\n",
            test_name, description);
    fflush(stderr);
    retVal = 2;
    break;

  fatal:

    fix_ownership(test_dir);
    if (log) {
      dump_logs(log_path, log);
    }

    fprintf(stderr, "\r\033[39m[\033[31;1;5mDIED\033[39;0m]  %s : %s\n",
            test_name, description);
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

char *config_template =
    "server.modules = (\n"
    "     \"mod_access\",\n"
    "     \"mod_alias\",\n"
    "     \"mod_fastcgi\",\n"
    "     \"mod_compress\",\n"
    "     \"mod_redirect\",\n"
    "     \"mod_accesslog\",\n"
    ")\n"
    "\n"
    "server.breakagelog          = \"%s/breakage.log\"\n"
    "server.document-root        = \"%s/front/build\"\n"
    "server.upload-dirs          = ( \"/var/cache/lighttpd/uploads\" )\n"
    "server.errorlog             = \"%s/lighttpd-error.log\"\n"
    "server.pid-file             = \"/var/run/lighttpd.pid\"\n"
    "server.username             = \"%s\"\n"
    "server.groupname            = \"%s\"\n"
    "server.port                 = %d\n"
    "\n"
    "accesslog.filename = \"%s/lighttpd-access.log\"\n"
    "\n"
    "index-file.names            = ( \"index.php\", \"index.html\", "
    "\"index.lighttpd.html\" )\n"
    "url.access-deny             = ( \"~\", \".inc\" )\n"
    "static-file.exclude-extensions = ( \".php\", \".pl\", \".fcgi\" )\n"
    "server.error-handler-404   = \"/index.html\"\n"
    "\n"
    "\n"
    "compress.cache-dir          = \"/var/cache/lighttpd/compress/\"\n"
    "compress.filetype           = ( \"application/javascript\", \"text/css\", "
    "\"text/html\", \"text/plain\" )\n"
    "\n"
    "fastcgi.debug = 1\n"
    "\n"
    "fastcgi.server = (\n"
    "  \"/surveyapi\" =>\n"
    "  (( \"host\" => \"127.0.0.1\",\n"
    "     \"port\" => %d,\n"
    "     \"bin-path\" => \"%s/surveyfcgi\",\n"
    "     \"bin-environment\" => (\n"
    "     \"SURVEY_HOME\" => \"%s\",\n"
    "     \"SURVEY_PYTHONDIR\" => \"%s\"\n"
    "     ),\n"
    "     \"check-local\" => \"disable\",\n"
    "     \"docroot\" => \"%s/front/build\" # remote server may use \n"
    "                      # its own docroot\n"
    "  ))\n"
    ")\n"
    "\n"
    "# default listening port for IPv6 falls back to the IPv4 port\n"
    "## Use ipv6 if available\n"
    "#include_shell \"/usr/share/lighttpd/use-ipv6.pl \" + server.port\n"
    "include_shell \"/usr/share/lighttpd/create-mime.assign.pl\"\n"
    "include_shell \"/usr/share/lighttpd/include-conf-enabled.pl\"\n";

time_t last_config_time = 0;
int configure_and_start_lighttpd(char *test_dir, int silent_flag) {
  int retVal = 0;

  do {
    // kill open ports
    stop_lighttpd();
    // Create config file
    char conf_data[16384];
    char cwd[1024];
    int cwdlen = 1024;

    if (!tests) {
      fprintf(stderr, "Pulling configuration together...\n");
    }

    // Make sure at least 10 seconds passes between restarts of the back end, so that we don't get
    // spurious errors.
    long long time_since_last = time(0) - last_config_time;
    if (time_since_last < 10) {
      sleep(11 - time_since_last);
    }
    last_config_time = time(0);

    // Create log file in test directory, and make it globally writeable
    {
      snprintf(conf_data, 16384, "%s/breakage.log", test_dir);
      FILE *f = fopen(conf_data, "w");
      if (f) {
        fclose(f);
      }
      chmod(conf_data, 0777);
    }

    if (!tests) {
      fprintf(stderr, "Created breakage.log\n");
    }

    // point python dir ALWAYS to test dir
    char python_dir[4096];
    snprintf(python_dir, 4096, "%s/python", test_dir);
    require_directory(python_dir, 0775);

    snprintf(conf_data, 16384, config_template, test_dir, getcwd(cwd, cwdlen),
             test_dir, LIGHTY_USER, LIGHTY_GROUP, HTTP_PORT, test_dir,
             SURVEYFCGI_PORT, test_dir, test_dir, python_dir,
             getcwd(cwd, cwdlen));
    char tmp_conf_file[1024];
    snprintf(tmp_conf_file, 1024, "%s/lighttpd.conf", test_dir);

    FILE *f = fopen(tmp_conf_file, "w");
    if (!f) {
      LOG_ERRORV("Failed to open temporary lighttpd.conf file: %s",
                 strerror(errno));
    }

    fprintf(f, "%s", conf_data);
    fclose(f);

    if (!tests) {
      fprintf(stderr, "Config file created.\n");
    }

    char cmd[2048];
    snprintf(cmd, 2048,
             "sudo cp %s/lighttpd.conf /etc/lighttpd/lighttpd.test.conf",
             test_dir);
    if (!tests) {
      fprintf(stderr, "Running '%s'\n", cmd);
    }
    if (system(cmd)) {
      LOG_ERRORV("system() call to install lighttpd.conf failed: %s",
                 strerror(errno));
    }

    snprintf(cmd, 2048, "sudo cp surveyfcgi %s/surveyfcgi", test_dir);
    if (!tests) {
      fprintf(stderr, "Running '%s'\n", cmd);
    }
    if (system(cmd)) {
      LOG_ERRORV("system() call to copy surveyfcgi failed: %s",
                 strerror(errno));
    }

    snprintf(cmd, 2048, "sudo lighttpd -f /etc/lighttpd/lighttpd.test.conf");
    if (!tests) {
      fprintf(stderr, "Running '%s'\n", cmd);
    }
    if (system(cmd)) {
      LOG_ERRORV("system() call to start lighttpd failed: %s", strerror(errno));
    }

    snprintf(
        cmd, 2048,
        "curl -s -o /dev/null -f http://localhost:%d/surveyapi/fastcgitest",
        HTTP_PORT);
    if (!tests) {
      fprintf(stderr, "Running '%s'\n", cmd);
    }

    int v = 0;
    while ((v = system(cmd)) != 0) {
      if (!silent_flag) {
        fprintf(stderr, "cmd returned code %d\n", v);
      }
      char log_dir[8192];
      // This should be /logs, but it doesn't exist yet, and we really only need it for the
      // ../ to get to breakage.log
      snprintf(log_dir, 8192, "%s/sessions", test_dir);
      if (!silent_flag) {
        dump_logs(log_dir, stderr);
      }
      sleep(2);
      continue;
    }

    if (!tests) {
      fprintf(stderr, "lighttpd is now responding to requests.\n");
    }
    if (!tests) {
      fprintf(stderr, "All done.\n");
    }

  } while (0);

  return 0;
}

int stop_lighttpd() {
  if (!tests)
    fprintf(stderr, "Stop lighttpd on port %d...\n", HTTP_PORT);

  FILE *fp;
  char lsof_cmd[2048];
  char kill_cmd[2048];
  char pidc[20];
  size_t hits = 0;

  snprintf(lsof_cmd, 2048, "sudo lsof -t -i:%d", HTTP_PORT);
  if (!tests) {
    fprintf(stderr, "Got lsof output\n");
  }

  fp = popen(lsof_cmd, "r");
  while (fgets(pidc, sizeof(pidc), fp) != NULL) {

    hits++;
    snprintf(kill_cmd, 2048, "sudo kill %s", pidc);
    if (!tests) {
      fprintf(stderr, " - running: %s\n", kill_cmd);
    }
    if (system(kill_cmd)) {
      fprintf(stderr, "system() call to stop lighttpd failed (kill %s)\n",
              pidc);
      exit(-3);
    }
  }
  fclose(fp);

  if (!hits) {
    if (!tests) {
      fprintf(stderr, "No pid to kill.\n");
    }
    return 0;
  }

  if (!tests) {
    fprintf(stderr, "Considering my options...\n");
  }
  sleep(1);
  if (!tests) {
    fprintf(stderr, "Done.\n");
  }

  return 0;
}

int main(int argc, char **argv) {
  snprintf(test_dir, 1024, "/tmp/surveytestrunner.%d.%d", (int)time(0),
           getpid());
  fprintf(stderr, "\e[33m *\e[0m Using \e[1m%s\e[0m as test directory\n",
          test_dir);
  fprintf(stderr, "\e[33m *\e[0m Using \e[1m%d\e[0m as http port\n", HTTP_PORT);

  require_directory(test_dir, 0755);

  // Make surveys, sessions and lock directories
  // Also make sure the survey directory is writable when running tests
  // (this is so accesstest will succeed.  For production, this can be
  // avoided by making sure to use the commandline tool to create a single
  // session in a survey after the survey has been modified, to make sure
  // all the necessary hash files get created.)
  require_test_directory("surveys", 0777);
  require_test_directory("sessions", 0777);
  require_test_directory("logs", 0777);
  require_test_directory("locks", 0777);
  // note: python dir is created when setting up lighttpd conf
  require_test_file("lighttpd-access.log", 0777);
  require_test_file("lighttpd-error.log", 0777);

  // Make sure we have a test log directory
  require_directory("testlog", 0755);

  stop_lighttpd();
  fprintf(stderr, "About to request config gets pulled together\n");
  // Make config file pointing to the temp_dir, and start the server
  if (configure_and_start_lighttpd(test_dir, 0)) {
    exit(-3);
  }

  fprintf(stderr, "\n");

  int passes = 0;
  int fails = 0;
  int errors = 0;
  int fatals = 0;

  for (int i = 1; i < argc; i++) {
    switch (run_test(test_dir, argv[i])) {
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

#if 0
  fix_ownership(test_dir);
  if (recursive_delete(test_dir)) {
    fprintf(stderr,"Error encountered while deleting temporary directories.\n");
  }
#endif

  stop_lighttpd(); // remove for debugging
  fprintf(stderr, "\n");
  fprintf(stderr,
          "Summary: %d/%d tests passed (%d failed, %d errors, %d fatalities "
          "during tests)\n",
          passes, tests, fails, errors, fatals);
  return 0;
}
