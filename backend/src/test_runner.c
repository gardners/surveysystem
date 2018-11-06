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
  int retVal=0;
  FILE *in=NULL;
  FILE *log=NULL;
  char line[1024];
  
  do {
  
    // Get name of test file without path
    char *test_name=test_file;
    for(int i=0;test_file[i];i++) if (test_file[i]=='/') test_name=&test_file[i];
    
    in=fopen(test_file,"r");
    if (!in) {
      fprintf(stderr,"\nCould not open test file '%s' for reading",test_file);
      perror("fopen");
      retVal=-1; break;
    }

    // Read first line of test for description
    line[0]=0; fgets(line,1024,in);
    if (!line[0]) {
      fprintf(stderr,"\nFirst line of test definition must be description <description text>\n");
      retVal=-1; break;
    }
    char description[1024];
    if (sscanf(line,"description %[^\r\n]",description)!=1) {
      fprintf(stderr,"\nCould not parse description line of test.\n");
      retVal=-1; break;
    }

    fprintf(stderr,"\033[39m[    ]  \033[37m%s\033[39m",description); fflush(stderr);
    
    char testlog[1024];
    snprintf(testlog,1024,"testlog/%s.log",test_name);
    
    log=fopen(testlog,"w");

    if (!log) goto error;

    pass:

    fprintf(stderr,"\r\033[39m[\033[32mPASS\033[39m]  %s\n",description); fflush(stderr);
    break;
      
  fail:

    fprintf(stderr,"\r\033[39m[\033[31mFAIL\033[39m]  %s\n",description); fflush(stderr);
    retVal=1;
    break;

  error:

    fprintf(stderr,"\r\033[39m[\033[32;1;5mEROR\033[39;0m]  %s\n",description); fflush(stderr);
    retVal=2;
    break;
    
  fatal:

    fprintf(stderr,"\r\033[39m[\033[32;1;5mDEID\033[39;0m]  %s\n",description); fflush(stderr);
    retVal=3;
    break;

  } while(0);

  if (in) fclose(in);
  if (log) fclose(log);
    
  return retVal;
}

char *config_template=
  "server.modules = (\n"
  "	\"mod_access\",\n"
  "	\"mod_alias\",\n"
  "	\"mod_fastcgi\",\n"
  "	\"mod_compress\",\n"
  " 	\"mod_redirect\",\n"
  ")\n"
  "\n"
  "server.document-root        = \"%s/front/build\"\n"
  "server.upload-dirs          = ( \"/var/cache/lighttpd/uploads\" )\n"
  "server.errorlog             = \"/var/log/lighttpd/error.log\"\n"
  "server.pid-file             = \"/var/run/lighttpd.pid\"\n"
  "server.username             = \"www-data\"\n"
  "server.groupname            = \"www-data\"\n"
  "server.port                 = 80\n"
  "\n"
  "\n"
  "index-file.names            = ( \"index.php\", \"index.html\", \"index.lighttpd.html\" )\n"
  "url.access-deny             = ( \"~\", \".inc\" )\n"
  "static-file.exclude-extensions = ( \".php\", \".pl\", \".fcgi\" )\n"
  "server.error-handler-404   = \"/index.html\"\n"
  "\n"
  "\n"
  "compress.cache-dir          = \"/var/cache/lighttpd/compress/\"\n"
  "compress.filetype           = ( \"application/javascript\", \"text/css\", \"text/html\", \"text/plain\" )\n"
  "\n"
  "fastcgi.debug = 1\n"
  "\n"
  "fastcgi.server = (\n"
  "  \"/surveyapi\" =>\n"
  "  (( \"host\" => \"127.0.0.1\",\n"
  "     \"port\" => 9000,\n"
  "     \"bin-path\" => \"/var/www/fastcgi/surveyfcgi\",\n"
  "     \"bin-environment\" => (\n"
  "	\"SURVEY_HOME\" => \"%s\"\n"
  "	),\n"
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
  "include_shell \"/usr/share/lighttpd/include-conf-enabled.pl\"\n"
  ;

int configure_and_start_lighttpd(char *test_dir)
{ 
  // Create config file
  char conf_data[16384];
  char cwd[1024];
  int cwdlen=1024;
  snprintf(conf_data,16384,config_template,getcwd(cwd,cwdlen),test_dir,getcwd(cwd,cwdlen));
  char tmp_conf_file[1024];
  snprintf(tmp_conf_file,1024,"%s/lighttpd.conf",test_dir);
  FILE *f=fopen(tmp_conf_file,"w");
  if (!f) {
    perror("Failed to open temporary lighttpd.conf file");
    exit(-3);
  }
  fprintf(f,"%s",conf_data);
  fclose(f);
  char cmd[2048];
  snprintf(cmd,2048,"sudo -p 'Overwrite /etc/lighttpd/lighttpd.conf?' cp %s/lighttpd.conf /etc/lighttpd/lighttpd.conf",test_dir);
  fprintf(stderr,"WARNING: If you proceed, this test suite will overwrite /etc/lighttpd/lighttpd.conf.\n");
  if (system(cmd)) {
    perror("system() call to install lighttpd.conf failed");
    exit(-3);
  }
  fprintf(stderr,"Restarting lighttpd service...");
  snprintf(cmd,2048,"sudo -p 'Restart lighttpd ready for running tests?' service lighttpd restart");
  if (system(cmd)) {
    perror("system() call to restart lighttpd failed");
    exit(-3);
  }

  return 0;
}

int stop_lighttpd(void)
{
  fprintf(stderr,"Stop lighttpd service...");
  char cmd[2048];
  snprintf(cmd,2048,"sudo -p 'Stop lighttpd ready for running tests?' service lighttpd restart");
  if (system(cmd)) {
    perror("system() call to stop lighttpd failed");
    exit(-3);
  }
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

  // Make sure we have a test log directory
  mkdir("testlogs",0755);

  // Make config file pointing to the temp_dir, and start the server
  configure_and_start_lighttpd(test_dir);

  fprintf(stderr,"\n");
  
  for(int i=1;i<argc;i++) {
    run_test(argv[i]);
  }

  // Clean up after ourselves
  fprintf(stderr,"Cleaning up...\n");
  if (recursive_delete(test_dir)) {
    fprintf(stderr,"Error encountered while deleting temporary directories.\n");
  }

  stop_lighttpd();
  
}
