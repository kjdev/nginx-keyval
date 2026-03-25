# Commercial Version Compatibility

A reference for compatibility between the nginx keyval module and the [nginx commercial subscription version](https://nginx.org/en/docs/http/ngx_http_keyval_module.html).

## Overview

**Compatible**: The basic `keyval` and `keyval_zone` directives are compatible with the commercial version.

**Extension**: The following features are provided that are not available in the commercial version:
- `keyval_zone_redis` directive (Redis backend)
- `ttl` parameter (alternative name for `timeout`)

**Not implemented**: The following features from the commercial version are not implemented:
- `state=file` (file-based persistence)
- `type=prefix|ip` (key match types)
- `sync` (cross-cluster synchronization)
- API endpoints (CRUD operations via REST API)

**License**: MIT License

## Directive Comparison Table

| Commercial Version | OSS Version (This Module) | Compatibility |
|--------------------|---------------------------|---------------|
| `keyval key $variable zone=name` | `keyval key $variable zone=name` | Compatible |
| `keyval_zone zone=name:size` | `keyval_zone zone=name:size` | Compatible |
| &emsp; `timeout=time` | &emsp; `timeout=time` | Compatible |
| &emsp; `state=file` | — | Not implemented |
| &emsp; `type=prefix\|ip` | — | Not implemented |
| &emsp; `sync` | — | Not implemented |
| — | &emsp; `ttl=time` | Extension |
| — | `keyval_zone_redis zone=name [...]` | Extension |
| `GET /api/9/http/keyvals/{zone}` | — | Not implemented |
| `POST /api/9/http/keyvals/{zone}` | — | Not implemented |
| `PATCH /api/9/http/keyvals/{zone}` | — | Not implemented |
| `DELETE /api/9/http/keyvals/{zone}` | — | Not implemented |

> The same compatibility and limitations apply to the Stream context (`ngx_stream_keyval_module`) as to the HTTP context.

## Related Documentation

- [README.md](../README.md): Module overview
- [DIRECTIVES.md](DIRECTIVES.md): Directive reference
- [EXAMPLES.md](EXAMPLES.md): Configuration examples
- [INSTALL.md](INSTALL.md): Installation guide
- [SECURITY.md](SECURITY.md): Security guidelines
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md): Troubleshooting guide
