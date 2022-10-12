nginx-keyval
============

This nginx module creates variables with values taken from key-value pairs.

> This modules is heavenly inspired by the nginx original
> [http_keyval_module](https://nginx.org/en/docs/http/ngx_http_keyval_module.html).

Installation
------------

### Build install

``` sh
$ : "clone repository"
$ git clone https://github.com/kjdev/nginx-keyval
$ cd nginx-keyval
$ : "get nginx source"
$ NGINX_VERSION=1.x.x # specify nginx version
$ wget http://nginx.org/download/nginx-${NGINX_VERSION}.tar.gz
$ tar -zxf nginx-${NGINX_VERSION}.tar.gz
$ cd nginx-${NGINX_VERSION}
$ : "build module"
$ ./configure --add-dynamic-module=../
$ make && make install
```

### Docker

``` sh
$ docker build -t nginx-keyval .
$ : "app.conf: Create nginx configuration"
$ docker run -p 80:80 -v $PWD/app.conf:/etc/nginx/http.d/default.conf nginx-keyval
```

> Github package: ghcr.io/kjdev/nginx-keyval

Configuration
-------------

### Example

```
http {
  keyval_zone zone=one:32k;
  keyval $arg_text $text zone=one;
  ...
  server {
    ...
    location / {
      return 200 $text;
    }
  }
}
```

### Directives

```
Syntax: keyval key $variable zone=name;
Default: -
Context: http
```

Creates a new $variable whose value is looked up by the key
in the key-value database.
The database is stored in a shared memory zone specified by the zone parameter.

```
Syntax: keyval_zone zone=name:size;
Default: -
Context: http
```

Sets the name and size of the shared memory zone that
keeps the key-value database.

Example
-------

- [OpenID Connect Authentication](example/README.md)


TODO
----

- [ ] Support for `[state=file]`, `[timeout=time]` in `keyval_zone` directive
