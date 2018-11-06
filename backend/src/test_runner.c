/*
  Simple test runner for the survey system.
  It runs tests that are each defined by a single file.
  The test specification is as follows:

  description <description>
  definesurvey <name>
  <questions in normal question format>
  endofsurvey
  request <expected response code> <url path and query>
  verifysession
  <expected set of answers in the session file. Can be empty>
  endofsession

  These commands can be used more than once, so that more complex activities can be scripted.

  Apart from running these scripts, all that it has to do to is to setup and cleanup the
  directories for running the tests, and setup the config file for lighttpd for testing, and
  actually stop and start lighttpd.

*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fts.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

char test_dir[1024];

// From https://stackoverflow.com/questions/2256945/removing-a-non-empty-directory-programmatically-in-c-or-c
int recursive_delete(const char *dir)
{
  int ret = 0;
  FTS *ftsp = NULL;
  FTSENT *curr;
  
  // Cast needed (in C) because fts_open() takes a "char * const *", instead
  // of a "const char * const *", which is only allowed in C++. fts_open()
  // does not modify the argument.
  char *files[] = { (char *) dir, NULL };
  
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
      fprintf(stderr, "%s: fts_read error: %s\n",
	      curr->fts_accpath, strerror(curr->fts_errno));
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
	fprintf(stderr, "%s: Failed to remove: %s\n",
		curr->fts_path, strerror(errno));
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

int run_test(char *test_file)
{
  return 0;
}

int main(int argc,char **argv)
{
  snprintf(test_dir,1024,"/tmp/survey_test_runner_%d_%d",(int)time(0),getpid());
  fprintf(stderr,"Using %s as test directory\n",test_dir);
  if (mkdir(test_dir,0755)) {
    perror("mkdir() failed for test directory");
    exit(-3);
  }
  // Make surveys and sessions directories
  char tmp[2048];
  snprintf(tmp,2048,"%s/surveys",test_dir);
  if (mkdir(tmp,0755)) {
    perror("mkdir() failed for test/surveys directory");
    exit(-3);
  }
  snprintf(tmp,2048,"%s/sessions",test_dir);
  if (mkdir(tmp,0777)) {
    perror("mkdir() failed for test/sessions directory");
    exit(-3);
  }
  // Now make sessions directory writeable by all users
  if (chmod(tmp,S_IRUSR|S_IWUSR|S_IXUSR|
	    S_IROTH|S_IWOTH|S_IXGRP|
	    S_IRUSR|S_IWUSR|S_IXOTH)) {
    perror("chmod() failed for test/sessions directory");
    exit(-3);
  }
	
  for(int i=1;i<argc;i++) {
    run_test(argv[i]);
  }

  // Clean up after ourselves
  fprintf(stderr,"Cleaning up...\n");
  if (recursive_delete(test_dir)) {
    fprintf(stderr,"Error encountered while deleting temporary directories.\n");
  }
  
}
