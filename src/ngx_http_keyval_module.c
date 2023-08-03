#include <ngx_http.h>
#include "ngx_keyval.h"

static char *ngx_http_keyval_conf_set_zone(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
#if (NGX_HAVE_HTTP_KEYVAL_ZONE_REDIS)
static char *ngx_http_keyval_conf_set_zone_redis(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
#endif
static char *ngx_http_keyval_conf_set_variable(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static void ngx_http_keyval_variable_set_handler(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_keyval_variable_get_handler(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data);

static ngx_command_t ngx_http_keyval_commands[] = {
  { ngx_string("keyval"),
    NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE3,
    ngx_http_keyval_conf_set_variable,
    0,
    0,
    NULL },
  { ngx_string("keyval_zone"),
    NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
    ngx_http_keyval_conf_set_zone,
    0,
    0,
    NULL },
#if (NGX_HAVE_HTTP_KEYVAL_ZONE_REDIS)
  { ngx_string("keyval_zone_redis"),
    NGX_HTTP_MAIN_CONF|NGX_CONF_1MORE,
    ngx_http_keyval_conf_set_zone_redis,
    0,
    0,
    NULL },
#endif
  ngx_null_command
};

static ngx_http_module_t ngx_http_keyval_module_ctx = {
  NULL,                             /* preconfiguration */
  NULL,                             /* postconfiguration */
  ngx_keyval_create_main_conf,      /* create main configuration */
  NULL,                             /* init main configuration */
  NULL,                             /* create server configuration */
  NULL,                             /* merge server configuration */
  NULL,                             /* create location configuration */
  NULL                              /* merge location configuration */
};

ngx_module_t ngx_http_keyval_module = {
  NGX_MODULE_V1,
  &ngx_http_keyval_module_ctx, /* module context */
  ngx_http_keyval_commands,    /* module directives */
  NGX_HTTP_MODULE,             /* module type */
  NULL,                        /* init master */
  NULL,                        /* init module */
  NULL,                        /* init process */
  NULL,                        /* init thread */
  NULL,                        /* exit thread */
  NULL,                        /* exit process */
  NULL,                        /* exit master */
  NGX_MODULE_V1_PADDING
};

static char *
ngx_http_keyval_conf_set_zone(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
  ngx_keyval_conf_t *config;

  config = ngx_http_conf_get_module_main_conf(cf, ngx_http_keyval_module);

  return ngx_keyval_conf_set_zone(cf, cmd, conf,
                                  config, &ngx_http_keyval_module);
}

#if (NGX_HAVE_HTTP_KEYVAL_ZONE_REDIS)
static char *
ngx_http_keyval_conf_set_zone_redis(ngx_conf_t *cf,
                                    ngx_command_t *cmd, void *conf)
{
  ngx_keyval_conf_t *config;

  config = ngx_http_conf_get_module_main_conf(cf, ngx_http_keyval_module);

  return ngx_keyval_conf_set_zone_redis(cf, cmd, conf, config);
}
#endif

static char *
ngx_http_keyval_conf_set_variable(ngx_conf_t *cf,
                                  ngx_command_t *cmd, void *conf)
{
  char *retval;
  ngx_uint_t flags;
  ngx_http_variable_t *v;
  ngx_keyval_conf_t *config;
  ngx_keyval_variable_t *var = NULL;

  config = ngx_http_conf_get_module_main_conf(cf, ngx_http_keyval_module);

  retval = ngx_keyval_conf_set_variable(cf, cmd, conf,
                                        config, &ngx_http_keyval_module, &var,
                                        ngx_http_get_variable_index);
  if (retval != NGX_CONF_OK) {
    return retval;
  }
  if (!var) {
    return "failed to allocate";
  }

  /* add variable */
  flags = NGX_HTTP_VAR_CHANGEABLE | NGX_HTTP_VAR_NOCACHEABLE;
  v = ngx_http_add_variable(cf, &(var->variable), flags);
  if (v == NULL) {
    return "failed to add variable";
  }

  v->get_handler = ngx_http_keyval_variable_get_handler;
  v->set_handler = ngx_http_keyval_variable_set_handler;
  v->data = (uintptr_t) var;

  return NGX_CONF_OK;
}

static ngx_int_t
ngx_http_keyval_variable_get_key(ngx_http_request_t *r,
                                 ngx_keyval_variable_t *var, ngx_str_t *key)
{
  if (!key || !var) {
    return NGX_ERROR;
  }

  if (var->key_index != NGX_CONF_UNSET) {
    ngx_http_variable_value_t *v;
    v = ngx_http_get_indexed_variable(r, var->key_index);
    if (v == NULL || v->not_found) {
      ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                    "keyval: variable specified was not provided");
      return NGX_ERROR;
    } else {
      key->data = v->data;
      key->len = v->len;
    }
  } else {
    *key = var->key_string;
  }

  return NGX_OK;
}

static ngx_int_t
ngx_http_keyval_variable_init(ngx_http_request_t *r, uintptr_t data,
                              ngx_str_t *key, ngx_keyval_zone_t **zone)
{
  ngx_keyval_conf_t *cf;
  ngx_keyval_variable_t *var;

  if (data == 0) {
    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                  "keyval: rejected due to not handler data");
    return NGX_ERROR;
  }

  cf = ngx_http_get_module_main_conf(r, ngx_http_keyval_module);
  if (!cf) {
    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                  "keyval: rejected due to not found main configuration");
    return NGX_ERROR;
  }

  var = (ngx_keyval_variable_t *) data;

  if (ngx_http_keyval_variable_get_key(r, var, key) != NGX_OK) {
    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                  "keyval: rejected due to not found variable key");
    return NGX_ERROR;
  }

  if (!var->zone) {
    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                  "keyval: rejected due to not found variable zone");
    return NGX_ERROR;
  }

  *zone = var->zone;

  return NGX_OK;
}

static ngx_keyval_shm_ctx_t *
ngx_keyval_shm_get_context(ngx_shm_zone_t *shm, ngx_log_t *log)
{
  ngx_keyval_shm_ctx_t *ctx;

  if (!shm) {
    ngx_log_error(NGX_LOG_INFO, log, 0,
                  "keyval: rejected due to not found shared memory zone");
    return NULL;
  }

  ctx = shm->data;
  if (!ctx) {
    ngx_log_error(NGX_LOG_INFO, log, 0,
                  "keyval: rejected due to not found shared memory context");
    return NULL;
  }

  return ctx;
}

static ngx_int_t
ngx_keyval_shm_get_data(ngx_keyval_shm_ctx_t *ctx, ngx_shm_zone_t *shm,
                        ngx_str_t *key, ngx_str_t *val)
{
  uint32_t hash;
  ngx_rbtree_node_t *node;
  ngx_keyval_node_t *kv;

  if (!ctx || !shm || !key || !val) {
    return NGX_ERROR;
  }

  hash = ngx_crc32_short(key->data, key->len);

  ngx_shmtx_lock(&ctx->shpool->mutex);

  node = ngx_keyval_rbtree_lookup(&ctx->sh->rbtree, key, hash);

  ngx_shmtx_unlock(&ctx->shpool->mutex);

  if (node == NULL) {
    return NGX_DECLINED;
  }

  kv = (ngx_keyval_node_t *) &node->color;

  // key->len = kv->len;
  // key->data = kv->data;

  val->len = kv->size - kv->len;
  val->data = kv->data + kv->len;

  return NGX_OK;
}

static ngx_int_t
ngx_keyval_shm_set_data(ngx_keyval_shm_ctx_t *ctx, ngx_shm_zone_t *shm,
                        ngx_str_t *key, ngx_str_t *val, ngx_log_t *log)
{
  uint32_t hash;
  size_t n;
  ngx_int_t rc;
  ngx_rbtree_node_t *node;

  if (!ctx || !shm || !key || !val) {
    return NGX_ERROR;
  }

  hash = ngx_crc32_short(key->data, key->len);

  ngx_shmtx_lock(&ctx->shpool->mutex);

  node = ngx_keyval_rbtree_lookup(&ctx->sh->rbtree, key, hash);
  if (node != NULL) {
    ngx_rbtree_delete(&ctx->sh->rbtree, node);
    ngx_slab_free_locked(ctx->shpool, node);
  }

  n = offsetof(ngx_rbtree_node_t, color)
    + offsetof(ngx_keyval_node_t, data)
    + key->len
    + val->len;

  node = ngx_slab_alloc_locked(ctx->shpool, n);
  if (node == NULL) {
    ngx_log_error(NGX_LOG_ERR, log, 0,
                  "keyval: failed to allocate slab");
    rc = NGX_ERROR;
  } else {
    ngx_keyval_node_t *kv;
    kv = (ngx_keyval_node_t *) &node->color;

    node->key = hash;
    kv->size = key->len + val->len;
    kv->len = key->len;
    ngx_memcpy(kv->data, key->data, key->len);
    ngx_memcpy(kv->data + key->len, val->data, val->len);

    ngx_rbtree_insert(&ctx->sh->rbtree, node);

    rc = NGX_OK;
  }

  ngx_shmtx_unlock(&ctx->shpool->mutex);

  return rc;
}

#if (NGX_HAVE_HTTP_KEYVAL_ZONE_REDIS)
static ngx_keyval_redis_ctx_t *
ngx_http_keyval_redis_get_ctx(ngx_http_request_t *r)
{
  ngx_pool_cleanup_t *cleanup;
  ngx_keyval_redis_ctx_t *ctx;

  ctx = ngx_http_get_module_ctx(r, ngx_http_keyval_module);
  if (ctx != NULL) {
    return ctx;
  }

  ctx = ngx_pcalloc(r->pool, sizeof(ngx_keyval_redis_ctx_t));
  if (ctx == NULL) {
    ngx_log_error(NGX_LOG_CRIT, r->connection->log, 0,
                  "keyval: failed to allocate redis context");
    return NULL;
  }

  ctx->redis = NULL;

  ngx_http_set_ctx(r, ctx, ngx_http_keyval_module);

  cleanup = ngx_pool_cleanup_add(r->pool, 0);
  if (cleanup == NULL) {
    ngx_log_error(NGX_LOG_CRIT, r->connection->log, 0,
                  "keyval: failed to allocate redis context cleanup");
    return NULL;
  }
  cleanup->handler = ngx_keyval_redis_cleanup_ctx;
  cleanup->data = ctx;

  return ctx;
}

static redisContext *
ngx_http_keyval_redis_get_context(ngx_http_request_t *r,
                                  ngx_keyval_redis_conf_t *redis)
{
  struct timeval timeout = { 0, 0 };
  ngx_keyval_redis_ctx_t *ctx;

  ctx = ngx_http_keyval_redis_get_ctx(r);
  if (!ctx) {
    return NULL;
  }

  if (ctx->redis) {
    return ctx->redis;
  }

  timeout.tv_sec = redis->connect_timeout;

  ctx->redis = redisConnectWithTimeout((char *)redis->hostname, redis->port,
                                       timeout);
  if (!ctx->redis) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                  "keyval: failed to connect redis: "
                  "hostname=%s port=%d connect_timeout=%ds",
                  (char *)redis->hostname, redis->port, redis->connect_timeout);
    return NULL;
  } else if (ctx->redis->err) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                  "keyval: failed to connect redis: "
                  "hostname=%s port=%d connect_timeout=%ds: %s",
                  (char *)redis->hostname, redis->port, redis->connect_timeout,
                  ctx->redis->errstr);
    return NULL;
  }

  if (redis->db > 0) {
    redisReply *resp = NULL;

    resp = (redisReply *) redisCommand(ctx->redis, "SELECT %d", redis->db);
    if (!resp) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                    "keyval: failed to command redis: SELECT");
      return NULL;
    } else if (resp->type == REDIS_REPLY_ERROR) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                    "keyval: failed to command redis: SELECT: %s", resp->str);
      freeReplyObject(resp);
      return NULL;
    }
    freeReplyObject(resp);
  }

  return ctx->redis;
}

static ngx_int_t
ngx_http_keyval_redis_get_data(ngx_http_request_t *r,
                               ngx_keyval_redis_conf_t *redis,
                               ngx_str_t *zone, ngx_str_t *key, ngx_str_t *val)
{
  ngx_int_t rc = NGX_ERROR;
  redisContext *ctx = NULL;
  redisReply *resp = NULL;

  if (!redis || !zone || !key || !val) {
    return NGX_ERROR;
  }

  ctx = ngx_http_keyval_redis_get_context(r, redis);
  if (ctx == NULL) {
    return NGX_ERROR;
  }

  resp = (redisReply *) redisCommand(ctx, "GET %b:%b",
                                     zone->data, zone->len,
                                     key->data, key->len);
  if (!resp) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                  "keyval: failed to command redis: GET");
    return NGX_ERROR;
  }

  if (resp->type == REDIS_REPLY_STRING) {
    u_char *str;

    str = ngx_pnalloc(r->pool, resp->len + 1);
    if (str) {
      ngx_memcpy(str, resp->str, resp->len);
      str[resp->len] = '\0';

      val->data = str;
      val->len = resp->len;

      rc = NGX_OK;
    } else {
      ngx_log_error(NGX_LOG_CRIT, r->connection->log, 0,
                    "keyval: failed to allocate redis reply");
    }
  } else if (resp->type == REDIS_REPLY_ERROR) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                  "keyval: failed to command redis: GET: %s", resp->str);
  } else {
    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                  "keyval: failed to command redis: GET: type: %d", resp->type);
  }

  freeReplyObject(resp);

  return rc;
}

static ngx_int_t
ngx_http_keyval_redis_set_data(ngx_http_request_t *r,
                               ngx_keyval_redis_conf_t *redis,
                               ngx_str_t *zone, ngx_str_t *key, ngx_str_t *val)
{
  ngx_int_t rc = NGX_ERROR;
  redisContext *ctx = NULL;
  redisReply *resp = NULL;

  if (!redis || !zone || !key || !val) {
    return NGX_ERROR;
  }

  ctx = ngx_http_keyval_redis_get_context(r, redis);
  if (ctx == NULL) {
    return NGX_ERROR;
  }

  if (redis->ttl == 0) {
    resp = (redisReply *) redisCommand(ctx, "SET %b:%b %b",
                                       zone->data, zone->len,
                                       key->data, key->len,
                                       val->data, val->len);
  } else {
    resp = (redisReply *) redisCommand(ctx, "SETEX %b:%b %d %b",
                                       zone->data, zone->len,
                                       key->data, key->len,
                                       redis->ttl, val->data, val->len);
  }

  if (!resp) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                  "keyval: failed to command redis: SET|SETEX");
    return NGX_ERROR;
  }

  if (resp->type == REDIS_REPLY_ERROR) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                  "keyval: failed to command redis: SET|SETEX: %s", resp->str);
  } else {
    rc = NGX_OK;
  }

  freeReplyObject(resp);

  return rc;
}
#endif

static void
ngx_http_keyval_variable_set_handler(ngx_http_request_t *r,
                                     ngx_http_variable_value_t *v,
                                     uintptr_t data)
{
  ngx_str_t key, val;
  ngx_keyval_zone_t *zone;

  if (ngx_http_keyval_variable_init(r, data, &key, &zone) != NGX_OK) {
    return;
  }

  val.data = v->data;
  val.len = v->len;

  if (zone->type == NGX_KEYVAL_ZONE_SHM) {
    ngx_keyval_shm_ctx_t *ctx;

    ctx = ngx_keyval_shm_get_context(zone->shm, r->connection->log);
    ngx_keyval_shm_set_data(ctx, zone->shm, &key, &val, r->connection->log);
#if (NGX_HAVE_HTTP_KEYVAL_ZONE_REDIS)
  } else if (zone->type == NGX_KEYVAL_ZONE_REDIS) {
    ngx_http_keyval_redis_set_data(r, &zone->redis, &zone->name, &key, &val);
#endif
  } else {
    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                  "keyval: rejected due to wrong zone type");
  }
}

static ngx_int_t
ngx_http_keyval_variable_get_handler(ngx_http_request_t *r,
                                     ngx_http_variable_value_t *v,
                                     uintptr_t data)
{
  ngx_int_t rc;
  ngx_str_t key, val;
  ngx_keyval_zone_t *zone;

  if (ngx_http_keyval_variable_init(r, data, &key, &zone) != NGX_OK) {
    v->not_found = 1;
    return NGX_OK;
  }

  if (zone->type == NGX_KEYVAL_ZONE_SHM) {
    ngx_keyval_shm_ctx_t *ctx;

    ctx = ngx_keyval_shm_get_context(zone->shm, r->connection->log);
    rc = ngx_keyval_shm_get_data(ctx, zone->shm, &key, &val);
#if (NGX_HAVE_HTTP_KEYVAL_ZONE_REDIS)
  } else if (zone->type == NGX_KEYVAL_ZONE_REDIS) {
    rc = ngx_http_keyval_redis_get_data(r,
                                        &zone->redis, &zone->name,
                                        &key, &val);
#endif
  } else {
    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                  "keyval: rejected due to wrong zone type");
    v->not_found = 1;
    return NGX_OK;
  }

  if (rc == NGX_OK) {
    v->data = val.data;
    v->len = val.len;
  } else {
    v->data = NULL;
    v->len = 0;
  }
  v->valid = 1;
  v->no_cacheable = 0;
  v->not_found = 0;

  return NGX_OK;
}
