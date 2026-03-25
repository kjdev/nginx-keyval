# Configuration Examples

## Basic Key-Value Store

The simplest key-value store configuration using a shared memory zone.

```nginx
http {
    keyval_zone zone=one:32k;
    keyval $arg_text $text zone=one;

    server {
        listen 80;

        # Get a value
        # GET /get?text=hello -> Returns the value for key "hello" from the zone
        location /get {
            return 200 $text;
        }

        # Set a value
        # GET /set?text=hello&value=world -> Stores "world" for key "hello"
        location /set {
            set $text $arg_value;
            return 200 "OK";
        }
    }
}
```

Variables defined with `keyval` support both read and write operations. When referenced in `return` or `proxy_pass`, it performs a read (GET) from the zone. When assigned via the `set` directive, it performs a write (SET) to the zone.

## TTL-Based Data Management

By specifying the `ttl` parameter, data is automatically deleted after a certain period of time.

```nginx
http {
    keyval_zone zone=cache:64k ttl=30m;
    keyval $arg_key $cached_value zone=cache;

    server {
        listen 80;

        location / {
            return 200 $cached_value;
        }
    }
}
```

In this example, data stored in the zone `cache` automatically expires and is deleted after 30 minutes. This is useful for managing data that is only valid for a certain period, such as session data or rate limit counters.

## Redis Backend

A configuration example using Redis as the storage backend.

> **Build Requirement**: Must be built with the `NGX_KEYVAL_ZONE_REDIS=1` flag.

```nginx
http {
    keyval_zone_redis zone=kv_redis
                      hostname=127.0.0.1
                      port=6379
                      database=0
                      connect_timeout=3s
                      ttl=1h;
    keyval $arg_key $value zone=kv_redis;

    server {
        listen 80;

        location / {
            return 200 $value;
        }
    }
}
```

Using the Redis backend allows data to persist across nginx process restarts and reloads. It is also suitable for sharing data between multiple nginx instances.

## Composite Keys (Variable Expansion)

You can construct keys by combining multiple nginx variables and literal strings.

```nginx
http {
    keyval_zone zone=access:64k ttl=10m;
    keyval $remote_addr:$http_user_agent $access_flag zone=access;

    server {
        listen 80;

        location / {
            # Use the combination of client IP address and User-Agent as a key
            # to look up the access flag
            if ($access_flag = "blocked") {
                return 403;
            }

            proxy_pass http://backend;
        }
    }
}
```

The key `$remote_addr:$http_user_agent` is expanded at request processing time. For example, if the IP address is `192.168.1.1` and the User-Agent is `curl/7.68.0`, the key becomes `192.168.1.1:curl/7.68.0`.

## Multiple Zone Management

You can define multiple zones for different purposes to manage data separately.

```nginx
http {
    keyval_zone zone=sessions:1m ttl=30m;
    keyval_zone zone=config:32k;

    keyval $cookie_session_id $session_data zone=sessions;
    keyval $arg_setting $config_value zone=config;

    server {
        listen 80;

        # Reference session data
        location /profile {
            return 200 $session_data;
        }

        # Reference configuration values
        location /config {
            return 200 $config_value;
        }
    }
}
```

Zones are uniquely identified by name. Defining duplicate zones with the same name will result in an error.

## Stream Module

An L4 routing example in the Stream context.

```nginx
stream {
    keyval_zone zone=routes:32k;
    keyval $ssl_server_name $backend zone=routes;

    server {
        listen 12345 ssl;
        proxy_pass $backend;
        ssl_certificate     /etc/nginx/cert.pem;
        ssl_certificate_key /etc/nginx/cert.key;
    }
}
```

This configuration dynamically determines the destination backend using the TLS SNI server name as the key. By pre-registering server name and backend address pairs in the zone, dynamic routing at the L4 level can be achieved.

## Running with Docker

You can easily run the module using a container image.

```sh
# Build locally
docker build -t nginx-keyval .
docker run -p 80:80 -v $PWD/app.conf:/etc/nginx/http.d/default.conf nginx-keyval
```

```sh
# Use the GitHub Container Registry image
docker run -p 80:80 \
    -v $PWD/app.conf:/etc/nginx/http.d/default.conf \
    ghcr.io/kjdev/nginx-keyval/nginx:latest
```

Write the nginx configuration you want to use in `app.conf`.

```nginx
# Example app.conf
keyval_zone zone=one:32k;
keyval $arg_text $text zone=one;

server {
    listen 80;

    location / {
        return 200 $text;
    }
}
```

## Related Documentation

- [README.md](../README.md): Module Overview
- [DIRECTIVES.md](DIRECTIVES.md): Directive Reference
- [INSTALL.md](INSTALL.md): Installation Instructions
- [SECURITY.md](SECURITY.md): Security Guidelines
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md): Troubleshooting Guide
- [COMMERCIAL_COMPATIBILITY.md](COMMERCIAL_COMPATIBILITY.md): Commercial Version Compatibility
