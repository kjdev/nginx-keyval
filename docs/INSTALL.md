# Installation Guide

This document describes how to install the nginx-keyval module.

## Requirements

| Library | Required | Purpose |
|---------|----------|---------|
| hiredis | Optional | Only required when using the Redis backend |

No additional dependencies are needed if you are not using the Redis backend.

### Installing hiredis

If you plan to use the Redis backend, install hiredis.

```bash
# Debian / Ubuntu
apt-get install libhiredis-dev

# RHEL / Fedora
dnf install hiredis-devel

# Alpine
apk add hiredis-dev
```

## Build

### Basic Build (Shared Memory Backend Only)

```bash
# Clone the repository
git clone https://github.com/kjdev/nginx-keyval.git
cd nginx-keyval

# Download the nginx source
NGINX_VERSION=1.28.2
wget https://nginx.org/download/nginx-${NGINX_VERSION}.tar.gz
tar xzf nginx-${NGINX_VERSION}.tar.gz

# Configure and make
cd nginx-${NGINX_VERSION}
./configure --with-stream=dynamic --add-dynamic-module=../
make
```

### Build with Redis Backend

```bash
export NGX_KEYVAL_ZONE_REDIS=1

cd nginx-${NGINX_VERSION}
./configure --with-stream=dynamic --add-dynamic-module=../
make
```

After a successful build, the following module files are generated in the `objs/` directory.

- `ngx_http_keyval_module.so` — For the HTTP context
- `ngx_stream_keyval_module.so` — For the Stream context

## Installation

### Placing the Module Files

Copy the generated `.so` files to the nginx modules directory.

```bash
cp objs/ngx_http_keyval_module.so /usr/lib/nginx/modules/
cp objs/ngx_stream_keyval_module.so /usr/lib/nginx/modules/
```

### Loading the Modules

Load the modules at the top of the nginx configuration file (before the `events` block).

```nginx
# To use in the HTTP context
load_module modules/ngx_http_keyval_module.so;

# To use in the Stream context
load_module modules/ngx_stream_keyval_module.so;
```

### Verification

A minimal configuration example to verify that the module loads correctly.

```nginx
load_module modules/ngx_http_keyval_module.so;

events {
    worker_connections 1024;
}

http {
    keyval_zone zone=test:32k;
    keyval $arg_key $test_value zone=test;

    server {
        listen 8080;

        location /test {
            return 200 "key=$arg_key value=$test_value\n";
        }
    }
}
```

Validate the configuration file:

```bash
nginx -t -c /path/to/nginx.conf
```

## Docker Image

A pre-built Docker image is available from the GitHub Container Registry.

```bash
docker pull ghcr.io/kjdev/nginx-keyval/nginx:latest
```

Usage example:

```bash
docker run -d -p 8080:80 \
  -v $(pwd)/nginx.conf:/etc/nginx/nginx.conf:ro \
  ghcr.io/kjdev/nginx-keyval/nginx:latest
```

## Related Documentation

- [README.md](../README.md): Module overview
- [DIRECTIVES.md](DIRECTIVES.md): Directive reference
- [EXAMPLES.md](EXAMPLES.md): Configuration examples
- [SECURITY.md](SECURITY.md): Security guidelines
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md): Troubleshooting guide
- [COMMERCIAL_COMPATIBILITY.md](COMMERCIAL_COMPATIBILITY.md): Commercial version compatibility
