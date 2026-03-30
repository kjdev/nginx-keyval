# Changelog

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
