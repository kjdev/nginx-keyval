#include <ngx_event.h>
#include "ngx_keyval.h"

static void
ngx_keyval_rbtree_insert_value(ngx_rbtree_node_t *temp,
                               ngx_rbtree_node_t *node,
                               ngx_rbtree_node_t *sentinel)
{
  ngx_rbtree_node_t **p;
  ngx_keyval_node_t *n, *nt;

  for ( ;; ) {
    if (node->key < temp->key) {
      p = &temp->left;
    } else if (node->key > temp->key) {
      p = &temp->right;
    } else { /* node->key == temp->key */
      n = (ngx_keyval_node_t *) &node->color;
      nt = (ngx_keyval_node_t *) &temp->color;
      p = (ngx_memn2cmp(n->data, nt->data, n->len, nt->len) < 0)
        ? &temp->left : &temp->right;
    }
    if (*p == sentinel) {
      break;
    }
    temp = *p;
  }

  *p = node;
  node->parent = temp;
  node->left = sentinel;
  node->right = sentinel;
  ngx_rbt_red(node);
}

static ngx_rbtree_node_t *
ngx_keyval_rbtree_lookup(ngx_rbtree_t *rbtree, ngx_str_t *key, uint32_t hash)
{
  ngx_int_t rc;
  ngx_rbtree_node_t *node, *sentinel;
  ngx_keyval_node_t *n;

  node = rbtree->root;
  sentinel = rbtree->sentinel;

  while (node != sentinel) {
    if (hash < node->key) {
      node = node->left;
      continue;
    }

    if (hash > node->key) {
      node = node->right;
      continue;
    }

    /* hash == node->key */
    n = (ngx_keyval_node_t *) &node->color;

    rc = ngx_memn2cmp(key->data, n->data, key->len, (size_t) n->len);
    if (rc == 0) {
      return node;
    }

    node = (rc < 0) ? node->left : node->right;
  }

  /* not found */
  return NULL;
}

static ngx_int_t
ngx_keyval_init_zone(ngx_shm_zone_t *shm_zone, void *data)
{
  size_t len;
  ngx_keyval_shm_ctx_t *ctx, *octx;

  octx = data;
  ctx = shm_zone->data;

  if (octx) {
    ctx->sh = octx->sh;
    ctx->shpool = octx->shpool;
    return NGX_OK;
  }

  ctx->shpool = (ngx_slab_pool_t *) shm_zone->shm.addr;

  if (shm_zone->shm.exists) {
    ctx->sh = ctx->shpool->data;
    return NGX_OK;
  }

  ctx->sh = ngx_slab_alloc(ctx->shpool, sizeof(ngx_keyval_sh_t));
  if (ctx->sh == NULL) {
    return NGX_ERROR;
  }

  ctx->shpool->data = ctx->sh;

  ngx_rbtree_init(&ctx->sh->rbtree, &ctx->sh->sentinel,
                  ngx_keyval_rbtree_insert_value);

  len = sizeof(" in keyval zone \"\"") + shm_zone->shm.name.len;

  ctx->shpool->log_ctx = ngx_slab_alloc(ctx->shpool, len);
  if (ctx->shpool->log_ctx == NULL) {
    return NGX_ERROR;
  }

  ngx_sprintf(ctx->shpool->log_ctx, " in in keyval zone \"%V\"%Z",
              &shm_zone->shm.name);

  ctx->shpool->log_nomem = 0;

  return NGX_OK;
}

#if (NGX_HAVE_KEYVAL_ZONE_REDIS)
static ngx_int_t
ngx_keyval_init_zone_none(ngx_shm_zone_t *shm_zone, void *data)
{
  return NGX_OK;
}
#endif

static ngx_keyval_zone_t *
ngx_keyval_conf_zone_get(ngx_conf_t *cf, ngx_command_t *cmd,
                         ngx_keyval_conf_t *conf, ngx_str_t *name)
{
  ngx_uint_t i;
  ngx_keyval_zone_t *zone;

  if (!conf || !conf->zones || conf->zones->nelts == 0) {
    return NULL;
  }

  zone = conf->zones->elts;

  for (i = 0; i < conf->zones->nelts; i++) {
    if (ngx_memn2cmp(zone[i].name.data, name->data,
                     zone[i].name.len, name->len) == 0) {
      return &zone[i];
    }
  }

  return NULL;
}

static ngx_keyval_zone_t *
ngx_keyval_conf_zone_add(ngx_conf_t *cf, ngx_command_t *cmd,
                         ngx_keyval_conf_t *conf, ngx_str_t *name,
                         ngx_keyval_zone_type_t type)
{
  ngx_keyval_zone_t *zone;

  if (!conf) {
    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "\"%V\" failed to main configuration", &cmd->name);
    return NULL;
  }

  if (conf->zones == NULL) {
    conf->zones = ngx_array_create(cf->pool, 1, sizeof(*zone));
    if (conf->zones == NULL) {
      ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                         "\"%V\" failed to allocate", &cmd->name);
      return NULL;
    }
  }

  if (ngx_keyval_conf_zone_get(cf, cmd, conf, name) != NULL) {
    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "\"%V\" duplicate \"zone=%V\" parameter",
                       &cmd->name, name);
    return NULL;
  }

  zone = ngx_array_push(conf->zones);
  if (zone == NULL) {
    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "\"%V\" failed to allocate iteam", &cmd->name);
    return NULL;
  }

  zone->name = *name;
  zone->type = type;

  return zone;
}

char *
ngx_keyval_conf_set_zone(ngx_conf_t *cf, ngx_command_t *cmd, void *conf,
                         ngx_keyval_conf_t *config, void *tag)
{
  ssize_t size;
  ngx_uint_t i;
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
    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "\"%V\" failed to allocate memory or \"%V\" is already",
                       &cmd->name, &name);
    return NGX_CONF_ERROR;
  }

  shm_zone->init = ngx_keyval_init_zone;
  shm_zone->data = ctx;

  ctx->ttl = 0;

  for (i = 2; i < cf->args->nelts; i++) {
    ngx_str_t s = ngx_null_string;

    if (ngx_strncmp(value[i].data, "ttl=", 4) == 0 && value[i].len > 4) {
      s.len = value[i].len - 4;
      s.data = value[i].data + 4;
    } else if (ngx_strncmp(value[i].data, "timeout=", 8) == 0
               && value[i].len > 8) {
      s.len = value[i].len - 8;
      s.data = value[i].data + 8;
    } else {
      ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                         "\"%V\" invalid parameter \"%V\"",
                         &cmd->name, &value[i]);
      return NGX_CONF_ERROR;
    }

    if (ctx->ttl != 0) {
      ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                         "\"%V\" duplicate parameter \"%V\"",
                         &cmd->name, &value[i]);
      return NGX_CONF_ERROR;
    }

    ctx->ttl = ngx_parse_time(&s, 1);
    if (ctx->ttl == (time_t) NGX_ERROR) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "\"%V\" invalid parameter \"%V\"",
                           &cmd->name, &value[2]);
        return NGX_CONF_ERROR;
    }
  }

  return NGX_CONF_OK;
}

#if (NGX_HAVE_KEYVAL_ZONE_REDIS)
char *
ngx_keyval_conf_set_zone_redis(ngx_conf_t *cf, ngx_command_t *cmd, void *conf,
                               ngx_keyval_conf_t *config, void *tag)
{
  ssize_t size = 8 * ngx_pagesize;
  ngx_uint_t i;
  ngx_shm_zone_t *shm_zone;
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

  /* NOTE: for used check */
  shm_zone = ngx_shared_memory_add(cf, &name, size, tag);
  if (shm_zone == NULL) {
    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "\"%V\" failed to allocate memory or \"%V\" is already",
                       &cmd->name, &name);
    return NGX_CONF_ERROR;
  }
  shm_zone->init = ngx_keyval_init_zone_none;

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
#endif

char *
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

void *
ngx_keyval_create_main_conf(ngx_conf_t *cf)
{
  ngx_keyval_conf_t *conf;

  conf = ngx_pcalloc(cf->pool, sizeof(ngx_keyval_conf_t));
  if (conf == NULL) {
    return NULL;
  }

  conf->variables = NULL;
  conf->zones = NULL;

  return conf;
}

ngx_keyval_shm_ctx_t *
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

ngx_int_t
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

static void
ngx_keyval_delete_timeout_node_shm(ngx_event_t *node_status)
{
  ngx_keyval_node_timeout_t *arg
    = (ngx_keyval_node_timeout_t *) node_status->data;

  if (arg->ctx->shpool != NULL && arg->node != NULL) {
    ngx_rbtree_delete(&arg->ctx->sh->rbtree, arg->node);
    ngx_slab_free(arg->ctx->shpool, arg->node);
    ngx_slab_free(arg->ctx->shpool, arg);
  }
}

ngx_int_t
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

    if (ctx->ttl) {
      ngx_event_t *timeout_node_event
        = ngx_slab_alloc_locked(ctx->shpool, sizeof(ngx_event_t));

      if (timeout_node_event == NULL) {
        ngx_log_error(NGX_LOG_ERR, log, 0,
                      "keyval: failed to allocate timeout event");
        rc = NGX_ERROR;
      } else {
        ngx_keyval_node_timeout_t *timeout_node
          = ngx_slab_alloc_locked(ctx->shpool,
                                  sizeof(ngx_keyval_node_timeout_t));

        if (timeout_node == NULL) {
          ngx_log_error(NGX_LOG_ERR, log, 0,
                        "keyval: failed to allocate timeout node");
          rc = NGX_ERROR;
          ngx_slab_free_locked(ctx->shpool, timeout_node_event);
        } else {
          timeout_node->node = node;
          timeout_node->ctx = ctx;

          timeout_node_event->data = (void *) timeout_node;
          timeout_node_event->handler = ngx_keyval_delete_timeout_node_shm;
          timeout_node_event->log = shm->shm.log;
          ngx_add_timer(timeout_node_event, ctx->ttl * 1000);
        }
      }
    }
  }

  ngx_shmtx_unlock(&ctx->shpool->mutex);

  return rc;
}

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
