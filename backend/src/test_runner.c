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
 * request <expected_status> <method> /<url_path?query> <curl_args>
 * request proxy <expected_status> <method> /<url_path?query> <curl_args>
 *
 * extract_sessionid
 *
 * match <regex to search for in body of last response>
 *
 * nomatch <regex to search for in body of last response>
 *
 * definesession <session id>
 * <session headers>
 * <session answers>
 * endofsession
 *
 * verifysession
 * <expected set of answers in the session file. Can be empty>
 * endofsession
 *
 * session_add_answer <serialised answer>
 *
 * verify_session_id <existing session id>
 *
 * verify_sessionfiles_count <expected_number>
 *
 * verify_file_exists /<path>/<to>/<file> (check if file exists within test dir)
 *
 * copy_session_to <file path>
 *
 * create_checksum(<string and/or token>)
 *
 * verify_response_etag(<string and/or token>)
 *
 * verify_response_content_type(<string>)
 *
 * open_file(<path>)\n <contents> close_file()\n
 * - writes content into file '<path>'
 * - path replacements: <TEST_DIR>: path to current test
 *
 * python\n (code)\n endofpython\n
 *  - block for executing Python nextquestion() and analyse() hook functions>
 *  - traceback and logging core modules are available and pre-configured
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
#include <dirent.h>

#include "errorlog.h"
#include "survey.h"
#include "validators.h"
#include "utils.h"
#include "sha1.h"

#include "test.h"

#define SERVER_PORT 8099
#define SURVEYFCGI_PORT 9009
#define LIGHTY_USER "www-data"
#define LIGHTY_GROUP "www-data"
#define LIGHTY_PIDFILE "/var/run/lighttpd.pid"

// test counter
int tests = 0;

// python traceback function for nextquestion
char *py_module_head =
    "import traceback\n"
    "import logging\n"
    "\n"
    "logging.basicConfig(\n"
    "    filename='./logs/python.log',\n"
    "    level=logging.DEBUG,\n"
    "    format='%(asctime)s [%(levelname)s] %(message)s',\n"
    "    datefmt='%m/%d/%Y %I:%M:%S %p'\n"
    ")"
    "\n\n\n"
    "def cmodule_traceback(exc_type, exc_value, exc_tb):\n"
    "    lines = []\n"
    "    lines = traceback.format_exception(exc_type, exc_value, exc_tb)\n"
    "    output = '\\n'.join(lines)\n"
    "    return output\n"
    "\n";

/**
 * parses a 'request' directive line for defined patterns and creates a curl command
 * returns expected http status
 */
int parse_request(char *line, char *out_file, char *result, size_t sz, FILE *log) {

  enum { REQ__START, REQ_PROXY, REQ_STATUS, REQ_METHOD, REQ_URI, REQ__REST };

  int proxy = 0;
  int status = 0;

  char method[256] = { 0 };
  char uri[2048] = { 0 };
  char rest[TEST_MAX_BUFFER] = "";

  if (sz < TEST_MAX_BUFFER) {
    fprintf(log, "Error: allocated size for result string to small %ld < TEST_MAX_BUFFER'\n", sz);
    return 0;
  }

  if (!line) {
    fprintf(log, "Error: directive line is NULL'\n");
    return 0;
  }

  fprintf(log, "  * ");

  int key = REQ__START;
  char *ctx = line;
  char *tok = strtok_r(line, " ", &ctx);

  while (tok != NULL) {

    if (key > REQ_URI) {
      break;
    }

    // trim multiple whitespaces
    while(*tok && *tok == ' ') {
      tok++;
    }

    // validate
    if (key == REQ__START) {
      fprintf(log, "REQ_START: '%s', ", tok);
      if(strcmp(tok, "request")) {
        strncpy(result, "Error parsing REQ_DIRECTIVE: expected 'request'\n", sz);
        return 0;
      }
    }

    // optional
    if (key == REQ_PROXY) {
      if(strcmp(tok, "proxy") == 0) {
        proxy = 1;
      } else {
        key++; // optional, advance
      }
      fprintf(log, "REQ_PROXY: '%s', ", (proxy) ? "yes": "no");
    }

    // validate int
    if (key == REQ_STATUS) {
      fprintf(log, "REQ_STATUS: '%s', ", tok);
      status = atoi(tok);
      if(!status) {
        strncpy(result, "Error parsing REQ_STATUS: is not an int\n", sz);
        return 0;
      }
    }

    if (key == REQ_METHOD) {
      fprintf(log, "REQ_METHOD: '%s', ", tok);
      strncpy(method, tok, 256);
    }

    if (key == REQ_URI) {
      fprintf(log, "REQ_URI: '%s', ", tok);
      if(tok[0] != '/') {
        strncpy(result, "Error parsing REQ_URI: 'not starting starting with '/'\n", sz);
        return 0;
      }
      strncpy(uri, tok, 2048);
      break; // LAST!
    }

    key++;
    tok = strtok_r(NULL, " ", &ctx);

  } // while

  if (ctx) {
    fprintf(log, "REQ_REST: '%s'", ctx);
    strncpy(rest, ctx, TEST_MAX_BUFFER);
  }

  if (!strlen(method))  {
    strncpy(result, "Error parsing REQ_METHOD: not found\n", sz);
    return 0;
  }

  if (!strlen(uri))  {
    strncpy(result, "Error parsing REQ_URI: not found\n", sz);
    return 0;
  }

  // workaround for Lighttpd quirk for data requests with empty bodys: "HTTP/1.1 411 Length Required"
  //  - lighttpd:request.c, seee: https://github.com/lighttpd/lighttpd1.4/blob/80848d370807f2f8253ed5622d9f6585a02068c8/src/request.c#L1125
  if (!strcmp(method, "POST") || !strcmp(method, "PUT") || !strcmp(method, "DELETE") || !strcmp(method, "PATCH")) {

    int fix_411 = 1;

    // has data curlargs or content length header
    if (strstr(rest, "Content-Length:") || strstr(rest, "-d ") || strstr(rest, "--data ") || strstr(rest, "--data-binary ")) {
      fix_411 = 0;
    }

    // inserting -d "" arg will trigger curl to insert content length header (0)
    if (fix_411) {
      strncat(rest, " -d \"\"", TEST_MAX_BUFFER);
    }

  }

  fprintf(log, "\n");

  // build curl command
  int len = snprintf(result, sz, "curl --include --silent --request %s %s \"http://localhost:%d/surveyapi%s\" --output %s",
    method,
    rest,
    SERVER_PORT,
    uri,
    out_file
  );

  if (len < 0) {
    strncpy(result, "Error parsing REQ_URI: snprintf failed\n", sz);
    return 0;
  }
  return status;
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

  char tmp[TEST_MAX_LINE];
  snprintf(tmp, TEST_MAX_LINE, "%s/session.cpy", test->dir);
  unlink(tmp);

  snprintf(tmp, TEST_MAX_LINE, "sudo cp %s %s/session.cpy", session_file, test->dir);
  if(system(tmp)) {
    fprintf(stderr, "Command failed: '%s'\n", tmp);
    return NULL;
  }

  // Check that the file exists
  snprintf(tmp, TEST_MAX_LINE, "%s/session.cpy", test->dir);
  FILE *copy = fopen(tmp, "r");
  if (!copy) {
    fprintf(stderr, "Could not open session file '%s': %s\n", tmp, strerror(errno));
    return NULL;
  }

  return copy;
}

/**
 * loads test files, scans directives and runs single tests
 */
int run_test(struct Test *test) {
  // #198 flush errors accumulated by previous tests
  clear_errors();

  struct HttpResponse response;
  memset(&response, 0, sizeof(response));

  int retVal = 0;
  FILE *in = NULL;
  FILE *log = NULL;
  char line[TEST_MAX_LINE];
  char log_dir[1024];

  // writables for processing keywords
  char tmp[1024];
  char cmd[1024];

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
    test_config_print(log, test);
    fprintf(log, "\n");

    // starting test
    fprintf(stderr, "\033[39m[    ]  \033[37m%s : %s\033[39m", test->name, test->description);
    fflush(stderr);

    time_t now = time(0);
    char *ctime_str = ctime(&now);
    fprintf(log, "Started running test at %s", ctime_str);
    long long start_time = test_gettime_us();

    char surveyname[1024] = "";
    char custom_sessionid[1024] = "";
    char custom_checksum[1024] = "";
    char glob[TEST_MAX_BUFFER];

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
    fgets(line, TEST_MAX_LINE, in);

    while (line[0]) {

      int len = strlen(line);
      // Trim CR/LF from the end of the line
      while (len && (line[len - 1] < ' ')) {
        line[--len] = 0;
      }

      if (line[0] && line[0] != '@' && line[0] != '#') {
        fprintf(log, "T+%4.3fms : Executing directive '%s'\n", test_time_delta(start_time), line);
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
        fgets(line, TEST_MAX_LINE, in);

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
          fgets(line, TEST_MAX_LINE, in);
        }

        fclose(s);
      } else if (sscanf(line, "definesession %[^\r\n]", custom_sessionid) == 1) {

        ////
        // keyword: "definesession"
        ////

        // register session id
        trim_crlf(custom_sessionid);
        strcpy(last_sessionid, custom_sessionid);

        if(test_compile_session_definition(in, custom_sessionid, test)) {
          fprintf(stderr, "\nERROR: test_compile_session_definition() failed for custom sessionid %s", custom_sessionid);
          goto error;
        }
      } else if (sscanf(line, "verify_session_id %[^\r\n]", tmp) == 1) {

        ////
        // keyword: "verify_session_id"
        ////

        char path[1024];
        snprintf(path, 1024, "%s/sessions/%c%c%c%c/%s", test->dir,
            tmp[0], tmp[1], tmp[2],
            tmp[3], tmp);

        if (access(path, F_OK)) {
          fprintf(log, "T+%4.3fms : ERROR : session '%s'. file does not exist, path: '%s'\n", test_time_delta(start_time), tmp, path);
          goto error;
        }

      } else if (sscanf(line, "verify_sessionfiles_count %[^\r\n]", tmp) == 1) {

        ////
        // keyword: "verify_sessions_count"
        ////

        int left = atoi(tmp);
        tmp[0] = 0;

        char sess_path[1024];
        snprintf(sess_path, 1024, "%s/%s", test->dir, "sessions");

        int right = test_count_sessions(sess_path);

        if(right < 0) {
          fprintf(log, "T+%4.3fms : ERROR : verify_sessions_count failed: test_count_sessions() returned an error\n", test_time_delta(start_time));
          goto error;
        }
        if(left != right) {
          fprintf(log, "T+%4.3fms : ERROR : verify_sessions_count failed: %d sessions expected, %d found\n", test_time_delta(start_time), left, right);
          goto error;
        }

      } else if (sscanf(line, "verify_file_exists %[^\r\n]", tmp) == 1) {

        ////
        // keyword: "verify_file_exists"
        ////

        char file_path[1024];
        snprintf(file_path, 1024, "%s%s", test->dir, tmp); // tmp needs to start with '/'!
        tmp[0] = 0;

        char session_prefix[5] = {last_sessionid[0], last_sessionid[1], last_sessionid[2], last_sessionid[3], '\0'};

        test_replace_str(file_path, "<session_id>", last_sessionid, TEST_MAX_BUFFER);
        test_replace_str(file_path, "<session_prefix>", session_prefix, TEST_MAX_BUFFER);

        int err = access(file_path, R_OK);
        if (err) {
          fprintf(log, "T+%4.3fms : ERROR : verify_file_exists failed for path '%s'\n", test_time_delta(start_time), file_path);
          goto error;
        }

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
          fprintf(log, "T+%4.3fms : ERROR : Could not set permissions on python directory '%s'\n", test_time_delta(start_time), python_dir);
          goto error;
        }

        // create python package (__ini.py__)
        char python_ini[2048];
        snprintf(python_ini, 2048, "%s/__init__.py", python_dir);

        FILE *s = fopen(python_ini, "w");
        if (!s) {
          fprintf(log, "T+%4.3fms : ERROR : Could not create python file '%s'\n", test_time_delta(start_time), python_ini);
          goto error;
        }
        fclose(s);

        if (chmod(python_ini, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
          fprintf(log, "T+%4.3fms : ERROR : Could not set permissions on python  file '%s'\n", test_time_delta(start_time), python_ini);
          goto error;
        }

        // create python module (nextquestion.py)
        char python_module[2048];
        snprintf(python_module, 2048, "%s/nextquestion.py", python_dir);
        s = fopen(python_module, "w");
        if (!s) {
          fprintf(log, "T+%4.3fms : ERROR : Could not create python file '%s'\n", test_time_delta(start_time), python_module);
          goto error;
        }

        fprintf(s, "%s\n", py_module_head);

        // write python content from testfile
        line[0] = 0;
        fgets(line, TEST_MAX_LINE, in);
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
          fgets(line, TEST_MAX_LINE, in);
        } // endwhile
        fclose(s);

        if (chmod(python_module, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {

          fprintf(log, "T+%4.3fms : ERROR : Could not set permissions on python file '%s'\n", test_time_delta(start_time), python_module);
          goto error;
        }

        // Compile the python
        char cmd[1024];
        snprintf(cmd, 1024, "python3 -m compileall %s 2>&1 >>%s",
                 python_module, test->log);
        int compile_result = system(cmd);

        if (compile_result) {
          fprintf(log, "T+%4.3fms : FATAL: Failed to compile python module. Does the python have errors?\n", test_time_delta(start_time));
          goto fatal;
        }

        // Then restart, to clear out any old python code we had loaded before
        fprintf(log, "T+%4.3fms : INFO : Restarting backend to clear loaded python code\n",test_time_delta(start_time));

        // #361, removed extra call to configure_and_start_lighttpd()
        //  give filesystem and python fs time to cope with newly created python file,
        //  we had random ModuleNotFoundErrors on tests with both no python and python
        sleep(1);
        fprintf(log, "T+%4.3fms : INFO : Backend restart complete\n", test_time_delta(start_time));

      } else if (!strcmp(line, "extract_sessionid")) {

        ////
        // keyword: "extract_sessionid"
        ////

        if (response.line_count != 1) {
          fprintf(log,
            "T+%4.3fms : FAIL : Could not parse session ID: Last response contained %d lines, instead of exactly 1.\n",
              test_time_delta(start_time), response.line_count);
          goto fail;
        }

        if (validate_session_id(response.lines[0])) {
          fprintf(log, "T+%4.3fms : FAIL : Could not parse session ID:  validate_session_id() reported failure.\n", test_time_delta(start_time));
          goto fail;
        }

        // Remember session ID for other directives
        strcpy(last_sessionid, response.lines[0]);
        fprintf(log, "T+%4.3fms : Session ID is '%s'\n", test_time_delta(start_time), last_sessionid);

      } else if (sscanf(line, "match_string %[^\r\n]", glob) == 1) {

        ////
        // keyword: "match_string"
        ////

        if(!strcmp("<empty>", glob)) {
            if (response.line_count) {
               fprintf(log, "T+%4.3fms : FAIL : Not empty (%d lines found).\n", test_time_delta(start_time), response.line_count);
            }
        } else {
            // Check that the response contains the supplied pattern
            int matches = 0;
            for (int i = 0; i < response.line_count; i++) {
                if (strstr(response.lines[i], glob)) {
                    matches++;
                }
            }

            if (!matches) {
                fprintf(log, "T+%4.3fms : FAIL : No match for literal string.\n", test_time_delta(start_time));
                goto fail;
            }
        }

      } else if (sscanf(line, "nomatch_string %[^\r\n]", glob) == 1) {

        ////
        // keyword: "nomatch_string %[^\r\n]"
        ////

        // Check that the response contains the supplied pattern
        int matches = 0;
        for (int i = 0; i < response.line_count; i++) {
          if (strstr(response.lines[i], glob)) {
            matches++;
          }
        }

        if (matches) {
          fprintf(log, "T+%4.3fms : FAIL : There are matches for literal string.\n", test_time_delta(start_time));
          goto fail;
        }

      } else if (sscanf(line, "for %s = %d to %d", var, &first, &last) == 3) {

        ////
        // keyword: "for $ = $ to $"
        ////

        if (for_count > 10) {
          fprintf(log, "T+%4.3fms : FATAL: Too many FOR statements\n", test_time_delta(start_time));
          goto fatal;
        }

        if (strlen(var) > 15) {
          fprintf( log, "T+%4.3fms : FATAL: Variable name too long in FOR statement\n",  test_time_delta(start_time));
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
          fprintf(log, "T+%4.3fms : FATAL: NEXT without FOR\n", test_time_delta(start_time));
          goto fatal;
        }

        if (for_prevval[for_count - 1] == for_last[for_count - 1]) {
          // No need to loop back
          for_count--;
        } else {
          for_prevval[for_count - 1] += for_step[for_count - 1];

          if (fseeko(in, for_seek_pos[for_count - 1], SEEK_SET)) {
            fprintf(log,
              "T+%4.3fms : ERROR : Could not seek to top of FOR %s loop at offset %lld\n",
              test_time_delta(start_time), for_var[for_count - 1],
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
          char err[TEST_MAX_LINE] = "";
          regerror(error_code, &regex, err, TEST_MAX_LINE);
          fprintf( log, "T+%4.3fms : FATAL: Could not compile regular expression: %s\n", test_time_delta(start_time), err);
          goto fatal;
        }

        for (int i = 0; i < response.line_count; i++) {
          if (REG_NOMATCH != regexec(&regex, response.lines[i], 0, NULL, 0)) {
            matches++;
          }
        }

        if (!matches) {
          fprintf(log, "T+%4.3fms : FAIL: No match for regular expression.\n", test_time_delta(start_time));
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
          char err[TEST_MAX_LINE] = "";
          regerror(error_code, &regex, err, TEST_MAX_LINE);
          fprintf(log, "T+%4.3fms : FATAL: Could not compile regular expression: %s\n", test_time_delta(start_time), err);
          goto fatal;
        }

        for (int i = 0; i < response.line_count; i++) {
          if (REG_NOMATCH != regexec(&regex, response.lines[i], 0, NULL, 0)) {
            matches++;
          }
        }

        if (matches) {
          fprintf(log, "T+%4.3fms : FAIL: There is a match to the regular expression.\n", test_time_delta(start_time));
          goto fail;
        }
      } else if (sscanf(line, "copy_session_to %[^\r\n]", arg) == 1) {

        ////
        // keyword: "copy_session_to"
        ////

        if (test_copy_session(last_sessionid, arg, test)) {
          fprintf(log, "T+%4.3fms : FAIL: Copy session file failed.\n", test_time_delta(start_time));
          goto fail;
        }

        arg[0] = 0;

      } else if (strncmp("request", line, 7) == 0) {

        ////
        // keyword: "request"
        ////

        // Exeucte curl call. If it is a newsession command, then remember the session ID
        // We also log the returned data from the request, so that we can look at that
        // as well, if required.
        char cmd[TEST_MAX_BUFFER];
        char out_file[1024];
        int http_status = 0;

        // Delete any old version of files laying around
        snprintf(out_file, 1024, "%s/request.out", test->dir);
        unlink(tmp);

        http_status = parse_request(line, out_file, cmd, TEST_MAX_BUFFER, log);

        if (!http_status) {
          fprintf(log, "T+%4.3fms : FATAL: failed parsing directive '%s', code: %d, error: '%s'\n", test_time_delta(start_time), line, http_status, cmd);
          goto fatal;
        }

        test_replace_str(cmd, "<session_id>", last_sessionid, TEST_MAX_BUFFER);
        test_replace_str(cmd, "<custom_checksum>", custom_checksum, TEST_MAX_BUFFER);
        test_replace_str(cmd, "<response_etag>", response.eTag, TEST_MAX_BUFFER);
        test_replace_tokens(cmd, test, TEST_MAX_BUFFER);

        fprintf(log, "  * curl command: '%s'\n", cmd);

        /*
         * now clear previous response !
         */
        memset(&response, 0, sizeof(response));

        int shell_result = system(cmd);
        if (shell_result) {
          fprintf(log, "T+%4.3fms : HTTP API request command (curl) returned error %d.\n", test_time_delta(start_time), shell_result);
        }
        fprintf(log, "T+%4.3fms : HTTP API request command completed.\n", test_time_delta(start_time));

        // load and parse request.out file
        FILE * rc = fopen(out_file, "r");
        if (!rc) {
          // not going to fatal, this might be a test case
          fprintf(log, "T+%4.3fms : NOTE : Could not open '%s/request.out'. No response from server ?\n", test_time_delta(start_time), test->dir);
        } else {
          if (test_parse_http_response(rc, &response)) {
            fprintf(log, "T+%4.3fms : FATAL: Could not parse HttpResponse from '%s/request.out'\n", test_time_delta(start_time), cmd);
            fclose(rc);
            goto fatal;
          }
          fclose(rc);
        } // endif !rc

        // log body
        fprintf(log, "T+%4.3fms : HTTP response body:\n", test_time_delta(start_time));
        for (int i = 0; i < response.line_count; i++) {
          fprintf(log,"::: %s\n", response.lines[i]);
        }
        fprintf(log, "T+%4.3fms : HTTP response status %d\n", test_time_delta(start_time), response.status);

        if (response.status != http_status) {
          fprintf(log, "T+%4.3fms : ERROR : Expected HTTP response code %d, but got %d.\n", test_time_delta(start_time), http_status, response.status);
          goto fail;
        }

      } else if (test_parse_fn_notation(line, "create_checksum", tmp, 1024) == 0) {

        ////
        // keyword: "create_checksum(%s)", #268
        ////

        test_replace_str(tmp, "<session_id>", last_sessionid, 1024); // parse optional $SESSION
        test_replace_tokens(tmp, test, 1024); // parse optional tokens

        snprintf(cmd, 1024, "echo -n '%s' | sha1sum", tmp);

        char csout[1024];
        int cstat = test_run_process(cmd, csout, 1024);

        if (cstat) {
          fprintf(log, "T+%4.3fms : ERROR : generate_checksum: failed! command: '%s', return code %d, stdout: ''%s'\n", test_time_delta(start_time), cmd, cstat, csout);
          goto fail;
        }

        strncpy(custom_checksum, strtok(csout, " "), 1024);
        fprintf(log, "T+%4.3fms : generated custom_checksum: '%s', from string '%s' (return code %d).\n", test_time_delta(start_time), custom_checksum, tmp, cstat);
        tmp[0] =0;

      } else if (test_parse_fn_notation(line, "open_file", tmp, 1024) == 0) {

        ////
        // keyword: "open_file(%s)"\n ...contents\n close_file
        ////


        test_replace_tokens(tmp, test, 2048); // parse optional tokens
        FILE *df = fopen(tmp, "w");
        if(!df) {
          fprintf(log, "T+%4.3fms : ERROR : open_file: failed! could  not open file '%s' for write\n", test_time_delta(start_time), tmp);
          goto fail;
        }

        // write following lines into file
        int clines = 0;

        line[0] = 0;
        fgets(line, TEST_MAX_LINE, in);

        while (line[0]) {
          trim_crlf(line);

          test_replace_tokens(line, test, TEST_MAX_LINE);

          if (!strcmp(line, "close_file()")) {
            break;
          }

          fprintf(df, "%s\n", line);
          line[0] = 0;
          clines++;
          fgets(line, TEST_MAX_LINE, in);
        }

        fclose(df);
        fprintf(log, "::: %d lines written to file '%s'\n", clines, tmp);

        tmp[0] = 0;

      } else if (test_parse_fn_notation(line, "verify_response_content_type", tmp, 1024) == 0) {

        ////
        // keyword: "verify_response_content_type(%s)"
        ////

        if (strncmp(tmp, response.contentType, 1024)) {
            fprintf(log, "T+%4.3fms : ERROR :  verify_response_content_type() failed: '%s' != '%s'\n", test_time_delta(start_time), response.contentType, tmp);
            goto fail;
        }

        fprintf(log, "T+%4.3fms : verify_response_etag('%s' == '%s') passed,  (response.eTag)\n", test_time_delta(start_time), tmp, response.eTag);
        tmp[0] = 0;

      } else if (test_parse_fn_notation(line, "verify_response_etag", tmp, 1024) == 0) {

        ////
        // keyword: "verify_response_etag(%s)", #268
        ////

        if (!strncmp(tmp, "<custom_checksum>", 1024)) {

          if (strncmp(custom_checksum, response.eTag, HASHSTRING_LENGTH + 1)) {
            fprintf(log, "T+%4.3fms : ERROR :  custom_checksum: verify_response_etag('%s' == '%s') failed\n", test_time_delta(start_time), custom_checksum, response.eTag);
            goto fail;
          }

        } else if (!strncmp(tmp, "<hashlike_etag>", 1024)) {

          if (sha1_validate_string_hashlike(response.eTag)) {
            fprintf(log, "T+%4.3fms : ERROR :  validate etag hashlike: verify_response_etag(has hash) '%s'failed\n", test_time_delta(start_time), response.eTag);
            goto fail;
          }

        } else {
          if (strncmp(tmp, response.eTag, HASHSTRING_LENGTH + 1)) {
            fprintf(log, "T+%4.3fms : ERROR : stringarg: verify_response_etag('%s' == '%s')failed\n", test_time_delta(start_time), tmp, response.eTag);
            goto fail;
          }

        }

        fprintf(log, "T+%4.3fms : verify_response_etag('%s' == '%s') passed,  (response.eTag)\n", test_time_delta(start_time), tmp, response.eTag);
        tmp[0] = 0;


      } else if (sscanf(line, "session_add_answer %[^\r\n]", arg) == 1) {

        ////
        // keyword: "session_add_answer"
        ////

       if (validate_session_id(last_sessionid)) {
          fprintf(log,
            "T+%4.3fms : FATAL: (session_add_answer) No session ID has been captured. Use extract_sessionid following request directive.\n",
            test_time_delta(start_time));
          goto fatal;
        }

        trim_crlf(arg);
        if (arg[0] == 0) {
          fprintf(log,
            "T+%4.3fms : FATAL: (session_add_answer) no answer defined.\n",
            test_time_delta(start_time));
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
          fprintf(log,
            "T+%4.3fms : FATAL: (verify_session) No session ID has been captured. Use extract_sessionid following request directive.\n",
            test_time_delta(start_time));
          goto fatal;
        }

        FILE *s = open_session_file_copy(last_sessionid, test);
        if(!s) {
          fprintf(log, "T+%4.3fms : Create copy of session file failed: '%s/%s.cpy'.\n", test_time_delta(start_time), test->dir, last_sessionid);
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
          char tmp[TEST_MAX_LINE];
          int count = 0;
          while (fgets(tmp, TEST_MAX_LINE, s)) {
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

        int ret = test_compare_session(s, skip_s, in, skip_c, test, SERVER_PORT, log, start_time);
        if (s) {
          fclose(s);
        }

        if (ret) {
          fprintf(log, "T+%4.3fms : compare session failed: '%s'.\n", test_time_delta(start_time), last_sessionid);
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
            test_time_delta(start_time), test->file, line);
        goto fatal;
      }

      line[0] = 0;
      fgets(line, 1024, in);

    } // endwhile line[0]

    //    pass:
    if (log) {
      test_dump_logs(log_dir, log);
    }

    fprintf(stderr, "\r\033[39m[\033[32mPASS\033[39m] \033[37m(%d/%d)\033[39m %s : %s\n", test->index, test->count, test->name, test->description);
    fflush(stderr);
    break;

  skip:

    fprintf(stderr, "\r\033[39m[\033[33mSKIP\033[39m] \033[37m(%d/%d)\033[39m %s : %s\n", test->index, test->count, test->name, test->description);
    fflush(stderr);
    break;

  fail:

    if (log) {
      test_dump_logs(log_dir, log);
    }

    fprintf(stderr, "\r\033[39m[\033[31mFAIL\033[39m] \033[37m(%d/%d)\033[39m %s : %s\n", test->index, test->count, test->name, test->description);
    fflush(stderr);
    retVal = 1;
    break;

  error:

    if (log) {
      test_dump_logs(log_dir, log);
    }

    fprintf(stderr, "\r\033[39m[\033[31;1;5mERROR\033[39;0m] \033[37m(%d/%d)\033[39m %s : %s\n", test->index, test->count, test->name, test->description);
    fflush(stderr);
    retVal = 2;
    break;

  fatal:

    if (log) {
      test_dump_logs(log_dir, log);
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

int main(int argc, char **argv) {
  // #333, we are using some backend functions in our test code who rely on generate_path,
  // for this to work we need to set SURVEY_HOME for this executable,
  // this is indepenent to fastcgi SURVEY_HOME
  char current_path[1024];
  if (test_get_current_path(argc, argv, current_path, 1024)) {
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
  test_require_directory("testlog", 0755);

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

    struct Test test = test_config_create(SERVER_PORT);
    test_load_test_file(argc - 1, i, argv[i], &test);

    // test_config_print(stderr, &test);
    test_init_filesystem(test.dir, TEST_MODE_HTTP);
    test_require_ownership(test.dir, LIGHTY_USER, LIGHTY_GROUP);

    test_start_lighttpd(&test, LIGHTY_PIDFILE, LIGHTY_USER, LIGHTY_GROUP, SERVER_PORT, SURVEYFCGI_PORT);

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

  test_stop_lighttpd(argc == 2, SERVER_PORT, SURVEYFCGI_PORT);

  fprintf(stderr,
    "\n--------------\n"
    "- Summary: %d/%d tests passed (%d failed, %d errors, %d fatalities)\n"
    "--------------\n",
    passes, tests, fails, errors, fatals);
  return 0;
}
