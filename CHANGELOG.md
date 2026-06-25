# Changelog

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [0.5.0] - 2026-06-15

### Added

- `command_timeout=` parameter for `keyval_zone_redis` (default `3s`) that bounds the send/receive of each Redis command (SELECT/GET/SET/SETEX) via `redisSetTimeout`. Previously only the connection timeout was configured, leaving command round-trips unbounded, so an unresponsive, extremely slow, or half-open Redis could block the nginx worker process and its entire event loop indefinitely, escalating into a worker-wide DoS

### Fixed

- Use `ngx_cpymem` when building variable keys so that values containing NUL bytes (e.g. `$binary_remote_addr`) are copied in full, preventing uninitialized pool memory from leaking into stored/transmitted keys and breaking get/set lookups
- Isolate Redis connection context per zone so that multiple `keyval_zone_redis` zones with different hostname/port/database accessed in the same request no longer reuse the first zone's connection

## [0.4.0] - 2026-03-30

### Changed

- Replace timer-based TTL expiration with lazy expiry at request time

## [0.3.0] - 2024-04-01

### Added

- Support multiple variables in `keyval` directive key (`$var1_$var2` format)
- TTL support for shared memory zones (`ttl=` parameter in `keyval_zone` directive)

## [0.2.0] - 2023-08-04

### Added

- Stream module support (`ngx_stream_keyval_module`)

## [0.1.0] - 2023-02-17

### Added

- Initial implementation of key-value store module with shared memory
- `keyval_zone` directive (shared memory zone definition)
- `keyval` directive (key-variable mapping)
- RB-tree indexed lookup
- Redis storage backend (`keyval_zone_redis` directive)

### Changed

- Rename `timeout` parameter to `ttl` in `keyval_zone_redis`

[0.5.0]: ../../compare/0.4.0...0.5.0
[0.4.0]: ../../compare/0.3.0...0.4.0
[0.3.0]: ../../compare/0.2.0...0.3.0
[0.2.0]: ../../compare/0.1.0...0.2.0
[0.1.0]: ../../releases/tag/0.1.0
