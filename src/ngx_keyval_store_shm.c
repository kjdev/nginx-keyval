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

ngx_int_t
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

    len = sizeof(" in in keyval zone \"\"") + shm_zone->shm.name.len;

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
ngx_int_t
ngx_keyval_init_zone_none(ngx_shm_zone_t *shm_zone, void *data)
{
    return NGX_OK;
}
#endif

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
                    timeout_node_event->handler =
                        ngx_keyval_delete_timeout_node_shm;
                    timeout_node_event->log = shm->shm.log;
                    ngx_add_timer(timeout_node_event, ctx->ttl * 1000);
                }
            }
        }
    }

    ngx_shmtx_unlock(&ctx->shpool->mutex);

    return rc;
}
