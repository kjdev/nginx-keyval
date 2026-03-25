# Directive Reference

## Directives

| Directive | Description | Context |
|---|---|---|
| [keyval](#keyval) | Define a variable from key-value pairs | http, stream |
| [keyval_zone](#keyval_zone) | Define a shared memory zone | http, stream |
| [keyval_zone_redis](#keyval_zone_redis) | Define a Redis zone | http, stream |

### keyval

```
Syntax:  keyval key $variable zone=name;
Default: --
Context: http, stream
```

Defines a variable that retrieves the value corresponding to `key` from a key-value database and stores it in `$variable`. The database is stored in shared memory or Redis as specified by the `zone` parameter.

#### Key Variable Expansion

The `key` can be an nginx variable, a literal string, or a combination of both.

| Key Format | Example |
|---|---|
| Single variable | `$arg_text` |
| Composite key (variable + literal) | `$remote_addr:$http_user_agent` |
| Composite key (multiple elements) | `'$remote_addr $http_user_agent $host "text"'` |

Variables are dynamically expanded during request processing. Within the key string, a sequence of alphanumeric characters and underscores following `$` is interpreted as a variable name.

#### Variable Behavior

Variables defined with `keyval` support both read and write operations.

- **GET (read)**: When the variable is referenced, it looks up the value from the zone using the expanded key and returns it
- **SET (write)**: When a value is assigned to the variable, it writes the value to the zone using the expanded key

#### Variable Flags

The following flags are set on the defined variable.

| Flag | Description |
|---|---|
| `CHANGEABLE` | Allows writing via the `set` directive and similar mechanisms |
| `NOCACHEABLE` | The handler is invoked on every request (value is not cached) |

### keyval_zone

```
Syntax:  keyval_zone zone=name:size [ttl=time] [timeout=time];
Default: --
Context: http, stream
```

Sets the name and size of the shared memory zone that holds the key-value database.

#### Parameters

| Parameter | Required | Default | Description |
|---|---|---|---|
| `zone=name:size` | Yes | -- | Zone name and size. `name` is the zone identifier, `size` is the shared memory size |
| `ttl=time` | No | `0` (no expiration) | Expiration time for key-value pairs |
| `timeout=time` | No | `0` (no expiration) | Legacy alias for `ttl` |

#### Zone Size

The `size` must be at least `8 * pagesize` (typically 32KB). Size suffixes `k` (kilobytes) and `m` (megabytes) can be used.

```nginx
keyval_zone zone=one:32k;     # 32KB
keyval_zone zone=two:1m;      # 1MB
```

#### TTL (Time to Live)

The `ttl` (or `timeout`) accepts nginx time format specifications.

| Suffix | Unit |
|---|---|
| `s` | Seconds (default) |
| `m` | Minutes |
| `h` | Hours |

The default value `0` means no expiration (data is retained permanently).

```nginx
keyval_zone zone=cache:64k ttl=30m;   # Expires after 30 minutes
keyval_zone zone=session:1m ttl=1h;   # Expires after 1 hour
```

> **Note**: `timeout` is a legacy alias for `ttl`. Specifying both at the same time will result in an error.

### keyval_zone_redis

```
Syntax:  keyval_zone_redis zone=name [hostname=name] [port=number] [database=number] [connect_timeout=time] [ttl=time];
Default: --
Context: http, stream
```

Defines the name and connection settings for a Redis zone that holds the key-value database.

> **Build Requirement**: To use this directive, the `NGX_KEYVAL_ZONE_REDIS=1` flag must be specified at build time.

#### Parameters

| Parameter | Required | Default | Description |
|---|---|---|---|
| `zone=name` | Yes | -- | Zone identifier |
| `hostname=name` | No | `127.0.0.1` | Redis server hostname |
| `port=number` | No | `6379` | Redis server port number |
| `database=number` | No | `0` | Redis database number |
| `connect_timeout=time` | No | `3s` | Redis connection timeout |
| `ttl=time` | No | `0` (no expiration) | Expiration time for key-value pairs |

#### TTL Behavior

When `ttl` is greater than `0`, the `SETEX` command is used for writing to Redis, and keys are automatically deleted after the specified time. When `ttl=0` (default), the standard `SET` command is used.

#### Configuration Examples

```nginx
# Local Redis (default settings)
keyval_zone_redis zone=kv_redis;

# Remote Redis (custom settings)
keyval_zone_redis zone=kv_remote
                  hostname=redis.example.com
                  port=6380
                  database=1
                  connect_timeout=5s
                  ttl=1h;
```

## Related Documentation

- [README.md](../README.md): Module Overview
- [EXAMPLES.md](EXAMPLES.md): Configuration Examples
- [INSTALL.md](INSTALL.md): Installation Instructions
- [SECURITY.md](SECURITY.md): Security Guidelines
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md): Troubleshooting Guide
- [COMMERCIAL_COMPATIBILITY.md](COMMERCIAL_COMPATIBILITY.md): Commercial Version Compatibility
