#include <ngx_stream.h>
#include "ngx_keyval.h"

static char *ngx_stream_keyval_conf_set_zone(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
#if (NGX_HAVE_KEYVAL_ZONE_REDIS)
static char *ngx_stream_keyval_conf_set_zone_redis(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
#endif
static char *ngx_stream_keyval_conf_set_variable(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static void ngx_stream_keyval_variable_set_handler(ngx_stream_session_t *s, ngx_stream_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_stream_keyval_variable_get_handler(ngx_stream_session_t *s, ngx_stream_variable_value_t *v, uintptr_t data);

static ngx_command_t ngx_stream_keyval_commands[] = {
  { ngx_string("keyval"),
    NGX_STREAM_MAIN_CONF|NGX_CONF_TAKE3,
    ngx_stream_keyval_conf_set_variable,
    0,
    0,
    NULL },
  { ngx_string("keyval_zone"),
    NGX_STREAM_MAIN_CONF|NGX_CONF_1MORE,
    ngx_stream_keyval_conf_set_zone,
    0,
    0,
    NULL },
#if (NGX_HAVE_KEYVAL_ZONE_REDIS)
  { ngx_string("keyval_zone_redis"),
    NGX_STREAM_MAIN_CONF|NGX_CONF_1MORE,
    ngx_stream_keyval_conf_set_zone_redis,
    0,
    0,
    NULL },
#endif
  ngx_null_command
};

static ngx_stream_module_t ngx_stream_keyval_module_ctx = {
  NULL,                               /* preconfiguration */
  NULL,                               /* postconfiguration */
  ngx_keyval_create_main_conf,        /* create main configuration */
  NULL,                               /* init main configuration */
  NULL,                               /* create server configuration */
  NULL,                               /* merge server configuration */
};

ngx_module_t ngx_stream_keyval_module = {
  NGX_MODULE_V1,
  &ngx_stream_keyval_module_ctx, /* module context */
  ngx_stream_keyval_commands,    /* module directives */
  NGX_STREAM_MODULE,             /* module type */
  NULL,                          /* init master */
  NULL,                          /* init module */
  NULL,                          /* init process */
  NULL,                          /* init thread */
  NULL,                          /* exit thread */
  NULL,                          /* exit process */
  NULL,                          /* exit master */
  NGX_MODULE_V1_PADDING
};

static char *
ngx_stream_keyval_conf_set_zone(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
  ngx_keyval_conf_t *config;

  config = ngx_stream_conf_get_module_main_conf(cf, ngx_stream_keyval_module);

  return ngx_keyval_conf_set_zone(cf, cmd, conf,
                                  config, &ngx_stream_keyval_module);
}

#if (NGX_HAVE_KEYVAL_ZONE_REDIS)
static char *
ngx_stream_keyval_conf_set_zone_redis(ngx_conf_t *cf,
                                      ngx_command_t *cmd, void *conf)
{
  ngx_keyval_conf_t *config;

  config = ngx_stream_conf_get_module_main_conf(cf, ngx_stream_keyval_module);

  return ngx_keyval_conf_set_zone_redis(cf, cmd, conf,
                                        config, &ngx_stream_keyval_module);
}
#endif

static char *
ngx_stream_keyval_conf_set_variable(ngx_conf_t *cf,
                                    ngx_command_t *cmd, void *conf)
{
  char *retval;
  ngx_uint_t flags;
  ngx_stream_variable_t *v;
  ngx_keyval_conf_t *config;
  ngx_keyval_variable_t *var = NULL;

  config = ngx_stream_conf_get_module_main_conf(cf, ngx_stream_keyval_module);

  retval = ngx_keyval_conf_set_variable(cf, cmd, conf,
                                        config, &ngx_stream_keyval_module,
                                        &var, ngx_stream_get_variable_index);
  if (retval != NGX_CONF_OK) {
    return retval;
  }
  if (!var) {
    return "failed to allocate";
  }

  /* add variable */
  flags = NGX_STREAM_VAR_CHANGEABLE | NGX_STREAM_VAR_NOCACHEABLE;
  v = ngx_stream_add_variable(cf, &(var->variable), flags);
  if (v == NULL) {
    return "failed to add variable";
  }

  v->get_handler = ngx_stream_keyval_variable_get_handler;
  v->set_handler = ngx_stream_keyval_variable_set_handler;
  v->data = (uintptr_t) var;

  return NGX_CONF_OK;
}

static ngx_int_t
ngx_stream_keyval_variable_get_key(ngx_stream_session_t *s,
                                   ngx_keyval_variable_t *var, ngx_str_t *key)
{
  if (!key || !var) {
    return NGX_ERROR;
  }

  if (var->key_index != NGX_CONF_UNSET) {
    ngx_stream_variable_value_t *v;
    v = ngx_stream_get_indexed_variable(s, var->key_index);
    if (v == NULL || v->not_found) {
      ngx_log_error(NGX_LOG_INFO, s->connection->log, 0,
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
ngx_stream_keyval_variable_init(ngx_stream_session_t *s, uintptr_t data,
                                ngx_str_t *key, ngx_keyval_zone_t **zone)
{
  ngx_keyval_conf_t *cf;
  ngx_keyval_variable_t *var;

  if (data == 0) {
    ngx_log_error(NGX_LOG_INFO, s->connection->log, 0,
                  "keyval: rejected due to not handler data");
    return NGX_ERROR;
  }

  cf = ngx_stream_get_module_main_conf(s, ngx_stream_keyval_module);
  if (!cf) {
    ngx_log_error(NGX_LOG_INFO, s->connection->log, 0,
                  "keyval: rejected due to not found main configuration");
    return NGX_ERROR;
  }

  var = (ngx_keyval_variable_t *) data;

  if (ngx_stream_keyval_variable_get_key(s, var, key) != NGX_OK) {
    ngx_log_error(NGX_LOG_INFO, s->connection->log, 0,
                  "keyval: rejected due to not found variable key");
    return NGX_ERROR;
  }

  if (!var->zone) {
    ngx_log_error(NGX_LOG_INFO, s->connection->log, 0,
                  "keyval: rejected due to not found variable zone");
    return NGX_ERROR;
  }

  *zone = var->zone;

  return NGX_OK;
}

#if (NGX_HAVE_KEYVAL_ZONE_REDIS)
static ngx_keyval_redis_ctx_t *
ngx_stream_keyval_redis_get_ctx(ngx_stream_session_t *s)
{
  ngx_pool_cleanup_t *cleanup;
  ngx_keyval_redis_ctx_t *ctx;

  ctx = ngx_stream_get_module_ctx(s, ngx_stream_keyval_module);
  if (ctx != NULL) {
    return ctx;
  }

  ctx = ngx_pcalloc(s->connection->pool, sizeof(ngx_keyval_redis_ctx_t));
  if (ctx == NULL) {
    ngx_log_error(NGX_LOG_CRIT, s->connection->log, 0,
                  "keyval: failed to allocate redis context");
    return NULL;
  }

  ctx->redis = NULL;

  ngx_stream_set_ctx(s, ctx, ngx_stream_keyval_module);

  cleanup = ngx_pool_cleanup_add(s->connection->pool, 0);
  if (cleanup == NULL) {
    ngx_log_error(NGX_LOG_CRIT, s->connection->log, 0,
                  "keyval: failed to allocate redis context cleanup");
    return NULL;
  }
  cleanup->handler = ngx_keyval_redis_cleanup_ctx;
  cleanup->data = ctx;

  return ctx;
}
#endif

static void
ngx_stream_keyval_variable_set_handler(ngx_stream_session_t *s,
                                       ngx_stream_variable_value_t *v,
                                       uintptr_t data)
{
  ngx_str_t key, val;
  ngx_keyval_zone_t *zone;

  if (ngx_stream_keyval_variable_init(s, data, &key, &zone) != NGX_OK) {
    return;
  }

  val.data = v->data;
  val.len = v->len;

  if (zone->type == NGX_KEYVAL_ZONE_SHM) {
    ngx_keyval_shm_ctx_t *ctx;

    ctx = ngx_keyval_shm_get_context(zone->shm, s->connection->log);
    ngx_keyval_shm_set_data(ctx, zone->shm, &key, &val, s->connection->log);
#if (NGX_HAVE_KEYVAL_ZONE_REDIS)
  } else if (zone->type == NGX_KEYVAL_ZONE_REDIS) {
    ngx_keyval_redis_ctx_t *ctx;
    redisContext *context;

    ctx = ngx_stream_keyval_redis_get_ctx(s);
    context = ngx_keyval_redis_get_context(ctx, &zone->redis,
                                           s->connection->log);
    ngx_keyval_redis_set_data(context, &zone->redis, &zone->name, &key, &val,
                              s->connection->log);
#endif
  } else {
    ngx_log_error(NGX_LOG_INFO, s->connection->log, 0,
                  "keyval: rejected due to wrong zone type");
  }
}

static ngx_int_t
ngx_stream_keyval_variable_get_handler(ngx_stream_session_t *s,
                                       ngx_stream_variable_value_t *v,
                                       uintptr_t data)
{
  ngx_int_t rc;
  ngx_str_t key, val;
  ngx_keyval_zone_t *zone;

  if (ngx_stream_keyval_variable_init(s, data, &key, &zone) != NGX_OK) {
    v->not_found = 1;
    return NGX_OK;
  }

  if (zone->type == NGX_KEYVAL_ZONE_SHM) {
    ngx_keyval_shm_ctx_t *ctx;

    ctx = ngx_keyval_shm_get_context(zone->shm, s->connection->log);
    rc = ngx_keyval_shm_get_data(ctx, zone->shm, &key, &val);
#if (NGX_HAVE_KEYVAL_ZONE_REDIS)
  } else if (zone->type == NGX_KEYVAL_ZONE_REDIS) {
    ngx_keyval_redis_ctx_t *ctx;
    redisContext *context;

    ctx = ngx_stream_keyval_redis_get_ctx(s);
    context = ngx_keyval_redis_get_context(ctx, &zone->redis,
                                           s->connection->log);
    rc = ngx_keyval_redis_get_data(context, &zone->name, &key, &val,
                                   s->connection->pool, s->connection->log);
#endif
  } else {
    ngx_log_error(NGX_LOG_INFO, s->connection->log, 0,
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
