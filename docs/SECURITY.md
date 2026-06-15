# Security Guidelines

## Shared Memory Security

With the shared memory (SHM) backend, data is stored in nginx's in-process shared memory.

- Data is accessible from all worker processes
- Data is not encrypted (stored in plaintext)
- Data is lost when nginx is restarted or reloaded
- OS-level process isolation restricts direct access from external processes

**Recommendations:**

- Do not use it to store sensitive data (passwords, tokens, etc.)
- It is suitable for storing temporary, non-sensitive data (session state, cache, etc.)

## Redis Security

When using the Redis backend, data is transmitted over the network.

**Note:** This module does not natively support Redis AUTH (password authentication) or TLS (encrypted communication).

**Recommendations:**

- Implement network isolation
  - Bind to localhost only with `bind 127.0.0.1`
  - Restrict access to the internal network
- Enable `protected-mode yes` (default since Redis 3.2)
- If communication over the network is required, use a TLS proxy between this module and Redis
  - Tunneling via stunnel
  - TLS proxy feature in Redis 6 and later
- If Redis AUTH (`requirepass`) is enabled, a proxy that handles authentication is also required, as this module does not send AUTH commands

### Blocking I/O DoS Risk

This module uses the hiredis synchronous API and sends/receives commands to
Redis in a blocking manner during request processing. Against a Redis that is
unresponsive, extremely slow, or behind a half-open network (TCP established
but no packets returned), the nginx worker process can stall along with its
entire event loop unless a bound is set. All other connections handled by that
worker are dragged down with it, so a Redis-side failure can escalate into a
service outage (DoS) for nginx as a whole.

**Recommendations:**

- Set both `connect_timeout` (connection establishment) and `command_timeout`
  (command send/receive) according to your requirements. Both default to `3s`.
- For unstable networks or remote Redis, set `command_timeout` as low as the
  acceptable response-latency ceiling allows.
- When a timeout occurs, the Redis connection for that request enters an error
  state and is not reused (it is reconnected on the next request).

## Key Design Best Practices

Improper key design can lead to security risks.

**Patterns to avoid:**

- Do not use user input directly as keys (risk of collision and injection)
- Avoid predictable key patterns

**Recommended patterns:**

- Use composite keys (e.g., combine multiple variables like `$request_method:$uri`)
- Add namespaces to zone names to prevent conflicts between HTTP and Stream
  - Example: `http_session_zone`, `stream_rate_zone`
- Be aware of key length limits (the shared memory slab allocator has constraints)

## TTL Configuration

Without TTL (Time-To-Live) settings, data accumulates indefinitely.

**For shared memory:**

- Zones without TTL settings will grow until memory is exhausted
- When memory is exhausted, adding new entries will fail
- TTL uses lazy expiry: expired entries are only reclaimed when accessed again, so write-once workloads may still exhaust the zone even with TTL enabled

**For Redis:**

- Keys without TTL will consume Redis memory indefinitely
- Eviction based on Redis `maxmemory` policy is unpredictable

**Recommendations:**

- Set TTL for all zones in production environments
- Choose appropriate TTL values based on the use case
- Short TTL (seconds to minutes): Rate limiting, temporary cache
- Long TTL (hours to days): Session data, configuration cache

## Related Documentation

- [README.md](../README.md): Module overview
- [DIRECTIVES.md](DIRECTIVES.md): Directive reference
- [EXAMPLES.md](EXAMPLES.md): Configuration examples
- [INSTALL.md](INSTALL.md): Installation guide
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md): Troubleshooting guide
- [COMMERCIAL_COMPATIBILITY.md](COMMERCIAL_COMPATIBILITY.md): Commercial version compatibility
