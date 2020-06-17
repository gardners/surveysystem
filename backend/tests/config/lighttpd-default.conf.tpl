# default ligtthpd config for tests

## variables

var.base_dir="{BASE_DIR}"
var.pid_file="{PID_FILE}"
var.lighty_user="{LIGHTY_USER}"
var.lighty_group="{LIGHTY_GROUP}"
var.server_port="{HTTP_PORT}"
var.surveyfcgi_port="{SURVEYFCGI_PORT}"

## config

server.modules = (
     "mod_access",
     "mod_alias",
     "mod_fastcgi",
     "mod_compress",
     "mod_redirect",
     "mod_accesslog",
)

server.breakagelog          = base_dir + "/breakage.log"
server.upload-dirs          = ("/var/cache/lighttpd/uploads")
server.errorlog             = base_dir + "/lighttpd-error.log"
server.pid-file             = pid_file
server.document-root        = base_dir + "www"
server.username             = lighty_user
server.groupname            = lighty_group
server.port                 = var.server_port

accesslog.filename          = base_dir + "/lighttpd-access.log"

index-file.names            = ( "index.php", "index.html", "index.lighttpd.html" )
url.access-deny             = ( "~", ".inc" )
static-file.exclude-extensions = ( ".php", ".pl", ".fcgi" )
server.error-handler-404   = "/index.html"

compress.cache-dir          = "/var/cache/lighttpd/compress/"
compress.filetype           = ( "application/javascript", "text/css", "text/html", "text/plain" )

fastcgi.debug = 1

fastcgi.server = (
  "/surveyapi" => ((
        "host" => "127.0.0.1",
        "port" => surveyfcgi_port,
        "max-procs" => 1,
        "bin-path" => base_dir + "/surveyfcgi",
        "bin-environment" => (
            "SURVEY_HOME" => base_dir,
            "SURVEY_PYTHONDIR" => base_dir + "/python",
            ## --- DO NOT USE ON PRODUCTION: set below var only for test envs ---
            "SURVEY_FORCE_PYINIT" => "1",
     ),
     "check-local" => "disable",
     # remote server may use its own docroot
     "docroot" => base_dir
  ))
)

# default listening port for IPv6 falls back to the IPv4 port
## Use ipv6 if available
#include_shell "/usr/share/lighttpd/use-ipv6.pl " + server.port
include_shell "/usr/share/lighttpd/create-mime.assign.pl"
include_shell "/usr/share/lighttpd/include-conf-enabled.pl"
