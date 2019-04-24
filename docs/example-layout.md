# Directory structure

Example for a simple surveysystem layout

```bash
/var/surveyproject
└── surveysystem                 # suveysystem home folder (git clone)
    ├── backend                  # fastcgi.server: location for environment var SURVEY_HOME
    │   ├── logs                 # backend logs
    │   ├── python               # location for Python controller scripts
    |   |   └── nextquestion.py  # optional: Python controller script
    │   ├── sessions             # session files with answer data!
    │   ├── surveys
    |   |   └── mysurvey
    |   |       └── current      # questions manifest )
    │   └── surveyfcgi           # location of fcgi bin (bin-path)
    └── front
        └── build                # frontend entry dir (server.document-root)
```

Example layout for a more complex surveysystem with linked python controller libraries

```bash
/var/surveyproject
├── auth                         # optional: auth scripts/databases
├── install                      # optional: install scripts
├── surveys                      # python controllers
│   └── mysurvey                 # a single Python controller example
│       └── engine
│           └── current          # Survey questions manifest - to be linked into backend (see below)
|   └── nextquestion.py          # python entry script - to be linked into backend (see below)
└── surveysystem                 # suveysystem home folder (git clone)
    ├── backend                  # fastcgi.server: location for environment var SURVEY_HOME
    │   ├── logs                 # backend logs
    │   ├── python               # location for Python controller scripts
    |   |   └──> nextquestion.py # symlink to "mysurvey" controller script (see above)
    │   ├── sessions             # session files with answer data!
    │   ├── surveys
    |   |   └── mysurvey
    |   |       └──> current     # symlink to "mysurvey" controller questions manifest (see above)
    │   └── surveyfcgi           # location of fcgi bin (bin-path)
    └── front
        └── build                # frontend entry dir (server.document-root)
```

# Lighttpd configs

a simplified configuration without authentication and ssl

```bash
server.modules = (
     "mod_access",
     "mod_alias",
     "mod_fastcgi",
     "mod_compress",
     "mod_redirect",
)

server.document-root        = "/var/surveyproject/surveysystem/backend/front/build"
server.upload-dirs          = ( "/var/cache/lighttpd/uploads" )
server.errorlog             = "/var/log/lighttpd/error.log"
server.pid-file             = "/var/run/lighttpd.pid"
server.username             = "www-data"
server.groupname            = "www-data"
server.port                 = 80


index-file.names            = ( "index.php", "index.html", "index.lighttpd.html" )
url.access-deny             = ( "~", ".inc" )
static-file.exclude-extensions = ( ".php", ".pl", ".fcgi" )
server.error-handler-404   = "/index.html"


compress.cache-dir          = "/var/cache/lighttpd/compress/"
compress.filetype           = ( "application/javascript", "text/css", "text/html", "text/plain" )

fastcgi.debug = 1

fastcgi.server = (
  "/surveyapi" =>
  (( "host" => "127.0.0.1",
     "port" => 9000,
     "bin-path" => "/var/surveyproject/surveysystem/backend/surveyfcgi",
     "bin-environment" => (
        "SURVEY_HOME" => "/var/surveyproject/surveysystem/backend"
     ),
     "check-local" => "disable",
     "docroot" => "/var/surveyproject/surveysystem/backend/front/build" # remote server may use its own docroot
  ))
)

# default listening port for IPv6 falls back to the IPv4 port
## Use ipv6 if available
#include_shell "/usr/share/lighttpd/use-ipv6.pl " + server.port
include_shell "/usr/share/lighttpd/create-mime.assign.pl"
include_shell "/usr/share/lighttpd/include-conf-enabled.pl"
```
