
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static char *ngx_http_keyval_conf_set_zone(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_keyval_conf_set_variable(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static void *ngx_http_keyval_create_main_conf(ngx_conf_t *cf);

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
  ngx_null_command
};

static ngx_http_module_t ngx_http_keyval_module_ctx = {
  NULL,                             /* preconfiguration */
  NULL,                             /* postconfiguration */
  ngx_http_keyval_create_main_conf, /* create main configuration */
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

typedef struct {
  ngx_array_t *variables;
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
} ngx_http_keyval_ctx_t;

typedef struct {
  ngx_int_t key_index;
  ngx_str_t key_string;
  ngx_str_t variable;
  ngx_shm_zone_t *shm_zone;
} ngx_http_keyval_variable_t;

static void
ngx_http_keyval_rbtree_insert_value(ngx_rbtree_node_t *temp,
                                    ngx_rbtree_node_t *node,
                                    ngx_rbtree_node_t *sentinel)
{
  ngx_rbtree_node_t **p;
  ngx_http_keyval_node_t *n, *nt;

  for ( ;; ) {
    if (node->key < temp->key) {
      p = &temp->left;
    } else if (node->key > temp->key) {
      p = &temp->right;
    } else { /* node->key == temp->key */
      n = (ngx_http_keyval_node_t *) &node->color;
      nt = (ngx_http_keyval_node_t *) &temp->color;
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
ngx_http_keyval_rbtree_lookup(ngx_rbtree_t *rbtree,
                              ngx_str_t *key, uint32_t hash)
{
  ngx_int_t rc;
  ngx_rbtree_node_t *node, *sentinel;
  ngx_http_keyval_node_t *n;

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
    n = (ngx_http_keyval_node_t *) &node->color;

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
ngx_http_keyval_init_zone(ngx_shm_zone_t *shm_zone, void *data)
{
  size_t len;
  ngx_http_keyval_ctx_t *ctx, *octx;

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

  ctx->sh = ngx_slab_alloc(ctx->shpool, sizeof(ngx_http_keyval_sh_t));
  if (ctx->sh == NULL) {
    return NGX_ERROR;
  }

  ctx->shpool->data = ctx->sh;

  ngx_rbtree_init(&ctx->sh->rbtree, &ctx->sh->sentinel,
                  ngx_http_keyval_rbtree_insert_value);

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

static char *
ngx_http_keyval_conf_set_zone(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
  ssize_t size;
  ngx_shm_zone_t *shm_zone;
  ngx_str_t name, *value;
  ngx_http_keyval_ctx_t *ctx;

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
  } else {
    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "\"%V\" invalid zone size \"%V\"",
                       &cmd->name, &value[1]);
    return NGX_CONF_ERROR;
  }

  if (name.len == 0) {
    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "\"%V\" must have \"zone\" parameter",
                       &cmd->name);
    return NGX_CONF_ERROR;
  }

  ctx = ngx_pcalloc(cf->pool, sizeof(ngx_http_keyval_ctx_t));
  if (ctx == NULL) {
    return "failed to allocate";
  }

  shm_zone = ngx_shared_memory_add(cf, &name, size, &ngx_http_keyval_module);
  if (shm_zone == NULL) {
    return "failed to allocate shared memory";
  }

  shm_zone->init = ngx_http_keyval_init_zone;
  shm_zone->data = ctx;

  return NGX_CONF_OK;
}

static char *
ngx_http_keyval_conf_set_variable(ngx_conf_t *cf,
                                  ngx_command_t *cmd, void *conf)
{
  ngx_uint_t flags;
  ngx_str_t *value;
  ngx_http_variable_t *v;
  ngx_http_keyval_conf_t *config;
  ngx_http_keyval_variable_t *var;

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

  config = ngx_http_conf_get_module_main_conf(cf, ngx_http_keyval_module);
  if (config->variables == NULL) {
    config->variables = ngx_array_create(cf->pool, 4, sizeof(*var));
    if (config->variables == NULL) {
      return "failed to allocate";
    }
  }

  var = ngx_array_push(config->variables);
  if (var == NULL) {
    return "failed to allocate iteam";
  }

  if (value[1].data[0] == '$') {
    value[1].data++;
    value[1].len--;
    var->key_index = ngx_http_get_variable_index(cf, &value[1]);
    ngx_str_null(&var->key_string);
  } else {
    var->key_index = NGX_CONF_UNSET;
    var->key_string = value[1];
  }

  var->variable = value[2];

  var->shm_zone = ngx_shared_memory_add(cf, &value[3], 0,
                                        &ngx_http_keyval_module);
  if (var->shm_zone == NULL) {
    return "failed to allocate shared memory";
  }

  /* add variable */
  flags = NGX_HTTP_VAR_CHANGEABLE | NGX_HTTP_VAR_NOCACHEABLE;
  v = ngx_http_add_variable(cf, &value[2], flags);
  if (v == NULL) {
    return "failed to add variable";
  }

  v->get_handler = ngx_http_keyval_variable_get_handler;
  v->set_handler = ngx_http_keyval_variable_set_handler;
  v->data = (uintptr_t) var;

  return NGX_CONF_OK;
}

static void *
ngx_http_keyval_create_main_conf(ngx_conf_t *cf)
{
  ngx_http_keyval_conf_t *conf;

  conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_keyval_conf_t));
  if (conf == NULL) {
    return NULL;
  }

  conf->variables = NULL;

  return conf;
}

static ngx_int_t
ngx_http_keyval_variable_get_key(ngx_http_request_t *r,
                                 ngx_http_keyval_variable_t *var,
                                 ngx_str_t *key)
{
  if (!key || !var) {
    return NGX_ERROR;
  }

  if (var->key_index != NGX_CONF_UNSET) {
    ngx_http_variable_value_t *v;
    v = ngx_http_get_indexed_variable(r, var->key_index);
    if (v == NULL || v->not_found) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
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
ngx_http_keyval_get_data(ngx_http_request_t *r, ngx_http_keyval_ctx_t *ctx,
                         ngx_str_t *key, ngx_str_t *val)
{
  uint32_t hash;
  ngx_rbtree_node_t *node;
  ngx_http_keyval_node_t *kv;

  if (!ctx || !key || !val) {
    return NGX_ERROR;
  }

  hash = ngx_crc32_short(key->data, key->len);

  ngx_shmtx_lock(&ctx->shpool->mutex);

  node = ngx_http_keyval_rbtree_lookup(&ctx->sh->rbtree, key, hash);

  ngx_shmtx_unlock(&ctx->shpool->mutex);

  if (node == NULL) {
    return NGX_DECLINED;
  }

  kv = (ngx_http_keyval_node_t *) &node->color;

  // key->len = kv->len;
  // key->data = kv->data;

  val->len = kv->size - kv->len;
  val->data = kv->data + kv->len;

  return NGX_OK;
}

static ngx_int_t
ngx_http_keyval_set_data(ngx_http_request_t *r, ngx_http_keyval_ctx_t *ctx,
                         ngx_str_t *key, ngx_str_t *val)
{
  uint32_t hash;
  size_t n;
  ngx_int_t rc;
  ngx_rbtree_node_t *node;

  if (!ctx || !key || !val) {
    return NGX_ERROR;
  }

  hash = ngx_crc32_short(key->data, key->len);

  ngx_shmtx_lock(&ctx->shpool->mutex);

  node = ngx_http_keyval_rbtree_lookup(&ctx->sh->rbtree, key, hash);
  if (node != NULL) {
    ngx_rbtree_delete(&ctx->sh->rbtree, node);
    ngx_slab_free_locked(ctx->shpool, node);
  }

  n = offsetof(ngx_rbtree_node_t, color)
    + offsetof(ngx_http_keyval_node_t, data)
    + key->len
    + val->len;

  node = ngx_slab_alloc_locked(ctx->shpool, n);
  if (node == NULL) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                  "keyval: failed to allocate slab");
    rc = NGX_ERROR;
  } else {
    ngx_http_keyval_node_t *kv;
    kv = (ngx_http_keyval_node_t *) &node->color;

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

static ngx_int_t
ngx_http_keyval_variable_init(ngx_http_request_t *r, uintptr_t data,
                              ngx_http_keyval_ctx_t **ctx, ngx_str_t *key)
{
  ngx_http_keyval_conf_t *cf;
  ngx_http_keyval_variable_t *var;

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

  var = (ngx_http_keyval_variable_t *) data;
  if (!var->shm_zone) {
    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                  "keyval: rejected due to not found shared memory zone");
    return NGX_ERROR;
  }

  *ctx = var->shm_zone->data;
  if (!(*ctx)) {
    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                  "keyval: rejected due to not found shared memory context");
    return NGX_ERROR;
  }

  if (ngx_http_keyval_variable_get_key(r, var, key) != NGX_OK) {
    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                  "keyval: rejected due to not found variable key");
    return NGX_ERROR;
  }

  return NGX_OK;
}

static void
ngx_http_keyval_variable_set_handler(ngx_http_request_t *r,
                                     ngx_http_variable_value_t *v,
                                     uintptr_t data)
{
  ngx_str_t key, val;
  ngx_http_keyval_ctx_t *ctx;

  if (ngx_http_keyval_variable_init(r, data, &ctx, &key) != NGX_OK) {
    return;
  }

  val.data = v->data;
  val.len = v->len;

  if (ngx_http_keyval_set_data(r, ctx, &key, &val) != NGX_OK) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                  "keyval: failed to set data: \"%V\":\"%V\"", &key, &val);
  }
}

static ngx_int_t
ngx_http_keyval_variable_get_handler(ngx_http_request_t *r,
                                     ngx_http_variable_value_t *v,
                                     uintptr_t data)
{
  ngx_str_t key, val;
  ngx_http_keyval_ctx_t *ctx;

  if (ngx_http_keyval_variable_init(r, data, &ctx, &key) != NGX_OK) {
    v->not_found = 1;
    return NGX_OK;
  }

  if (ngx_http_keyval_get_data(r, ctx, &key, &val) != NGX_OK) {
    v->not_found = 1;
    return NGX_OK;
  }

  v->data = val.data;
  v->len = val.len;
  v->valid = 1;
  v->no_cacheable = 0;
  v->not_found = 0;

  return NGX_OK;
}
