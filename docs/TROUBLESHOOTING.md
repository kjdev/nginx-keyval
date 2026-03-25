# Troubleshooting

## Empty Values (not_found)

If a keyval variable returns an empty string, the following causes are possible.

**Possible causes:**

- The zone is not defined
- Key variable expansion failed (`$var` is not set)
- TTL has expired
- The key has never been written

**How to check:**

Check the error.log for the following messages.

```
keyval: variable specified was not provided
```

This message indicates that the variable specified as the key could not be expanded.

```
rejected due to not found variable zone
```

This message indicates that the referenced zone was not found.

**How to fix:**

1. Verify that the `keyval_zone` or `keyval_zone_redis` directive is correctly defined
2. Verify that the variable used as the key (e.g., `$arg_key`, `$http_x_key`) has a value at request time
3. Check the TTL settings and verify that the data has not expired

## Redis Connection Errors

When using a Redis zone but values are always empty.

**Symptoms:**

- Variables for zones defined with `keyval_zone_redis` always return empty

**How to check:**

Check the error.log for the following message.

```
keyval: failed to connect redis
```

**How to fix:**

1. Verify that the Redis hostname and port are correct

   ```bash
   redis-cli -h <hostname> -p <port> ping
   ```

2. Verify that the Redis server is running

   ```bash
   redis-cli ping
   # Expected response: PONG
   ```

3. Verify that network connectivity is available

   ```bash
   telnet <hostname> <port>
   ```

4. Adjust `connect_timeout` (default: 3 seconds)
   - Increase the value in environments with slow networks
   - Example: `connect_timeout=5s`

5. Verify that the Redis `database` number is correct (default: 0)

## Configuration Errors

Errors that occur during nginx startup or configuration reload.

### zone is too small

```
"zone" is too small
```

The zone size is too small. The minimum size is `8 * pagesize` (typically 32KB).

```nginx
# Before fix
keyval_zone zone=my_zone:16k;

# After fix
keyval_zone zone=my_zone:1m;
```

### duplicate zone parameter

```
duplicate zone parameter
```

The same zone name has been defined multiple times. Zone names must be unique.

### invalid parameter

```
invalid parameter "xxx"
```

Check the spelling of the parameter. Valid parameters:

- `keyval_zone`: `zone=name:size`, `ttl=time`, `timeout=time`
- `keyval_zone_redis`: `zone=name`, `hostname=addr`, `port=num`, `database=num`, `connect_timeout=time`, `ttl=time`

### zone not found

The zone referenced by a `keyval` directive is not defined.

```nginx
# Using keyval without defining keyval_zone
keyval $arg_key $my_val zone=undefined_zone;  # Error
```

Define the zone first using `keyval_zone` or `keyval_zone_redis`.

## Performance Issues

### Shared Memory (SHM)

- **Handling a large number of keys:** Allocate a sufficiently large zone size
  - Account for the overhead of RB-tree nodes and the slab allocator
  - Guideline: number of keys x (key length + value length + approximately 128 bytes)
- **Monitoring slab allocation:** Check slab usage through nginx debug logs
- **Using TTL:** Set TTL to promote memory reuse

### Redis

- **Connection latency:** A TCP connection is established for each request to Redis
  - Using localhost (`127.0.0.1`) is recommended
  - Set `connect_timeout` low to reduce wait time on connection failure
- **Network latency:** For remote Redis servers, connection latency is added to request processing time
- **Redis server load:** In high-traffic environments, also monitor resources on the Redis server side

## Related Documentation

- [README.md](../README.md): Module overview
- [DIRECTIVES.md](DIRECTIVES.md): Directive reference
- [EXAMPLES.md](EXAMPLES.md): Configuration examples
- [INSTALL.md](INSTALL.md): Installation guide
- [SECURITY.md](SECURITY.md): Security guidelines
- [COMMERCIAL_COMPATIBILITY.md](COMMERCIAL_COMPATIBILITY.md): Commercial version compatibility
