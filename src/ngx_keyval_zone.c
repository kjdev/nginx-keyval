/*
 * Copyright (c) Tatsuya Kamijo
 * Copyright (c) Bengo4.com, Inc.
 */

#include "ngx_keyval.h"


#define NGX_KEYVAL_STORE_VERSION "1"
#define NGX_KEYVAL_SHM_SIGNATURE "keyval-store:" NGX_KEYVAL_STORE_VERSION

ngx_keyval_zone_t *
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
                         zone[i].name.len, name->len) == 0)
        {
            return &zone[i];
        }
    }

    return NULL;
}

ngx_keyval_zone_t *
ngx_keyval_conf_zone_add(ngx_conf_t *cf, ngx_command_t *cmd,
    ngx_keyval_conf_t *conf, ngx_str_t *name,
    ngx_keyval_zone_type_t type)
{
    ngx_keyval_zone_t *zone;

    if (!conf) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "\"%V\" failed to get main configuration",
                           &cmd->name);
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
                           "\"%V\" failed to allocate item", &cmd->name);
        return NULL;
    }

    ngx_memzero(zone, sizeof(*zone));

    zone->name = *name;
    zone->type = type;

    return zone;
}

char *
ngx_keyval_conf_set_zone(ngx_conf_t *cf, ngx_command_t *cmd, void *conf,
    ngx_keyval_conf_t *config, void *tag)
{
    ngx_uint_t i;
    ngx_shm_zone_t *shm_zone;
    ngx_str_t *value, tmp;
    ngx_keyval_shm_ctx_t *ctx;
    ngx_keyval_zone_t *zone;
    ngx_shm_zone_params_t   zp;

    if (!config || !tag) {
        return "missing required parameter";
    }

    value = cf->args->elts;

    ngx_memzero(&zp, sizeof(ngx_shm_zone_params_t));
    zp.min_size = 8 * ngx_pagesize;
    zp.size = NGX_CONF_UNSET;
    zp.restorable = 1;
    zp.tag = tag;

    zp.signature.len = sizeof(NGX_KEYVAL_SHM_SIGNATURE)-1;
    zp.signature.data = ngx_pnalloc(cf->pool, zp.signature.len+1);
    if (zp.signature.data == NULL) {
        return NGX_CONF_ERROR;
    }
    memcpy(zp.signature.data, NGX_KEYVAL_SHM_SIGNATURE, zp.signature.len+1);

    if (ngx_strncmp(value[1].data, "zone=", 5) == 0) {

        tmp.data = value[1].data + 5;
        tmp.len = value[1].len - 5;

        if (ngx_conf_parse_zone_spec(cf, &zp, &tmp) != NGX_OK) {
            return NGX_CONF_ERROR;
        }
    }

    if (zp.name.len == 0 || zp.size == 0) {
        return "must have \"zone\" parameter";
    }

    zone = ngx_keyval_conf_zone_add(cf, cmd, config, &zp.name,
                                    NGX_KEYVAL_ZONE_SHM);
    if (zone == NULL) {
        return NGX_CONF_ERROR;
    }

    zone->store = &ngx_keyval_store_shm_ops;

    ctx = ngx_pcalloc(cf->pool, sizeof(ngx_keyval_shm_ctx_t));
    if (ctx == NULL) {
        return "failed to allocate";
    }

    shm_zone = ngx_shared_memory_add_ext(cf, &zp);
    if (shm_zone == NULL) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "\"%V\" failed to allocate memory or \"%V\" is already",
                           &cmd->name, &zp.name);
        return NGX_CONF_ERROR;
    }

    shm_zone->init = ngx_keyval_init_zone;
    shm_zone->data = ctx;

    zone->shm = shm_zone;

    ctx->ttl = 0;

    for (i = 2; i < cf->args->nelts; i++) {
        ngx_str_t s = ngx_null_string;

        if (ngx_strncmp(value[i].data, "ttl=", 4) == 0 && value[i].len > 4) {
            s.len = value[i].len - 4;
            s.data = value[i].data + 4;
        } else if (ngx_strncmp(value[i].data, "timeout=", 8) == 0
                   && value[i].len > 8)
        {
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
                               &cmd->name, &value[i]);
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

    zone->store = &ngx_keyval_store_redis_ops;

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
    zone->redis.command_timeout = 3;

    for (i = 2; i < cf->args->nelts; i++) {
        if (ngx_strncmp(value[i].data, "hostname=",
                        9) == 0 && value[i].len > 9)
        {
            zone->redis.hostname = ngx_pnalloc(cf->pool, value[i].len - 9 + 1);
            if (zone->redis.hostname == NULL) {
                return "failed to allocate hostname";
            }
            ngx_memcpy(zone->redis.hostname, value[i].data + 9,
                       value[i].len - 9);
            zone->redis.hostname[value[i].len - 9] = '\0';
            continue;
        }

        if (ngx_strncmp(value[i].data, "port=", 5) == 0 && value[i].len > 5) {
            zone->redis.port = ngx_atoi(value[i].data + 5, value[i].len - 5);
            if (zone->redis.port <= 0 || zone->redis.port > 65535) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "\"%V\" invalid port \"%V\"",
                                   &cmd->name, &value[i]);
                return NGX_CONF_ERROR;
            }
            continue;
        }

        if (ngx_strncmp(value[i].data, "database=",
                        9) == 0 && value[i].len > 9)
        {
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
            && value[i].len > 16)
        {
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

        if (ngx_strncmp(value[i].data, "command_timeout=", 16) == 0
            && value[i].len > 16)
        {
            ngx_str_t s;

            s.len = value[i].len - 16;
            s.data = value[i].data + 16;

            zone->redis.command_timeout = ngx_parse_time(&s, 1);
            if (zone->redis.command_timeout == (time_t) NGX_ERROR
                || zone->redis.command_timeout == 0)
            {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "\"%V\" invalid command timeout \"%V\"",
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
