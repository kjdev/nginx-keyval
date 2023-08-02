#ifndef NGX_KEYVAL_H
#define NGX_KEYVAL_H

#include <ngx_config.h>
#include <ngx_core.h>
#if (NGX_HAVE_HTTP_KEYVAL_ZONE_REDIS)
#include <hiredis/hiredis.h>
#endif

typedef enum {
  NGX_HTTP_KEYVAL_ZONE_SHM,
  NGX_HTTP_KEYVAL_ZONE_REDIS
} ngx_http_keyval_zone_type_t;

typedef struct {
  ngx_array_t *variables;
  ngx_array_t *zones;
} ngx_http_keyval_conf_t;

typedef struct {
  u_char color;
  size_t len;
  size_t size;
  u_char data[1];
} ngx_http_keyval_node_t;

typedef struct {
  ngx_rbtree_t rbtree;
  ngx_rbtree_node_t sentinel;
} ngx_http_keyval_sh_t;

typedef struct {
  ngx_http_keyval_sh_t *sh;
  ngx_slab_pool_t *shpool;
} ngx_http_keyval_shm_ctx_t;

typedef struct {
  u_char *hostname;
  ngx_int_t port;
  ngx_int_t db;
  time_t ttl;
  time_t connect_timeout;
} ngx_http_keyval_redis_t;

typedef struct {
  ngx_str_t name;
  ngx_http_keyval_zone_type_t type;
  ngx_shm_zone_t *shm;
  ngx_http_keyval_redis_t redis;
} ngx_http_keyval_zone_t;

typedef struct {
  ngx_int_t key_index;
  ngx_str_t key_string;
  ngx_str_t variable;
  ngx_http_keyval_zone_t *zone;
} ngx_http_keyval_variable_t;

#if (NGX_HAVE_HTTP_KEYVAL_ZONE_REDIS)
typedef struct {
  redisContext *redis;
} ngx_http_keyval_redis_ctx_t;
#endif

ngx_rbtree_node_t *ngx_keyval_rbtree_lookup(ngx_rbtree_t *rbtree, ngx_str_t *key, uint32_t hash);
ngx_int_t ngx_keyval_init_zone(ngx_shm_zone_t *shm_zone, void *data);
ngx_http_keyval_zone_t *ngx_keyval_conf_zone_get(ngx_conf_t *cf, ngx_command_t *cmd, ngx_http_keyval_conf_t *conf, ngx_str_t *name);
ngx_http_keyval_zone_t *ngx_keyval_conf_zone_add(ngx_conf_t *cf, ngx_command_t *cmd, ngx_http_keyval_conf_t *conf, ngx_str_t *name, ngx_http_keyval_zone_type_t type);

void *ngx_keyval_create_main_conf(ngx_conf_t *cf);

#if (NGX_HAVE_HTTP_KEYVAL_ZONE_REDIS)
void ngx_keyval_redis_cleanup_ctx(void *data);
#endif

#endif /* NGX_KEYVAL_H */
