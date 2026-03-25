#include "ngx_keyval.h"

#if (NGX_HAVE_KEYVAL_ZONE_REDIS)
void
ngx_keyval_redis_cleanup_ctx(void *data)
{
  ngx_keyval_redis_ctx_t *ctx = data;

  if (ctx && ctx->redis) {
    redisFree(ctx->redis);
    ctx->redis = NULL;
  }
}

redisContext *
ngx_keyval_redis_get_context(ngx_keyval_redis_ctx_t *ctx,
                             ngx_keyval_redis_conf_t *conf, ngx_log_t *log)
{
  struct timeval timeout = { 0, 0 };

  if (!ctx || !conf) {
    return NULL;
  }

  if (ctx->redis) {
    return ctx->redis;
  }

  timeout.tv_sec = conf->connect_timeout;

  ctx->redis = redisConnectWithTimeout((char *) conf->hostname, conf->port,
                                       timeout);
  if (!ctx->redis) {
    ngx_log_error(NGX_LOG_ERR, log, 0,
                  "keyval: failed to connect redis: "
                  "hostname=%s port=%d connect_timeout=%ds",
                  (char *) conf->hostname, conf->port, conf->connect_timeout);
    return NULL;
  } else if (ctx->redis->err) {
    ngx_log_error(NGX_LOG_ERR, log, 0,
                  "keyval: failed to connect redis: "
                  "hostname=%s port=%d connect_timeout=%ds: %s",
                  (char *) conf->hostname, conf->port, conf->connect_timeout,
                  ctx->redis->errstr);
    return NULL;
  }

  if (conf->db > 0) {
    redisReply *resp = NULL;

    resp = (redisReply *) redisCommand(ctx->redis, "SELECT %d", conf->db);
    if (!resp) {
      ngx_log_error(NGX_LOG_ERR, log, 0,
                    "keyval: failed to command redis: SELECT");
      return NULL;
    } else if (resp->type == REDIS_REPLY_ERROR) {
      ngx_log_error(NGX_LOG_ERR, log, 0,
                    "keyval: failed to command redis: SELECT: %s", resp->str);
      freeReplyObject(resp);
      return NULL;
    }
    freeReplyObject(resp);
  }

  return ctx->redis;
}

ngx_int_t
ngx_keyval_redis_get_data(redisContext *ctx, ngx_str_t *zone, ngx_str_t *key,
                          ngx_str_t *val, ngx_pool_t *pool, ngx_log_t *log)
{
  ngx_int_t rc = NGX_ERROR;
  redisReply *resp = NULL;

  if (!ctx || !zone || !key || !val) {
    return NGX_ERROR;
  }

  resp = (redisReply *) redisCommand(ctx, "GET %b:%b",
                                     zone->data, zone->len,
                                     key->data, key->len);
  if (!resp) {
    ngx_log_error(NGX_LOG_ERR, log, 0,
                  "keyval: failed to command redis: GET");
    return NGX_ERROR;
  }

  if (resp->type == REDIS_REPLY_STRING) {
    u_char *str;

    str = ngx_pnalloc(pool, resp->len + 1);
    if (str) {
      ngx_memcpy(str, resp->str, resp->len);
      str[resp->len] = '\0';

      val->data = str;
      val->len = resp->len;

      rc = NGX_OK;
    } else {
      ngx_log_error(NGX_LOG_CRIT, log, 0,
                    "keyval: failed to allocate redis reply");
    }
  } else if (resp->type == REDIS_REPLY_ERROR) {
    ngx_log_error(NGX_LOG_ERR, log, 0,
                  "keyval: failed to command redis: GET: %s", resp->str);
  } else {
    ngx_log_error(NGX_LOG_INFO, log, 0,
                  "keyval: failed to command redis: GET: type: %d", resp->type);
  }

  freeReplyObject(resp);

  return rc;
}

ngx_int_t
ngx_keyval_redis_set_data(redisContext *ctx, ngx_keyval_redis_conf_t *conf,
                          ngx_str_t *zone, ngx_str_t *key, ngx_str_t *val,
                          ngx_log_t *log)
{
  ngx_int_t rc = NGX_ERROR;
  redisReply *resp = NULL;

  if (!ctx || !conf || !zone || !key || !val) {
    return NGX_ERROR;
  }

  if (conf->ttl == 0) {
    resp = (redisReply *) redisCommand(ctx, "SET %b:%b %b",
                                       zone->data, zone->len,
                                       key->data, key->len,
                                       val->data, val->len);
  } else {
    resp = (redisReply *) redisCommand(ctx, "SETEX %b:%b %d %b",
                                       zone->data, zone->len,
                                       key->data, key->len,
                                       conf->ttl, val->data, val->len);
  }

  if (!resp) {
    ngx_log_error(NGX_LOG_ERR, log, 0,
                  "keyval: failed to command redis: SET|SETEX");
    return NGX_ERROR;
  }

  if (resp->type == REDIS_REPLY_ERROR) {
    ngx_log_error(NGX_LOG_ERR, log, 0,
                  "keyval: failed to command redis: SET|SETEX: %s", resp->str);
  } else {
    rc = NGX_OK;
  }

  freeReplyObject(resp);

  return rc;
}
#endif
