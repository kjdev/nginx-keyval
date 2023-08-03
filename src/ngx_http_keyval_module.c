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
ngx_keyval_conf_set_zone(ngx_conf_t *cf, ngx_command_t *cmd, void *conf,
                         ngx_keyval_conf_t *config, void *tag)
{
  ssize_t size;
  ngx_shm_zone_t *shm_zone;
  ngx_str_t name, *value;
  ngx_keyval_shm_ctx_t *ctx;
  ngx_keyval_zone_t *zone;

  if (!config || !tag) {
    return "missing required parameter";
  }

  value = cf->args->elts;

  size = 0;
  name.len = 0;

  if (ngx_strncmp(value[1].data, "zone=", 5) == 0) {
    u_char *p;
    ngx_str_t s;

    name.data = value[1].data + 5;
    p = (u_char *) ngx_strchr(name.data, ':');
    if (p == NULL) {
      ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                         "\"%V\" invalid zone size \"%V\"",
                         &cmd->name, &value[1]);
      return NGX_CONF_ERROR;
    }

    name.len = p - name.data;

    s.data = p + 1;
    s.len = value[1].data + value[1].len - s.data;

    size = ngx_parse_size(&s);

    if (size == NGX_ERROR) {
      ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                         "\"%V\" invalid zone size \"%V\"",
                         &cmd->name, &value[1]);
      return NGX_CONF_ERROR;
    }

    if (size < (ssize_t) (8 * ngx_pagesize)) {
      ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                         "\"%V\" zone \"%V\" is too small",
                         &cmd->name, &value[1]);
      return NGX_CONF_ERROR;
    }
  }

  if (name.len == 0) {
    return "must have \"zone\" parameter";
  }

  zone = ngx_keyval_conf_zone_add(cf, cmd, config, &name, NGX_KEYVAL_ZONE_SHM);
  if (zone == NULL) {
    return NGX_CONF_ERROR;
  }

  ctx = ngx_pcalloc(cf->pool, sizeof(ngx_keyval_shm_ctx_t));
  if (ctx == NULL) {
    return "failed to allocate";
  }

  shm_zone = ngx_shared_memory_add(cf, &name, size, tag);
  if (shm_zone == NULL) {
    return "failed to allocate shared memory";
  }

  shm_zone->init = ngx_keyval_init_zone;
  shm_zone->data = ctx;

  return NGX_CONF_OK;
}

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
ngx_keyval_conf_set_zone_redis(ngx_conf_t *cf, ngx_command_t *cmd, void *conf,
                               ngx_keyval_conf_t *config)
{
  ngx_uint_t i;
  ngx_str_t name, *value;
  ngx_keyval_zone_t *zone;

  if (!config) {
    return "missing required parameter";
  }

  value = cf->args->elts;

  name.len = 0;

  if (ngx_strncmp(value[1].data, "zone=", 5) == 0) {
    name.data = value[1].data + 5;
    name.len = value[1].len - 5;
  }

  if (name.len == 0) {
    return "must have \"zone\" parameter";
  }

  zone = ngx_keyval_conf_zone_add(cf, cmd, config,
                                  &name, NGX_KEYVAL_ZONE_REDIS);
  if (zone == NULL) {
    return NGX_CONF_ERROR;
  }

  /* redis default */
  zone->redis.hostname = NULL;
  zone->redis.port = 6379;
  zone->redis.db = 0;
  zone->redis.ttl = 0;
  zone->redis.connect_timeout = 3;

  for (i = 2; i < cf->args->nelts; i++) {
    if (ngx_strncmp(value[i].data, "hostname=", 9) == 0 && value[i].len > 9) {
      zone->redis.hostname = ngx_pnalloc(cf->pool, value[i].len - 9 + 1);
      if (zone->redis.hostname == NULL) {
        return "failed to allocate hostname";
      }
      ngx_memcpy(zone->redis.hostname, value[i].data + 9, value[i].len - 9);
      zone->redis.hostname[value[i].len - 9] = '\0';
      continue;
    }

    if (ngx_strncmp(value[i].data, "port=", 5) == 0 && value[i].len > 5) {
      zone->redis.port = ngx_atoi(value[i].data + 5, value[i].len - 5);
      if (zone->redis.port <= 0) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "\"%V\" invalid port \"%V\"",
                           &cmd->name, &value[i]);
        return NGX_CONF_ERROR;
      }
      continue;
    }

    if (ngx_strncmp(value[i].data, "database=", 9) == 0 && value[i].len > 9) {
      zone->redis.db = ngx_atoi(value[i].data + 9, value[i].len - 9);
      if (zone->redis.db < 0) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "\"%V\" invalid database \"%V\"",
                           &cmd->name, &value[i]);
        return NGX_CONF_ERROR;
      }
      continue;
    }

    if (ngx_strncmp(value[i].data, "ttl=", 4) == 0 && value[i].len > 4) {
      ngx_str_t s;

      s.len = value[i].len - 4;
      s.data = value[i].data + 4;

      zone->redis.ttl = ngx_parse_time(&s, 1);
      if (zone->redis.ttl == (time_t) NGX_ERROR) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "\"%V\" invalid ttl \"%V\"",
                           &cmd->name, &value[i]);
        return NGX_CONF_ERROR;
      }
      continue;
    }

    if (ngx_strncmp(value[i].data, "connect_timeout=", 16) == 0
        && value[i].len > 16) {
      ngx_str_t s;

      s.len = value[i].len - 16;
      s.data = value[i].data + 16;

      zone->redis.connect_timeout = ngx_parse_time(&s, 1);
      if (zone->redis.connect_timeout == (time_t) NGX_ERROR) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "\"%V\" invalid connect timeout \"%V\"",
                           &cmd->name, &value[i]);
        return NGX_CONF_ERROR;
      }
      continue;
    }

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "\"%V\" invalid parameter \"%V\"",
                       &cmd->name, &value[i]);
    return NGX_CONF_ERROR;
  }

  if (zone->redis.hostname == NULL) {
    zone->redis.hostname = ngx_pnalloc(cf->pool, sizeof("127.0.0.1"));
    if (zone->redis.hostname == NULL) {
      return "failed to allocate hostname";
    }
    ngx_memcpy(zone->redis.hostname, "127.0.0.1", sizeof("127.0.0.1") - 1);
    zone->redis.hostname[sizeof("127.0.0.1") - 1] = '\0';
  }

  return NGX_CONF_OK;
}

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
ngx_keyval_conf_set_variable(ngx_conf_t *cf, ngx_command_t *cmd, void *conf,
                             ngx_keyval_conf_t *config, void *tag,
                             ngx_keyval_variable_t **var,
                             ngx_keyval_get_variable_index get_variable_index)
{
  ngx_str_t *value;

  if (!config || !tag) {
    return "missing required parameter";
  }

  value = cf->args->elts;

  if (value[1].len == 0) {
    return "is empty";
  }

  if (value[2].data[0] != '$') {
    return "not a variable specified";
  }
  value[2].data++;
  value[2].len--;

  if (ngx_strncmp(value[3].data, "zone=", 5) != 0) {
    return "not a zone specified";
  }
  value[3].data += 5;
  value[3].len -= 5;

  if (config->variables == NULL) {
    config->variables = ngx_array_create(cf->pool, 4,
                                         sizeof(ngx_keyval_variable_t));
    if (config->variables == NULL) {
      return "failed to allocate";
    }
  }

  *var = ngx_array_push(config->variables);
  if (*var == NULL) {
    return "failed to allocate iteam";
  }

  if (value[1].data[0] == '$') {
    value[1].data++;
    value[1].len--;
    (*var)->key_index = get_variable_index(cf, &value[1]);
    ngx_str_null(&((*var)->key_string));
  } else {
    (*var)->key_index = NGX_CONF_UNSET;
    (*var)->key_string = value[1];
  }

  (*var)->variable = value[2];

  (*var)->zone = ngx_keyval_conf_zone_get(cf, cmd, config, &value[3]);
  if ((*var)->zone == NULL) {
    return "zone not found";
  }

  if ((*var)->zone->type == NGX_KEYVAL_ZONE_SHM) {
    (*var)->zone->shm = ngx_shared_memory_add(cf, &value[3], 0, tag);
    if ((*var)->zone->shm == NULL) {
      return "failed to allocate shared memory";
    }
  } else if ((*var)->zone->type != NGX_KEYVAL_ZONE_REDIS) {
    return "invalid zone type";
  }

  return NGX_CONF_OK;
}

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
ngx_http_keyval_shm_get_context(ngx_http_request_t *r, ngx_shm_zone_t *shm)
{
  ngx_keyval_shm_ctx_t *ctx;

  if (!shm) {
    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                  "keyval: rejected due to not found shared memory zone");
    return NULL;
  }

  ctx = shm->data;
  if (!ctx) {
    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                  "keyval: rejected due to not found shared memory context");
    return NULL;
  }

  return ctx;
}

static ngx_int_t
ngx_http_keyval_shm_get_data(ngx_http_request_t *r,
                             ngx_shm_zone_t *shm,
                             ngx_str_t *key, ngx_str_t *val)
{
  uint32_t hash;
  ngx_rbtree_node_t *node;
  ngx_keyval_node_t *kv;
  ngx_keyval_shm_ctx_t *ctx;

  if (!shm || !key || !val) {
    return NGX_ERROR;
  }

  ctx = ngx_http_keyval_shm_get_context(r, shm);
  if (!ctx) {
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
ngx_http_keyval_shm_set_data(ngx_http_request_t *r, ngx_shm_zone_t *shm,
                             ngx_str_t *key, ngx_str_t *val)
{
  uint32_t hash;
  size_t n;
  ngx_int_t rc;
  ngx_rbtree_node_t *node;
  ngx_keyval_shm_ctx_t *ctx;

  if (!shm || !key || !val) {
    return NGX_ERROR;
  }

  ctx = ngx_http_keyval_shm_get_context(r, shm);
  if (!ctx) {
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
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
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
    ngx_http_keyval_shm_set_data(r, zone->shm, &key, &val);
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
    rc = ngx_http_keyval_shm_get_data(r, zone->shm, &key, &val);
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
