# To setup on linux:


## Modify backend/lighttpd.conf to correct paths

Modify the path after SURVEY_HOME to point to where you want the survey
data to live. You will need to create that directory and set the appropriate
ownership on it as well.

Also the two places where the document root is specified need to be corrected

## Setup where the fastcgi binary will live

```
sudo mkdir /var/www/fastcgi
sudo chown <your username> /var/www/fastcgi
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

But note that if you run testrun, it will replace /etc/lighttpd/lighttpd.conf with the
one in this directory -- so you should modify that file.
