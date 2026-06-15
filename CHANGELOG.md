# Changelog

## [b3a3223](../../commit/b3a3223) - 2026-06-15

### Added

- `command_timeout=` parameter for `keyval_zone_redis` (default `3s`) that bounds the send/receive of each Redis command (SELECT/GET/SET/SETEX) via `redisSetTimeout`. Previously only the connection timeout was configured, leaving command round-trips unbounded, so an unresponsive, extremely slow, or half-open Redis could block the nginx worker process and its entire event loop indefinitely, escalating into a worker-wide DoS

## [5850601](../../commit/5850601) - 2026-06-15

### Fixed

- Use `ngx_cpymem` when building variable keys so that values containing NUL bytes (e.g. `$binary_remote_addr`) are copied in full, preventing uninitialized pool memory from leaking into stored/transmitted keys and breaking get/set lookups

## [fdc4294](../../commit/fdc4294) - 2026-06-15

### Fixed

- Isolate Redis connection context per zone so that multiple `keyval_zone_redis` zones with different hostname/port/database accessed in the same request no longer reuse the first zone's connection

## [172ac41](../../commit/172ac41) - 2026-03-27

### Changed

- Replace timer-based TTL expiration with lazy expiry at request time

## [adb5609](../../commit/adb5609) - 2024-02-28

### Added

- Support multiple variables in `keyval` directive key (`$var1_$var2` format)

## [27433a0](../../commit/27433a0) - 2024-03-10

### Added

- TTL support for shared memory zones (`ttl=` parameter in `keyval_zone` directive)

## [3e514c0](../../commit/3e514c0) - 2023-08-03

### Added

- Stream module support (`ngx_stream_keyval_module`)

## [8ec1ccd](../../commit/8ec1ccd) - 2023-02-13

### Changed

- Rename `timeout` parameter to `ttl` in `keyval_zone_redis`

## [950d059](../../commit/950d059) - 2022-10-14

### Added

- Redis storage backend (`keyval_zone_redis` directive)

## [3b60670](../../commit/3b60670) - 2022-10-12

### Added

- Initial implementation of key-value store module with shared memory
- `keyval_zone` directive (shared memory zone definition)
- `keyval` directive (key-variable mapping)
- RB-tree indexed lookup
