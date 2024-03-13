#ifndef NGX_KEYVAL_H
#define NGX_KEYVAL_H

#include <ngx_config.h>
#include <ngx_core.h>
#if (NGX_HAVE_KEYVAL_ZONE_REDIS)
#include <hiredis/hiredis.h>
#endif

typedef enum {
  NGX_KEYVAL_ZONE_SHM,
  NGX_KEYVAL_ZONE_REDIS
} ngx_keyval_zone_type_t;

typedef struct {
  ngx_array_t *variables;
  ngx_array_t *zones;
} ngx_keyval_conf_t;

typedef struct {
  u_char color;
  size_t len;
  size_t size;
  u_char data[1];
} ngx_keyval_node_t;

typedef struct {
  ngx_rbtree_t rbtree;
  ngx_rbtree_node_t sentinel;
} ngx_keyval_sh_t;

typedef struct {
  ngx_keyval_sh_t *sh;
  ngx_slab_pool_t *shpool;
  time_t ttl;
} ngx_keyval_shm_ctx_t;

typedef struct {
  ngx_rbtree_node_t *node;
  ngx_keyval_shm_ctx_t *ctx;
} ngx_keyval_node_timeout_t;

typedef struct {
  u_char *hostname;
  ngx_int_t port;
  ngx_int_t db;
  time_t ttl;
  time_t connect_timeout;
} ngx_keyval_redis_conf_t;

typedef struct {
  ngx_str_t name;
  ngx_keyval_zone_type_t type;
  ngx_shm_zone_t *shm;
  ngx_keyval_redis_conf_t redis;
} ngx_keyval_zone_t;

typedef struct {
  ngx_int_t key_indexes[15];
  ngx_int_t num_indexes;
  ngx_str_t key_string;
  ngx_str_t variable;
  ngx_keyval_zone_t *zone;
} ngx_keyval_variable_t;

#if (NGX_HAVE_KEYVAL_ZONE_REDIS)
typedef struct {
  redisContext *redis;
} ngx_keyval_redis_ctx_t;
#endif

typedef ngx_int_t (*ngx_keyval_get_variable_index)(ngx_conf_t *cf, ngx_str_t *name);
typedef ngx_variable_value_t *(*ngx_keyval_get_index_variable)(void *data, ngx_uint_t index);

char *ngx_keyval_conf_set_zone(ngx_conf_t *cf, ngx_command_t *cmd, void *conf, ngx_keyval_conf_t *config, void *tag);
#if (NGX_HAVE_KEYVAL_ZONE_REDIS)
char *ngx_keyval_conf_set_zone_redis(ngx_conf_t *cf, ngx_command_t *cmd, void *conf, ngx_keyval_conf_t *config, void *tag);
#endif
char *ngx_keyval_conf_set_variable(ngx_conf_t *cf, ngx_command_t *cmd, void *conf, ngx_keyval_conf_t *config, void *tag, ngx_keyval_variable_t **var, ngx_keyval_get_variable_index get_variable_index);
ngx_int_t ngx_keyval_variable_get_key(ngx_connection_t *connection, ngx_keyval_variable_t *var, ngx_str_t *key, ngx_keyval_get_index_variable get_index_variable, void *data);

void *ngx_keyval_create_main_conf(ngx_conf_t *cf);

ngx_keyval_shm_ctx_t *ngx_keyval_shm_get_context(ngx_shm_zone_t *shm, ngx_log_t *log);
ngx_int_t ngx_keyval_shm_get_data(ngx_keyval_shm_ctx_t *ctx, ngx_shm_zone_t *shm, ngx_str_t *key, ngx_str_t *val);
ngx_int_t ngx_keyval_shm_set_data(ngx_keyval_shm_ctx_t *ctx, ngx_shm_zone_t *shm, ngx_str_t *key, ngx_str_t *val, ngx_log_t *log);

#if (NGX_HAVE_KEYVAL_ZONE_REDIS)
void ngx_keyval_redis_cleanup_ctx(void *data);
redisContext *ngx_keyval_redis_get_context(ngx_keyval_redis_ctx_t *ctx, ngx_keyval_redis_conf_t *conf, ngx_log_t *log);
ngx_int_t ngx_keyval_redis_get_data(redisContext *ctx, ngx_str_t *zone, ngx_str_t *key, ngx_str_t *val, ngx_pool_t *pool, ngx_log_t *log);
ngx_int_t ngx_keyval_redis_set_data(redisContext *ctx, ngx_keyval_redis_conf_t *conf, ngx_str_t *zone, ngx_str_t *key, ngx_str_t *val, ngx_log_t *log);
#endif

#endif /* NGX_KEYVAL_H */
