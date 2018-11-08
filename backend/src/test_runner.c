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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fts.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <regex.h>

#include "survey.h"

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

long long gettime_us()
{
  long long retVal = -1;

  do 
  {
    struct timeval nowtv;

    // If gettimeofday() fails or returns an invalid value, all else is lost!
    if (gettimeofday(&nowtv, NULL) == -1)
    {
      fprintf(stderr,"\nFATAL: gettimeofday returned -1");
      exit(-3);
    }

    if (nowtv.tv_sec < 0 || nowtv.tv_usec < 0 || nowtv.tv_usec >= 1000000)
    {
      fprintf(stderr,"\nFATAL: gettimeofday returned invalid value");
      exit(-3);
    }

    retVal = nowtv.tv_sec * 1000000LL + nowtv.tv_usec;
  }
  while (0);

  return retVal;
}


int run_test(char *dir, char *test_file)
{
  int retVal=0;
  FILE *in=NULL;
  FILE *log=NULL;
  char line[8192];

  int response_line_count=0;
  char response_lines[100][8192];
  char last_sessionid[100]="";
  
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
    line[0]=0; fgets(line,8192,in);
    if (!line[0]) {
      fprintf(stderr,"\nFirst line of test definition must be description <description text>\n");
      retVal=-1; break;
    }
    char description[8192];
    if (sscanf(line,"description %[^\r\n]",description)!=1) {
      fprintf(stderr,"\nCould not parse description line of test.\n");
      retVal=-1; break;
    }

    fprintf(stderr,"\033[39m[    ]  \033[37m%s\033[39m",description); fflush(stderr);
    
    char testlog[1024];
    snprintf(testlog,1024,"testlog/%s.log",test_name);
    
    log=fopen(testlog,"w");

    if (!log) {
      fprintf(stderr,"\rFATAL: Could not create test log file '%s' for test '%s'                                    \n",
	      testlog,test_file);
      goto error;
    }

    time_t now=time(0);
    char *ctime_str=ctime(&now);
    fprintf(log,"Started running test at %s",ctime_str);
    long long start_time = gettime_us();
    
    char surveyname[8192]="";
    int expected_result=200;
    char url[65536];
    char glob[65536];
    double tdelta;
    
    // Now iterate through test script
    line[0]=0; fgets(line,8192,in);
    while(line[0]) {
      int len=strlen(line);
      // Trim CR/LF from the end of the line
      while(len&&(line[len-1]<' ')) line[--len]=0;

      tdelta=gettime_us()-start_time; tdelta/=1000;
      if (line[0]&&line[0]!='#')
	fprintf(log,"T+%4.3fms : Executing directive '%s'\n",tdelta,line);
      
      if (sscanf(line,"definesurvey %[^\r\n]",surveyname)==1) {
	// Read survey definition and create survey file
	char survey_file[8192];

	snprintf(survey_file,8192,"%s/surveys/%s",dir,surveyname);
	mkdir(survey_file,0777);
	if (chmod(survey_file,S_IRUSR|S_IWUSR|S_IXUSR|
		  S_IRGRP|S_IWGRP|S_IXGRP|
		  S_IROTH|S_IWOTH|S_IXOTH)) {
	  fprintf(stderr,"\nERROR: chmod() failed for new survey directory %s",survey_file);
	  goto error;
	}
	
	snprintf(survey_file,8192,"%s/surveys/%s/current",dir,surveyname);	
	FILE *s=fopen(survey_file,"w");
	if (!s) {
	  fprintf(stderr,"\rERROR: Could not create survey file '%s'                                                               \n",
		  survey_file);
	  goto error;
	}
	line[0]=0; fgets(line,8192,in);    
	while(line[0]) {
	  int len=strlen(line);
	  // Trim CR/LF from the end of the line
	  while(len&&(line[len-1]<' ')) line[--len]=0;

	  if (!strcmp(line,"endofsurvey")) break;

	  fprintf(s,"%s\n",line);

	  line[0]=0; fgets(line,8192,in);    
	}
	
	fclose(s);
      }
      else if (!strcmp(line,"extract_sessionid")) {
	if (response_line_count!=1) {
	  tdelta=gettime_us()-start_time; tdelta/=1000;
	  fprintf(log,"T+%4.3fms : FAIL : Could not parse session ID: Last response contained %d lines, instead of exactly 1.\n",
		  tdelta,response_line_count);
	  goto fail;
	}
	if (validate_session_id(response_lines[0]))
	  {
	    tdelta=gettime_us()-start_time; tdelta/=1000;
	    fprintf(log,"T+%4.3fms : FAIL : Could not parse session ID: validate_session_id() reported failure.\n",tdelta);
	    goto fail;
	  }
	// Remember session ID for other directives
	strcpy(last_sessionid,response_lines[0]);
	tdelta=gettime_us()-start_time; tdelta/=1000;
	fprintf(log,"T+%4.3fms : Session ID is '%s'\n",tdelta,last_sessionid);
      }
      else if (sscanf(line,"match %[^\r\n]",glob)==1) {
	// Check that the response contains the supplied pattern
	regex_t regex;
	int matches=0;
	int error_code=regcomp(&regex,glob,REG_EXTENDED|REG_NOSUB);
	if (error_code) {
	  char err[8192]="";
	  regerror(error_code,&regex,err,8192);
	  tdelta=gettime_us()-start_time; tdelta/=1000;
	  fprintf(log,"T+%4.3fms : FATAL : Could not compile regular expression: %s\n",
		  tdelta,err);
	  goto fatal;
	}
	for(int i=0;i<response_line_count;i++) {
	  if (REG_NOMATCH!=regexec(&regex,response_lines[i],0,NULL,0)) {
	    matches++;
	  }
	}
	if (!matches) {
	  tdelta=gettime_us()-start_time; tdelta/=1000;
	  fprintf(log,"T+%4.3fms : FAIL : No match for regular expression.\n",tdelta);
	  goto fail;
	}
      }
      else if (sscanf(line,"nomatch %[^\r\n]",glob)==1) {
	// Check that the response contains the supplied pattern
	regex_t regex;
	int matches=0;
	int error_code=regcomp(&regex,glob,REG_EXTENDED|REG_NOSUB);
	if (error_code) {
	  char err[8192]="";
	  regerror(error_code,&regex,err,8192);
	  tdelta=gettime_us()-start_time; tdelta/=1000;
	  fprintf(log,"T+%4.3fms : FATAL : Could not compile regular expression: %s\n",
		  tdelta,err);
	  goto fatal;
	}
	for(int i=0;i<response_line_count;i++) {
	  if (REG_NOMATCH!=regexec(&regex,response_lines[i],0,NULL,0)) {
	    matches++;
	  }
	}
	if (matches) {
	  tdelta=gettime_us()-start_time; tdelta/=1000;
	  fprintf(log,"T+%4.3fms : FAIL : Regular expression matches %d times.\n",tdelta,matches);
	  goto fail;
	}
      }
      else if (sscanf(line,"request %d %[^\r\n]",&expected_result,url)==2) {
	// Exeucte wget call. If it is a newsession command, then remember the session ID
	// We also log the returned data from the request, so that we can look at that
	// as well, if required.
	char cmd[65536];
	snprintf(cmd,65536,"%s/request.out",dir); unlink(cmd);
	snprintf(cmd,65536,"%s/request.code",dir); unlink(cmd);
	snprintf(cmd,65536,"curl -s -w \"HTTPRESULT=%%{http_code}\" -o %s/request.out http://localhost/surveyapi/%s > %s/request.code",
		 dir,url,dir);
	tdelta=gettime_us()-start_time; tdelta/=1000;
	fprintf(log,"T+%4.3fms : HTTP API request command: '%s'\n",tdelta,cmd);
	int shell_result=system(cmd);
	if (shell_result) {
	  tdelta=gettime_us()-start_time; tdelta/=1000;
	  fprintf(log,"T+%4.3fms : HTTP API request command returned with non-zero status %d.\n",tdelta,shell_result);
	}

	int httpcode=-1;
	tdelta=gettime_us()-start_time; tdelta/=1000;
	fprintf(log,"T+%4.3fms : HTTP API request command completed.\n",tdelta);
	FILE *rc=NULL;

	snprintf(cmd,65536,"%s/request.out",dir);
	rc=fopen(cmd,"r");
	if (!rc) {
	  tdelta=gettime_us()-start_time; tdelta/=1000;
	  fprintf(log,"T+%4.3fms : NOTE : Could not open '%s/request.out'. No response from web page?\n",tdelta,dir);
	} else {
	  tdelta=gettime_us()-start_time; tdelta/=1000;
	  fprintf(log,"T+%4.3fms : HTTP request response body:\n",tdelta);
	  line[0]=0; fgets(line,8192,rc);
	  while (line[0]) {
	    int len=strlen(line);
	    // Trim CR/LF from the end of the line
	    while(len&&(line[len-1]<' ')) line[--len]=0;

	    fprintf(log,"::: %s\n",line);
	    
	    // Keep the lines returned from the request in case we want to probe them
	    if (response_line_count<100) {
	      line[8191]=0;
	      strcpy(response_lines[response_line_count++],line);
	    }
	    
	    line[0]=0; fgets(line,8192,rc);	    
	  }
	  fclose(rc);
	}
       
	snprintf(cmd,65536,"%s/request.code",dir);
	rc=fopen(cmd,"r");
	if (!rc) {
	  tdelta=gettime_us()-start_time; tdelta/=1000;
	  fprintf(log,"T+%4.3fms : FATAL : Could not open '%s/request.code' to retrieve HTTP response code.\n",tdelta,dir);
	  goto fatal;
	} else {
	  tdelta=gettime_us()-start_time; tdelta/=1000;
	  fprintf(log,"T+%4.3fms : HTTP request.code response:\n",tdelta);
	  line[0]=0; fgets(line,1024,rc);
	  while (line[0]) {
	    int len=strlen(line);
	    // Trim CR/LF from the end of the line
	    while(len&&(line[len-1]<' ')) line[--len]=0;

	    sscanf(line,"HTTPRESULT=%d",&httpcode);
	    
	    fprintf(log,">>> %s\n",line);
	    line[0]=0; fgets(line,1024,rc);	    
	  }
	  fclose(rc);
	}
	tdelta=gettime_us()-start_time; tdelta/=1000;
	fprintf(log,"T+%4.3fms : HTTP response code %d\n",tdelta,httpcode);
	if (httpcode==-1) {
	  tdelta=gettime_us()-start_time; tdelta/=1000;
	  fprintf(log,"T+%4.3fms : FATAL : Could not find HTTP response code in request.code file.\n",tdelta);
	  goto fatal;
	}
	if (httpcode!=expected_result) {
	  tdelta=gettime_us()-start_time; tdelta/=1000;
	  fprintf(log,"T+%4.3fms : ERROR : Expected HTTP response code %d, but got %d.\n",
		  tdelta,expected_result,httpcode);
	  goto fail;
	}	
      }
      else if (!strcmp(line,"verify_session")) {
	if (validate_session_id(last_sessionid)) {
	  tdelta=gettime_us()-start_time; tdelta/=1000;
	  fprintf(log,"T+%4.3fms : FATAL : No session ID has been captured. Use extract_sessionid following request directive.\n",tdelta);
	  goto fatal;
	}

	// Build path to session file
	char session_file[8192];
	snprintf(session_file,8192,"%s/sessions/%c%c%c%c/%s",
		 test_dir,
		 last_sessionid[0],last_sessionid[1],last_sessionid[2],last_sessionid[3],
		 last_sessionid);
	tdelta=gettime_us()-start_time; tdelta/=1000;
	fprintf(log,"T+%4.3fms : Examining contents of session file '%s'.\n",tdelta,session_file);	

	// Check that the file exists
	FILE *s=fopen(session_file,"r");
	if (!s) {
	  tdelta=gettime_us()-start_time; tdelta/=1000;
	  fprintf(log,"T+%4.3fms : FAIL : Could not open session file: %s\n",tdelta,strerror(errno));
	  goto fail;
	}
	fclose(s);
      }
      else if (line[0]==0) {
	// Ignore blank lines
      }
      else if (line[0]=='#') {
	// Ignore comments
      }
      else {
	fprintf(log,"T+%4.3fms : FATAL : Test script '%s' has unknown directive '%s'\n",test_file,line);
	goto fatal;
      }
      line[0]=0; fgets(line,1024,in);    
    }
    
    //    pass:

    fprintf(stderr,"\r\033[39m[\033[32mPASS\033[39m]  %s\n",description); fflush(stderr);
    break;
      
  fail:

    fprintf(stderr,"\r\033[39m[\033[31mFAIL\033[39m]  %s\n",description); fflush(stderr);
    retVal=1;
    break;

  error:

    fprintf(stderr,"\r\033[39m[\033[31;1;5mEROR\033[39;0m]  %s\n",description); fflush(stderr);
    retVal=2;
    break;
    
  fatal:

    fprintf(stderr,"\r\033[39m[\033[31;1;5mDIED\033[39;0m]  %s\n",description); fflush(stderr);
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
  snprintf(cmd,2048,"sudo -p 'Overwrite /etc/lighttpd/lighttpd.conf? Type your password to continue: ' cp %s/lighttpd.conf /etc/lighttpd/lighttpd.conf",test_dir);
  fprintf(stderr,"WARNING: If you proceed, this test suite will overwrite /etc/lighttpd/lighttpd.conf.\n");
  if (system(cmd)) {
    perror("system() call to install lighttpd.conf failed");
    exit(-3);
  }

  fprintf(stderr,"Stopping lighttpd service...\n");
  snprintf(cmd,2048,"sudo -p 'Restart lighttpd ready for running tests? Type your password to continue: ' service lighttpd restart");
  if (system(cmd)) {
    perror("system() call to restart lighttpd failed");
    exit(-3);
  }  

  fprintf(stderr,"Updating surveyfcgi binary...\n");
  snprintf(cmd,2048,"sudo -p 'Overwrite /var/www/fastcgi/surveyfcgi? Type your password to continue: ' cp surveyfcgi /var/www/fastcgi/surveyfcgi");
  fprintf(stderr,"WARNING: If you proceed, this test suite will overwrite /var/www/fastcgi/surveycgi.\n");
  if (system(cmd)) {
    perror("system() call to install lighttpd.conf failed");
    exit(-3);
  }
  
  fprintf(stderr,"Restarting lighttpd service...\n");
  snprintf(cmd,2048,"sudo -p 'Restart lighttpd ready for running tests? Type your password to continue: ' service lighttpd restart");
  if (system(cmd)) {
    perror("system() call to start lighttpd failed");
    exit(-3);
  }
  fprintf(stderr,"Waiting for lighttpd to finish restarting...\n");
  while(system("curl -s -o /dev/null -f http://localhost/surveyapi/fastcgitest")) continue;
  fprintf(stderr,"lighttpd is now responding to requests.\n");

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
  if (mkdir(tmp,0777)) {
    perror("mkdir() failed for test/surveys directory");
    exit(-3);
  }
  // Also make sure the survey directory is writable when running tests
  // (this is so accesstest will succeed.  For production, this can be
  // avoided by making sure to use the commandline tool to create a single
  // session in a survey after the survey has been modified, to make sure
  // all the necessary hash files get created.)
  if (chmod(tmp,S_IRUSR|S_IWUSR|S_IXUSR|
	    S_IRGRP|S_IWGRP|S_IXGRP|
	    S_IROTH|S_IWOTH|S_IXOTH)) {
    fprintf(stderr,"\nERROR: chmod() failed for new survey directory %s",tmp);
    exit(-3);
  }
  
  snprintf(tmp,2048,"%s/sessions",test_dir);
  if (mkdir(tmp,0777)) {
    perror("mkdir() failed for test/sessions directory");
    exit(-3);
  }
  // Now make sessions directory writeable by all users
  if (chmod(tmp,S_IRUSR|S_IWUSR|S_IXUSR|
	    S_IRGRP|S_IWGRP|S_IXGRP|
	    S_IROTH|S_IWOTH|S_IXOTH)) {
    perror("chmod() failed for test/sessions directory");
    exit(-3);
  }

  // Make sure we have a test log directory
  mkdir("testlogs",0755);

  // Make config file pointing to the temp_dir, and start the server
  configure_and_start_lighttpd(test_dir);

  fprintf(stderr,"\n");

  int passes=0;
  int fails=0;
  int errors=0;
  int fatals=0;
  int tests=0;
  for(int i=1;i<argc;i++) {
    switch (run_test(test_dir,argv[i])) {
    case 0: passes++; break;
    case 1: fails++; break;
    case 2: errors++; break;
    case 3: fatals++; break;
    }
    tests++;
  }

  // Clean up after ourselves
  fprintf(stderr,"Cleaning up...\n");
#if 0
  if (recursive_delete(test_dir)) {
    fprintf(stderr,"Error encountered while deleting temporary directories.\n");
  }
#endif

  stop_lighttpd();
  fprintf(stderr,"\n");
  fprintf(stderr,"Summary: %d/%d tests passed (%d failed, %d errors, %d fatalities during tests)\n",
	  passes,tests,fails,errors,fatals);
  return 0;
}
