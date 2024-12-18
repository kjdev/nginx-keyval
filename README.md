nginx-keyval
============

This nginx module creates variables with values taken from key-value pairs.

> This module is heavily inspired by the nginx original
> [http_keyval_module](https://nginx.org/en/docs/http/ngx_http_keyval_module.html).

Dependency
----------

Using the Redis store.

- [hiredis](https://github.com/redis/hiredis)

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
$ : "using redis store"
$ : export NGX_KEYVAL_ZONE_REDIS=1
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

> Github package: ghcr.io/kjdev/nginx-keyval/nginx

Configuration: `ngx_http_keyval_module`
---------------------------------------

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

Creates a new `$variable` whose value is looked up by the `key`
in the key-value database.

The database is stored in shared memory or Redis as specified
by the zone parameter.

In `key`, you can use a mix of variables and text or just variables.

> For example:
> - `$remote_addr:$http_user_agent`
> - `'$remote_addr    $http_user_agent   $host "a random text"'`

```
Syntax: keyval_zone zone=name:size [timeout=time] [ttl=time];
Default: -
Context: http
```

Sets the `name` and `size` of the shared memory zone that
keeps the key-value database.

The optional `timeout` or `ttl` parameter sets the time to live
which key-value pairs are removed (default value is `0` seconds).

```
Syntax: keyval_zone_redis zone=name [hostname=name] [port=number] [database=number] [connect_timeout=time] [ttl=time];
Default: -
Context: http
```

> Using the Redis store

Sets the `name` of the Redis zone that keeps the key-value database.

The optional `hostname` parameter sets the Redis hostname
(default value is `127.0.0.1`).

The optional `port` parameter sets the Redis port
(default value is `6379`).

The optional `database` parameter sets the Redis database number
(default value is `0`).

The optional `connect_timeout` parameter sets the Redis connection
timeout seconds (default value is `3`).

The optional `ttl` parameter sets the time to live
which key-value pairs are removed (default value is `0` seconds).

Configuration: `ngx_stream_keyval_module`
---------------------------------------

### Example

```
stream {
  keyval_zone zone=one:32k;
  keyval $ssl_server_name $name zone=one;

  server {
    listen 12345 ssl;
    proxy_pass $name;
    ssl_certificate /usr/share/nginx/conf/cert.pem;
    ssl_certificate_key /usr/share/nginx/conf/cert.key;
  }
}
```

### Directives

```
Syntax: keyval key $variable zone=name;
Default: -
Context: http
```

Creates a new `$variable` whose value is looked up by the `key`
in the key-value database.

The database is stored in shared memory or Redis as specified
by the zone parameter.

```
Syntax: keyval_zone zone=name:size [timeout=time] [ttl=time];
Default: -
Context: http
```

Sets the `name` and `size` of the shared memory zone that
keeps the key-value database.

The optional `timeout` or `ttl` parameter sets the time to live which key-value pairs are removed (default value is 0 seconds).

```
Syntax: keyval_zone_redis zone=name [hostname=name] [port=number] [database=number] [connect_timeout=time] [ttl=time];
Default: -
Context: http
```

> Using the Redis store

Sets the `name` of the Redis zone that keeps the key-value database.

The optional `hostname` parameter sets the Redis hostname
(default value is `127.0.0.1`).

The optional `port` parameter sets the Redis port
(default value is `6379`).

The optional `database` parameter sets the Redis database number
(default value is `0`).

The optional `connect_timeout` parameter sets the Redis connection
timeout seconds (default value is `3`).

The optional `ttl` parameter sets the time to live
which key-value pairs are removed (default value is `0` seconds).

Example
-------

- [OpenID Connect Authentication](example/README.md)


TODO
----

- [ ] Support for `[state=file]` in `keyval_zone` directive
