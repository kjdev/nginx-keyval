/*
 * Copyright (c) Tatsuya Kamijo
 * Copyright (c) Bengo4.com, Inc.
 */

#include "ngx_keyval.h"


static ngx_int_t
ngx_keyval_store_shm_get(ngx_keyval_zone_t *zone, ngx_str_t *key,
    ngx_str_t *val, ngx_pool_t *pool, ngx_log_t *log)
{
    ngx_keyval_shm_ctx_t *ctx;

    ctx = ngx_keyval_shm_get_context(zone->shm, log);
    if (ctx == NULL) {
        return NGX_ERROR;
    }

    return ngx_keyval_shm_get_data(ctx, zone->shm, key, val);
}


static void
ngx_keyval_store_shm_set(ngx_keyval_zone_t *zone, ngx_str_t *key,
    ngx_str_t *val, ngx_pool_t *pool, ngx_log_t *log)
{
    ngx_keyval_shm_ctx_t *ctx;

    ctx = ngx_keyval_shm_get_context(zone->shm, log);
    if (ctx == NULL) {
        return;
    }

    ngx_keyval_shm_set_data(ctx, zone->shm, key, val, log);
}


ngx_keyval_store_ops_t ngx_keyval_store_shm_ops = {
    ngx_keyval_store_shm_get,
    ngx_keyval_store_shm_set
};


#if (NGX_HAVE_KEYVAL_ZONE_REDIS)

static ngx_keyval_redis_ctx_t *
ngx_keyval_store_redis_get_ctx(ngx_pool_t *pool, ngx_log_t *log)
{
    ngx_pool_cleanup_t *cln;
    ngx_keyval_redis_ctx_t *ctx;

    /* find existing Redis ctx registered on this pool */
    for (cln = pool->cleanup; cln; cln = cln->next) {
        if (cln->handler == ngx_keyval_redis_cleanup_ctx) {
            return cln->data;
        }
    }

    /* create new ctx and register cleanup */
    ctx = ngx_pcalloc(pool, sizeof(ngx_keyval_redis_ctx_t));
    if (ctx == NULL) {
        ngx_log_error(NGX_LOG_CRIT, log, 0,
                      "keyval: failed to allocate redis context");
        return NULL;
    }

    cln = ngx_pool_cleanup_add(pool, 0);
    if (cln == NULL) {
        ngx_log_error(NGX_LOG_CRIT, log, 0,
                      "keyval: failed to allocate redis context cleanup");
        return NULL;
    }
    cln->handler = ngx_keyval_redis_cleanup_ctx;
    cln->data = ctx;

    return ctx;
}


static ngx_int_t
ngx_keyval_store_redis_get(ngx_keyval_zone_t *zone, ngx_str_t *key,
    ngx_str_t *val, ngx_pool_t *pool, ngx_log_t *log)
{
    ngx_keyval_redis_ctx_t *ctx;
    redisContext *context;

    ctx = ngx_keyval_store_redis_get_ctx(pool, log);
    if (ctx == NULL) {
        return NGX_ERROR;
    }

    context = ngx_keyval_redis_get_context(ctx, &zone->redis, log);
    if (context == NULL) {
        return NGX_ERROR;
    }

    return ngx_keyval_redis_get_data(context, &zone->name, key, val, pool, log);
}


static void
ngx_keyval_store_redis_set(ngx_keyval_zone_t *zone, ngx_str_t *key,
    ngx_str_t *val, ngx_pool_t *pool, ngx_log_t *log)
{
    ngx_keyval_redis_ctx_t *ctx;
    redisContext *context;

    ctx = ngx_keyval_store_redis_get_ctx(pool, log);
    if (ctx == NULL) {
        return;
    }

    context = ngx_keyval_redis_get_context(ctx, &zone->redis, log);
    if (context == NULL) {
        return;
    }

    ngx_keyval_redis_set_data(context, &zone->redis, &zone->name, key, val,
                              log);
}


ngx_keyval_store_ops_t ngx_keyval_store_redis_ops = {
    ngx_keyval_store_redis_get,
    ngx_keyval_store_redis_set
};

#endif


ngx_int_t
ngx_keyval_store_get(ngx_keyval_zone_t *zone, ngx_str_t *key, ngx_str_t *val,
    ngx_pool_t *pool, ngx_log_t *log)
{
    if (zone == NULL || zone->store == NULL || zone->store->get == NULL) {
        return NGX_ERROR;
    }

    return zone->store->get(zone, key, val, pool, log);
}


void
ngx_keyval_store_set(ngx_keyval_zone_t *zone, ngx_str_t *key, ngx_str_t *val,
    ngx_pool_t *pool, ngx_log_t *log)
{
    if (zone == NULL || zone->store == NULL || zone->store->set == NULL) {
        return;
    }

    zone->store->set(zone, key, val, pool, log);
}
