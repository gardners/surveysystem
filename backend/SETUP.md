# To setup on linux:


## Create /etc/lighttpd/lighttpd.conf  with the following contents:

```
server.modules = (
	"mod_access",
	"mod_alias",
	"mod_fastcgi",
	"mod_compress",
 	"mod_redirect",
)

server.document-root        = "/var/www/html"
server.upload-dirs          = ( "/var/cache/lighttpd/uploads" )
server.errorlog             = "/var/log/lighttpd/error.log"
server.pid-file             = "/var/run/lighttpd.pid"
server.username             = "www-data"
server.groupname            = "www-data"
server.port                 = 80


index-file.names            = ( "index.php", "index.html", "index.lighttpd.html" )
url.access-deny             = ( "~", ".inc" )
static-file.exclude-extensions = ( ".php", ".pl", ".fcgi" )

compress.cache-dir          = "/var/cache/lighttpd/compress/"
compress.filetype           = ( "application/javascript", "text/css", "text/html", "text/plain" )

fastcgi.debug = 1

fastcgi.server = (
  "/survey" =>
  (( "host" => "127.0.0.1",
     "port" => 9000,
     "bin-path" => "/var/www/fastcgi/surveyfcgi",
     "bin-environment" => (
	"SURVEY_HOME" => "/home/paul/Projects/survey/surveysystem/backend"
	),
     "check-local" => "disable",
     "docroot" => "/home/paul/Projects/survey/surveysystem/backend" # remote server may use 
                      # its own docroot
  ))
)

# default listening port for IPv6 falls back to the IPv4 port
## Use ipv6 if available
#include_shell "/usr/share/lighttpd/use-ipv6.pl " + server.port
include_shell "/usr/share/lighttpd/create-mime.assign.pl"
include_shell "/usr/share/lighttpd/include-conf-enabled.pl"
```

Modify the path after SURVEY_HOME to point to where you want the survey
data to live. You will need to create that directory and set the appropriate
ownership on it as well.


## Setup where the fastcgi binary will live

```
sudo mkdir /var/www/fastcgi
cp surveyfcgi /var/www/fastcgi
```

You will need to make sure that the 

## Stop and re-start lighttpd

```
service lighttpd stop ; service lighttpd start
```

## Testing

For convenience for testing, you can use the testrun script, which will recompile,
reinstall the fastcgi binary, and restart things.
