/*
 * Copyright (c) Tatsuya Kamijo
 * Copyright (c) Bengo4.com, Inc.
 */

#include <ngx_http.h>
#include "ngx_keyval.h"

static char *ngx_http_keyval_conf_set_zone(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
#if (NGX_HAVE_KEYVAL_ZONE_REDIS)
static char *ngx_http_keyval_conf_set_zone_redis(ngx_conf_t *cf,
    ngx_command_t *cmd, void *conf);
#endif
static char *ngx_http_keyval_conf_set_variable(ngx_conf_t *cf,
    ngx_command_t *cmd, void *conf);

static void ngx_http_keyval_variable_set_handler(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_keyval_variable_get_handler(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);

static ngx_command_t ngx_http_keyval_commands[] = {
    { ngx_string("keyval"),
      NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE3,
      ngx_http_keyval_conf_set_variable,
      0,
      0,
      NULL },
    { ngx_string("keyval_zone"),
      NGX_HTTP_MAIN_CONF | NGX_CONF_1MORE,
      ngx_http_keyval_conf_set_zone,
      0,
      0,
      NULL },
#if (NGX_HAVE_KEYVAL_ZONE_REDIS)
    { ngx_string("keyval_zone_redis"),
      NGX_HTTP_MAIN_CONF | NGX_CONF_1MORE,
      ngx_http_keyval_conf_set_zone_redis,
      0,
      0,
      NULL },
#endif
    ngx_null_command
};

static ngx_http_module_t ngx_http_keyval_module_ctx = {
    NULL,                           /* preconfiguration */
    NULL,                           /* postconfiguration */
    ngx_keyval_create_main_conf,    /* create main configuration */
    NULL,                           /* init main configuration */
    NULL,                           /* create server configuration */
    NULL,                           /* merge server configuration */
    NULL,                           /* create location configuration */
    NULL                            /* merge location configuration */
};

ngx_module_t ngx_http_keyval_module = {
    NGX_MODULE_V1,
    &ngx_http_keyval_module_ctx, /* module context */
    ngx_http_keyval_commands,  /* module directives */
    NGX_HTTP_MODULE,           /* module type */
    NULL,                      /* init master */
    NULL,                      /* init module */
    NULL,                      /* init process */
    NULL,                      /* init thread */
    NULL,                      /* exit thread */
    NULL,                      /* exit process */
    NULL,                      /* exit master */
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

static ngx_variable_value_t *
ngx_http_keyval_get_indexed_variable(void *data, ngx_uint_t index)
{
    return ngx_http_get_indexed_variable((ngx_http_request_t *) data, index);
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

    if (ngx_keyval_variable_get_key(r->pool, r->connection, var, key,
                                    ngx_http_keyval_get_indexed_variable,
                                    (void *) r) != NGX_OK)
    {
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

    ngx_keyval_store_set(zone, &key, &val, r->pool, r->connection->log);
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

    rc = ngx_keyval_store_get(zone, &key, &val, r->pool, r->connection->log);

    if (rc == NGX_OK) {
        v->data = val.data;
        v->len = val.len;
        v->not_found = 0;
    } else {
        v->data = NULL;
        v->len = 0;
        v->not_found = 1;
    }
    v->valid = 1;
    v->no_cacheable = 0;

    return NGX_OK;
}
