#include <ngx_http.h>
#include "ngx_keyval.h"

static char *ngx_http_keyval_conf_set_zone(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
#if (NGX_HAVE_KEYVAL_ZONE_REDIS)
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
#if (NGX_HAVE_KEYVAL_ZONE_REDIS)
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

#if (NGX_HAVE_KEYVAL_ZONE_REDIS)
static char *
ngx_http_keyval_conf_set_zone_redis(ngx_conf_t *cf,
                                    ngx_command_t *cmd, void *conf)
{
  ngx_keyval_conf_t *config;

  config = ngx_http_conf_get_module_main_conf(cf, ngx_http_keyval_module);

  return ngx_keyval_conf_set_zone_redis(cf, cmd, conf,
                                        config, &ngx_http_keyval_module);
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

  if (var->num_indexes != 0) // There is at least one variable to replace
  {
    ngx_http_variable_value_t *v[var->num_indexes]; // Array to store each variable
    ngx_int_t current_index = 0; // Current variable index we're reading
    ngx_str_t string_var = var->key_string; // Pointer to doesn't change the original one
    ngx_uint_t size_string = 0; // Size of the final string with variables replaces and other characters

    for (ngx_int_t i = 0 ; i < var->num_indexes ; i++) // For each variable, verify the size and store it
    {
	    v[i] = ngx_http_get_indexed_variable(r, var->key_indexes[i]); // Get the variable

	    if (v[i] == NULL || v[i]-> not_found) // Sanity check
	    {
		    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
				    "keyval: variable specified was not provided");
		    return NGX_ERROR;
	    }

	    size_string += v[i]->len; // Increments final string size
    }

    /* Variable that holds the final string and it's allocated with the exactly size it needs */

    key->data = (u_char *) ngx_pnalloc(r->pool, size_string + (string_var.len - var->num_indexes) + 1);
    key->len = 0;

    u_char *last_space_available = key->data; // The same purpose as size_string var, but for the final string space

    for ( ; *(string_var.data) != '\0' ; string_var.data++) // Walks by the intermediate string
    {
	    if (*(string_var.data) == '$') // A variable belongs here. Do the replace in the variable order we stored
	    {
		    last_space_available = ngx_cpystrn(last_space_available, v[current_index]->data, v[current_index]->len + 1); // Replace $ by the content of the variable
		    key->len += v[current_index++]->len; // Increments string size
	    }

	    else // Common char, just copy
	    {
		    *last_space_available = *(string_var.data);
		    last_space_available += sizeof(u_char); // Increments the pointer by the size of the data type
		    key->len++;
	    }
    }

    *last_space_available = '\0'; // Ends the string
  }

  else // No vars, just copy
	  *key = var->key_string;

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

#if (NGX_HAVE_KEYVAL_ZONE_REDIS)
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
#if (NGX_HAVE_KEYVAL_ZONE_REDIS)
  } else if (zone->type == NGX_KEYVAL_ZONE_REDIS) {
    ngx_keyval_redis_ctx_t *ctx;
    redisContext *context;

    ctx = ngx_http_keyval_redis_get_ctx(r);
    context = ngx_keyval_redis_get_context(ctx, &zone->redis,
                                           r->connection->log);
    ngx_keyval_redis_set_data(context, &zone->redis, &zone->name, &key, &val,
                              r->connection->log);
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
#if (NGX_HAVE_KEYVAL_ZONE_REDIS)
  } else if (zone->type == NGX_KEYVAL_ZONE_REDIS) {
    ngx_keyval_redis_ctx_t *ctx;
    redisContext *context;

    ctx = ngx_http_keyval_redis_get_ctx(r);
    context = ngx_keyval_redis_get_context(ctx, &zone->redis,
                                           r->connection->log);
    rc = ngx_keyval_redis_get_data(context, &zone->name, &key, &val,
                                   r->pool, r->connection->log);
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
