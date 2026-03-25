# nginx keyval Module

## Overview

nginx-keyval is a key-value store dynamic module for nginx. It was developed inspired by the commercial version of nginx's [ngx_http_keyval_module](https://nginx.org/en/docs/http/ngx_http_keyval_module.html).

**License**: MIT License

### Key Features

- **Dual Backend**: Supports two types of storage backends: shared memory (SHM) and Redis
- **HTTP / Stream Support**: Works in both HTTP and Stream contexts
- **TTL Support**: Configurable expiration time for key-value pairs
- **Composite Keys**: Build keys by combining multiple variables and literal strings (e.g., `$remote_addr:$http_user_agent`)
- **Variable Expansion**: Use nginx variables as keys, dynamically expanded at runtime

## Quick Start

See [INSTALL.md](docs/INSTALL.md) for installation instructions.

### Minimal Configuration Example

```nginx
http {
    keyval_zone zone=one:32k;
    keyval $arg_text $text zone=one;

    server {
        listen 80;

        location / {
            return 200 $text;
        }
    }
}
```

This configuration retrieves a value from the shared memory zone `one` using the query parameter `text` as the key, and stores it in the variable `$text`.

## Directives

| Directive | Description | Context |
|---|---|---|
| `keyval` | Define a variable from key-value pairs | http, stream |
| `keyval_zone` | Define a shared memory zone | http, stream |
| `keyval_zone_redis` | Define a Redis zone | http, stream |

See [DIRECTIVES.md](docs/DIRECTIVES.md) for detailed directive reference.

## Related Documentation

**Configuration & Operations**:
- [DIRECTIVES.md](docs/DIRECTIVES.md): Directive Reference
- [EXAMPLES.md](docs/EXAMPLES.md): Configuration Examples
- [INSTALL.md](docs/INSTALL.md): Installation Instructions
- [SECURITY.md](docs/SECURITY.md): Security Guidelines
- [TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md): Troubleshooting Guide

**Reference**:
- [COMMERCIAL_COMPATIBILITY.md](docs/COMMERCIAL_COMPATIBILITY.md): Commercial Version Compatibility
