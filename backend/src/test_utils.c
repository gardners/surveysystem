#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fts.h>

#include "utils.h"
#include "survey.h"
#include "validators.h"
#include "test.h"

////
// bootsrap environment
////


/**
 * set up a sandboxed file system for each test
 */
void test_init_filesystem(char *sandbox_dir, int mode) {

  // remove old test records
  if (!access(sandbox_dir, F_OK)) {
    if(test_recursive_delete(sandbox_dir)) {
      fprintf(stderr, "FATAL: cannot remove old test dir '%s'\n", sandbox_dir);
      exit(-3);
    }
  }

  // create test root
  test_require_directory(sandbox_dir, 0755);

  // Make surveys, sessions and lock directories
  // Also make sure the survey directory is writable when running tests
  // (this is so accesstest will succeed.  For production, this can be
  // avoided by making sure to use the commandline tool to create a single
  // session in a survey after the survey has been modified, to make sure
  // all the necessary hash files get created.)
  char path[2048];

  snprintf(path, 2048, "%s/surveys", sandbox_dir);
  test_require_directory(path, 0775);

  snprintf(path, 2048, "%s/sessions", sandbox_dir);
  test_require_directory(path, 0775);

  snprintf(path, 2048, "%s/logs", sandbox_dir);
  test_require_directory(path, 0775);

  snprintf(path, 2048, "%s/locks", sandbox_dir);
  test_require_directory(path, 0775);

  // point python dir ALWAYS to test dir
  snprintf(path, 2048, "%s/python", sandbox_dir);
  test_require_directory(path, 0775);

  if(mode == TEST_MODE_HTTP) {
    snprintf(path, 2048, "%s/lighttpd-access.log", sandbox_dir);
    test_require_file(path, 0664, NULL);

    snprintf(path, 2048, "%s/lighttpd-error.log", sandbox_dir);
    test_require_file(path, 0664, NULL);

    snprintf(path, 2048, "%s/breakage.log", sandbox_dir);
    test_require_file(path, 0664, NULL);

    // create docroot for lighttpd, we do not use it, but it is required by lighttpd
    snprintf(path, 2048, "%s/www", sandbox_dir);
    test_require_directory(path, 0775);
  }

}


////
// local helpers
////

long long test_gettime_us() {
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

double test_time_delta(long long start_time) {
    double tdelta = test_gettime_us() - start_time;
    tdelta /= 1000;
    return tdelta;
}

////
// string replacement
////

/**
 * Replace pattern with string.
 */
void test_replace_str(char *src, char *search, char *replace, size_t sz) {
    char buffer[sz];
    char *p = src;

    size_t slen = strlen(search);
    size_t rlen = strlen(replace);

    int hits = 0;
    while ((p = strstr(p, search))) {
        hits++;
        p++;
    }

    if(!hits) {
        return;
    }

    // calculate new len
    p = src;
    size_t nlen = strlen(src) - (hits * slen);
    nlen += (hits * rlen) + 1;

    // trunc result if it exceeds sz
    nlen = (nlen > sz) ? sz : nlen;

    int prev;
    while ((p = strstr(p, search))) {
        prev = p - src;
        strncpy(buffer, src, prev);
        buffer[prev] = '\0';

        strncat(buffer, replace, rlen);
        strncat(buffer, p + slen, strlen(p) + slen);
        strncpy(src, buffer, nlen);
        p++;
    }
    src[nlen] = '\0';
}

/**
 * Replace pattern with int.
 */
void test_replace_int(char *str, char *pattern, int replacement, size_t sz) {
  char tmp[sz];
  snprintf(tmp, sz, "%d", replacement);
  test_replace_str(str, pattern, tmp, sz);
}

/**
 * parses value out of a function-like directive line, example "myfunctionname(my value)\n"
 */
int test_parse_fn_notation(char *line, char *name, char *out, size_t len) {

    if (!line || !name || !out) {
        return -1;
    }

    if (strlen(name) >= strlen(line)) {
        return -1;
    }

    if(strncmp(name, line, strlen(name))) {
        return -1;
    }

    out[0] = 0;
    size_t n = strlen(name);
    if(line[n] != '(') {
        return -1;
    }
    n++;

    size_t o = 0;
    for (int i = n; i < strlen(line); i++) {
        if (o >= len) {
            out[0] = 0;
            return -1;
        }
        if (line[i] == ')') {
            out[o] = 0;
            return 0;
        }
        out[o] = line[i];
        o++;
    }

    out[0] = 0;
    return -1;
}

////
// logs
////

int test_dump_logs(char *dir, FILE *log) {

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
          char line[TEST_MAX_LINE];
          line[0] = 0;
          fgets(line, TEST_MAX_LINE, in);

          while (line[0]) {
            fprintf(log, "%s", line);
            line[0] = 0;
            fgets(line, TEST_MAX_LINE, in);
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


////
// lighttpd
////


void test_start_lighttpd(struct Test *test, char *pid_file, char *user, char *group, int server_port, int fcgi_port) {
  char conf_path[1024];
  char userfile_path[1024] = { '\0' };
  char cmd[2048];

  // kill open ports
  test_stop_lighttpd(test->count == 1, server_port, fcgi_port);

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

  snprintf(cmd, 2048,
    // vars
    "sed -i                              \\"
    "-e 's|{BASE_DIR}|%s|g'              \\"
    "-e 's|{PID_FILE}|%s|g'              \\"
    "-e 's|{LIGHTY_USER}|%s|g'           \\"
    "-e 's|{LIGHTY_GROUP}|%s|g'          \\"
    "-e 's|{SERVER_PORT}|%d|g'           \\"
    "-e 's|{FCGIENV_MIDDLEWARE}|%s|g' \\"
    "-e 's|{FCGI_PORT}|%d|g'       \\"
    "-e 's|{DIGEST_USERFILE}|%s|g'       \\"
    // path
    "%s",
    // vars
    test->dir,
    pid_file,
    user,
    group,
    server_port,
    test->fcgienv_middleware,
    fcgi_port,
    userfile_path,
    // path
    conf_path
  );

  if (system(cmd)) {
    fprintf(stderr, "replacing template string in lighttpd.conf failed: %s",conf_path);
    exit(-3);
  }

  // #333 remove creating temp config in /etc/lighttpd,
  // sysymlink /etc/lighttpd/conf-enabled into test dir (required by mod_fcgi)
  char sym_path[1024];
  snprintf(sym_path, 1024, "%s/conf-enabled", test->dir);
  if (access(sym_path, F_OK)) {
    if(symlink("/etc/lighttpd/conf-enabled", sym_path)) {
      fprintf(stderr, "symlinking '/etc/lighttpd/conf-enabled' => '%s' failed, error: %s\n",
                  sym_path, strerror(errno));
      exit(-3);
    }
  }

  snprintf(cmd, 2048, "sudo cp surveyfcgi %s/surveyfcgi", test->dir);
  if (!test->count) {
    fprintf(stderr, "Running '%s'\n", cmd);
  }

  if (system(cmd)) {
    fprintf(stderr, "system() call to copy surveyfcgi failed: %s\n",
                strerror(errno));
    exit(-3);
  }

  snprintf(cmd, 2048, "sudo lighttpd -f %s", conf_path);
  if (test->count == 1) {
    fprintf(stderr, "Running '%s'\n", cmd);
  }

  if (system(cmd)) {
    fprintf(stderr, "system() call to start lighttpd failed: '%s', error: %s\n", cmd, strerror(errno));
    exit(-3);
  }

  snprintf(
      cmd, 2048,
      "curl -s -o /dev/null -f http://localhost:%d/surveyapi/fastcgitest",
      server_port);

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
      test_dump_logs(log_dir, stderr);
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
void test_stop_lighttpd(int verbose, int server_port, int fcgi_port) {

    char cmd[2048];
    char out[2048];
    FILE *fp;
    int status;

    // server

    snprintf(cmd, 2048, "sudo ./scripts/killport %d 2>&1", server_port);
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
      fprintf(stderr, "stopping lighttpd on port '%d' failed (exit code: %d)\n", server_port, WEXITSTATUS(status));
      exit(-3);
    }

    // fcgi

    snprintf(cmd, 2048, "sudo ./scripts/killport %d 2>&1", fcgi_port);
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
      fprintf(stderr, "stopping lighttpd on port '%d' failed (exit code: %d)\n", fcgi_port, WEXITSTATUS(status));
      exit(-3);
    }

    sleep(1);
}

////
// file system
////


/**
 * #333, get current pat to executable (for setting SURVEY_HOME)
 */
int test_get_current_path(int argc, char **argv, char *path_out, size_t max_len) {
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

void test_require_ownership(char *dir, char *user, char *group) {
  char cmd[TEST_MAX_LINE];
  snprintf(cmd, TEST_MAX_LINE, "sudo chown -R %s:%s %s\n", user, group, dir);
  if (system(cmd)) {
    fprintf(stderr, "command '%s' failed.\n", cmd);
    exit(-3);
  }
}

void test_require_file(char *path, int perm, char *data) {
  FILE *fp = fopen(path, (data) ? "w+" : "w");
  if (!fp) {
    fprintf(stderr, "test_require_file() fopen(%s) failed.", path);
    exit(-3);
  }

  if (data) {
    fputs(data, fp);
  }

  fclose(fp);

  if (chmod(path, perm)) {
    fprintf(stderr, "test_require_file() chmod(%s) failed.", path);
    exit(-3);
  }
}

void test_require_directory(char *path, int perm) {
  if (mkdir(path, perm)) {
    if (errno != EEXIST) {
      fprintf(stderr, "mkdir(%s, %o) failed.", path, perm);
      exit(-3);
    }
  }

  // bypassing umask
  if (chmod(path, perm)) {
    fprintf(stderr, "test_require_directory() chmod(%s) failed.", path);
    exit(-3);
  }
}


int test_recursive_delete(const char *dir) {
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
                "test_recursive_delete('%s') encountered a problem: fts_read error: %s\n",
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
        fprintf(stderr, "%s: Failed to remove: %s\n", curr->fts_path, strerror(errno));
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

////
// struct test config
////


/**
 * initialise test config with default values
 */
struct Test test_config_create(int server_port) {
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

  snprintf(test.fcgienv_middleware, 1024, "127.0.0.1(%d)", server_port);
  return test;
}


void test_config_print(FILE *in, struct Test *test) {
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

FILE *test_load_test_file_header(char *test_file, struct Test *test) {
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
    char line[TEST_MAX_LINE];
    fgets(line, TEST_MAX_LINE, fp);
    if (sscanf(line, "description %[^\r\n]", test->description) != 1) {
      fprintf(stderr, "\nCould not parse description line of test.\n");
      fclose(fp);
      return NULL;
    }

    while(fgets(line, TEST_MAX_LINE, fp) != NULL) {
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


void test_load_test_file(int test_count, int test_index, char *test_file, struct Test *test) {
    FILE *fp = fopen(test_file, "r");
    if (!fp) {
      fprintf(stderr, "\nCould not open test file '%s' for reading\n", test_file);
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
    char line[TEST_MAX_LINE];
    fgets(line, TEST_MAX_LINE, fp);
    if (sscanf(line, "@description %[^\r\n]", test->description) != 1) {
      fprintf(stderr, "\nCould not parse description line of test->\n");
      fclose(fp);
      exit(-3);
    }

    while(fgets(line, TEST_MAX_LINE, fp) != NULL) {

      if (line[0] != '@') {
        break; // end of config header
      }

      if (strncmp(line, "@skip!", 6) == 0) {
          test->skip = 1;
      }

      if (strncmp(line, "@usecors!", 19) == 0) {
          strncpy(test->lighty_template, "tests/config/lighttpd-cors.conf.tpl", 1024);
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

////
// sessions
////

/**
 * recusively scan files in directory and count file names who represent a valid session id
 */
int test_count_sessions(char *path) {
  int count = 0;
  DIR *dir;
  struct dirent *entry;

  if (!(dir = opendir(path))){
    fprintf(stderr, "test_count_sessions(): opendir failed for '%s'", path);
    return -1;
  }

  errno = 0;
  while ((entry = readdir(dir)) != NULL) {
      if (entry->d_type == DT_DIR) {
          if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
              continue;
          }

          char sub_path[1024];
          snprintf(sub_path, 1024, "%s/%s", path, entry->d_name);

          int r = test_count_sessions(sub_path);
          if (r < 0) {
            closedir(dir);
            return -1;
          }
          count += r;
      } else {
          if (!validate_session_id(entry->d_name)) {
            count ++;
          }
      }
  }

  closedir(dir);
  if (errno) {
    fprintf(stderr, "test_count_sessions(): readdir '%s' failed with error %d (%s)", path, errno, strerror(errno));
    return -1;
  }

  return count;
}


/**
 * copies current session to another location in test directory
 */
int test_copy_session(char *session_id, char *targ, struct Test *test) {
  char path[1024];

  // open target file
  trim_crlf(targ);
  if (!strlen(targ)) {
    fprintf(stderr, "No target file specified\n");
    return -1;
  }

  snprintf(path, 1024, "%s/%s", test->dir, targ);
  FILE *out = fopen(path, "w");
  if (!out) {
    fprintf(stderr, "Cannot open target file '%s'\n", path);
    return -1;
  }

  //  open session file
  snprintf(path, 1024, "%s/sessions/%c%c%c%c/%s", test->dir,
            session_id[0], session_id[1], session_id[2],
            session_id[3], session_id);

  FILE *in = fopen(path, "r");
  if (!in) {
    fclose(out);
    fprintf(stderr, "Cannot open session file '%s'\n", path);
    return -1;
  }

  // copy
  char c;
  c = fgetc(in);
  while(c != EOF) {
    fputc(c, out);
    c = fgetc(in);
  }

  fclose(in);
  fclose(out);

  return 0;
}

////
// test file parsing
////

/**
 * Replaces dedefined tokens with struct test values or survey.h variables
 */
void test_replace_tokens(char *line, struct Test *test, size_t len) {
    if (!test || !line) {
      fprintf(stderr, "test_replace_tokens(): one or all required args are NULL\n");
      return;
    }
    test_replace_int(line, "<UTIME>", (int)time(0), len);
    test_replace_str(line, "<FCGIENV_MIDDLEWARE>", test->fcgienv_middleware, len);
    test_replace_int(line, "<IDENDITY_CLI>", IDENDITY_CLI, len);
    test_replace_int(line, "<IDENDITY_HTTP_PUBLIC>", IDENDITY_HTTP_PUBLIC, len);
    test_replace_int(line, "<IDENDITY_HTTP_BASIC>", IDENDITY_HTTP_BASIC, len);
    test_replace_int(line, "<IDENDITY_HTTP_DIGEST>", IDENDITY_HTTP_DIGEST, len);
    test_replace_int(line, "<IDENDITY_HTTP_TRUSTED>", IDENDITY_HTTP_TRUSTED, len);
    test_replace_int(line, "<IDENDITY_UNKOWN>", IDENDITY_UNKOWN, len);

    // #379 session states
    test_replace_int(line, "<SESSION_NEW>", SESSION_NEW, len);
    test_replace_int(line, "<SESSION_OPEN>", SESSION_OPEN, len);
    test_replace_int(line, "<SESSION_FINISHED>", SESSION_FINISHED, len);
    test_replace_int(line, "<SESSION_CLOSED>", SESSION_CLOSED, len);

    // paths
    test_replace_str(line, "<TEST_DIR>", test->dir, len);
}

/**
 * compiles and saves a session from a 'define session' directive in a test file, replaces placeholders with values defined in struct Test
 */
int test_compile_session_definition(FILE *in, char *session_id, struct Test *test) {
  char session_dir[1024];
  char session_file[1024];
  char line[TEST_MAX_LINE];

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
  fgets(line, TEST_MAX_LINE, in);

  while (line[0]) {
    trim_crlf(line);

    test_replace_tokens(line, test, TEST_MAX_LINE);

    if (!strcmp(line, "endofsession")) {
      break;
    }

    fprintf(s, "%s\n", line);
    line[0] = 0;
    fgets(line, TEST_MAX_LINE, in);
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
enum DiffResult compare_session_line(char *session_line, char *comparison_line, struct Test *test, int server_port, int *row_count) {

  char left_line[TEST_MAX_LINE];
  char right_line[TEST_MAX_LINE];

  char *left;
  char *right;
  char *left_ptr;
  char *right_ptr;

  (*row_count) = 0;
  int pass;

  // do not mutate arg *strings
  strncpy(left_line, session_line, TEST_MAX_LINE);
  strncpy(right_line, comparison_line, TEST_MAX_LINE);

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
      snprintf(tmp, 16, "127.0.0.1(%d)", server_port);
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

    /* #379 session states */

    // validate <SESSION_NEW>
    if (!strcmp(right, "<SESSION_NEW>")) {
      int val = atoi(left);

      if (val != SESSION_NEW) {
        return DIFF_MISMATCH_TOKEN;
      }
      pass++;
    }

    // validate <SESSION_OPEN>
    if (!strcmp(right, "<SESSION_OPEN>")) {
      int val = atoi(left);

      if (val != SESSION_OPEN) {
        return DIFF_MISMATCH_TOKEN;
      }
      pass++;
    }

    // validate <SESSION_FINISHED>
    if (!strcmp(right, "<SESSION_FINISHED>")) {
      int val = atoi(left);

      if (val != SESSION_FINISHED) {
        return DIFF_MISMATCH_TOKEN;
      }
      pass++;
    }

    // validate <SESSION_CLOSED>
    if (!strcmp(right, "<SESSION_CLOSED>")) {
      int val = atoi(left);

      if (val != SESSION_CLOSED) {
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
int test_compare_session(FILE *sess, int skip_s, FILE *comp, int skip_c, struct Test *test, int server_port, FILE *log, long long start_time) {
  int retVal = 0;

  // Now go through reading lines from here and from the session file
  char session_line[TEST_MAX_LINE];
  char comparison_line[TEST_MAX_LINE];

  int c_count = 0; // comparsion line count
  int s_count = 0; // session line count
  int diff = DIFF_MATCH; // diff result

  char *s;
  char *c;

  // wind both files forward
  while (c_count < skip_c) {
    c = fgets(comparison_line, TEST_MAX_LINE, comp);
    c_count++;
  }

  while (s_count < skip_s) {
    s = fgets(session_line, TEST_MAX_LINE, sess);
    s_count++;
  }

  while (1) {
    // reset both containers, we need this for checks below
    session_line[0] = 0;
    comparison_line[0] = 0;

    // read left and right
    s = fgets(session_line, TEST_MAX_LINE, sess);
    c = fgets(comparison_line, TEST_MAX_LINE, comp);

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
    diff = compare_session_line(session_line, comparison_line, test, server_port, &row_count);
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
          "  [DIFF_MISMATCH_TOKEN] compare_session_line() <TOKEN> validation failed (session line %d row %d, comparsion line %d, code %d)\n", s_count, row_count, c_count, diff);
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

  if (retVal) {
    fprintf(log, "T+%4.3fms : FAIL : verifysession with %d errors.\n", test_time_delta(start_time), retVal);
    return -1;
  }

  fprintf(log, "T+%4.3fms : OK : session file matches comparsion: %d session lines read, %d lines compared.\n", test_time_delta(start_time), s_count, c_count);
  return 0;
}

////
// http
////

int parse_http_status(FILE *fp, struct HttpResponse *resp) {
    char buffer[TEST_MAX_LINE];
    char *token = NULL;

    char *line = fgets(buffer, TEST_MAX_LINE, fp);
    if (!line) {
      return -1;
    }

    token = strtok(line, " ");
    if(!token) {
        return -1;
    }
    if (strncmp("HTTP", token, 4)) {
        return -1;
    }
    token = strtok(NULL, " ");
    if(!token) {
        return -1;
    }
    resp->status = atoi(token);
    return (resp->status) ? 0 : -1;
}

int parse_http_headers(FILE *fp, struct HttpResponse *resp) {
    char buffer[TEST_MAX_LINE];
    char *val;

    while (fgets(buffer, TEST_MAX_LINE, fp) != 0) {
        trim_crlf(buffer);
        if(buffer[0] == 0) {
            return 0;
        }
        if (resp->contentType[0] == 0 && strncmp(buffer, "Content-Type: ", 14) == 0) {
            val = buffer;
            val += 14;
            strncpy(resp->contentType, val, 1024);
            continue;
        }
        if (resp->eTag[0] == 0 && strncmp(buffer, "ETag: ", 6) == 0) {
            val = buffer;
            val += 6;
            strncpy(resp->eTag, val, 1024);
            continue;
        }
    }
    return 0;
}

int parse_http_body(FILE *fp, struct HttpResponse *resp) {
    char buffer[TEST_MAX_LINE];
    resp->line_count = 0;

    while (fgets(buffer, TEST_MAX_LINE, fp) != 0) {
      trim_crlf(buffer);
      // trailing newline
      if(buffer[0] == 0) {
          return 0;
      }
      strncpy(resp->lines[resp->line_count], buffer, TEST_MAX_LINE);
      resp->lines[resp->line_count][TEST_MAX_LINE - 1] = 0;

      resp->line_count++;

      if (resp->line_count >= TEST_MAX_LINE_COUNT) {
        fprintf(stderr, "parse_http_body(), max line count %d exceeded, content truncated!\n", TEST_MAX_LINE_COUNT);
        return -1;
      }
    }
    return 0;
}

/**
 * parses a saved curl request, contaning headers and body: `curl --silent --include --output /tmp/response.out www.example.com`
 */
int test_parse_http_response(FILE *fp, struct HttpResponse *resp) {
    if (!fp) {
        fprintf(stderr, "Failed to parse response: file pointer is NULL\n");
        return -1;
    }
    int ret = 0;

    // http status
    ret = parse_http_status(fp, resp);
    if (ret) {
        fprintf(stderr, "Failed to parse http status\n");
        return -1;
    }

    ret = parse_http_headers(fp, resp);
    if (ret) {
        fprintf(stderr, "Failed to parse headers\n");
        return -1;
    }

    ret = parse_http_body(fp, resp);
    if (ret) {
        fprintf(stderr, "Failed to parse body\n");
        return -1;
    }

    return 0;
}

void test_dump_http_response(FILE *fp, struct HttpResponse *resp) {
    if (!fp) {
        fprintf(stderr, "Failed to dump response: file pointer is NULL\n");
        return ;
    }

    fprintf(
      fp,
      "response {\n"
      "  status: \"%d\"\n"
      "  contentType: \"%s\"\n"
      "  eTag \"%s\"\n"
      "  line_count: \"%d\"\n\n",

      resp->status,
      resp->contentType,
      resp->eTag,
      resp->line_count
    );

    for (int i = 0; i < resp->line_count; i++) {
      fprintf(fp, "  lines[%d]: '%s'\n", i, resp->lines[i]);
    }
    fprintf(fp, "}\n");
}


////
// misc
///

/**
 * Execute a given command via popen, captures stdout and returns exit code
 */
int test_run_process(char *cmd, char *out, size_t len) {
    FILE *fp;
    char ch;

    if(!cmd || !out) {
      fprintf(stderr, "argument error\n");
      return -1;
    }

    fp = popen(cmd,"r");
    if(fp == NULL){
        fprintf(stderr,"Unable to open process for command '%s\n", cmd);
        return -1;
    }

    size_t i = 0;
    do {
      if (i > len - 1) {
        break;
      }
      ch = fgetc(fp);
      if(feof(fp)) {
         break ;
      }
      out[i++] = ch;
   } while(1);

   int status = pclose(fp);

  return WEXITSTATUS(status);
}
