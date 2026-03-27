/*
 * Copyright (c) Tatsuya Kamijo
 * Copyright (c) Bengo4.com, Inc.
 */

#include "ngx_keyval.h"

ngx_int_t
ngx_keyval_variable_get_key(ngx_pool_t *pool, ngx_connection_t *connection,
    ngx_keyval_variable_t *var, ngx_str_t *key,
    ngx_keyval_get_index_variable get_index_variable,
    void *data)
{
    if (!key || !var || !pool || !connection || !data) {
        return NGX_ERROR;
    }

    if (var->indexes->nelts != 0) {
        ngx_variable_value_t **v;
        ngx_int_t current_index = 0;
        ngx_uint_t size_string = 0;
        u_char *last_space_available, *p, *end;

        v = ngx_palloc(pool,
                       sizeof(ngx_variable_value_t *) * var->indexes->nelts);

        if (v == NULL) {
            ngx_log_error(NGX_LOG_ERR, connection->log, 0,
                          "keyval: failed to allocate memory "
                          "for variable values array");
            return NGX_ERROR;
        }

        ngx_int_t *indexes = var->indexes->elts;

        for (ngx_uint_t i = 0 ; i < var->indexes->nelts ; i++) {
            v[i] = get_index_variable(data, indexes[i]);

            if (v[i] == NULL || v[i]->not_found) {
                ngx_log_error(NGX_LOG_INFO, connection->log, 0,
                              "keyval: variable specified was not provided");
                return NGX_ERROR;
            }

            size_string += v[i]->len;
        }

        key->data = (u_char *) ngx_pnalloc(pool,
                                           size_string
                                           + (var->key_string.len -
                                              var->indexes->nelts)
                                           + 1);

        if (key->data == NULL) {
            ngx_log_error(NGX_LOG_ERR, connection->log, 0,
                          "keyval: error allocating memory for key string");
            return NGX_ERROR;
        }

        key->len = 0;

        last_space_available = key->data;
        p = var->key_string.data;
        end = p + var->key_string.len;

        for ( ; p < end; p++) {
            if (*p == '$') {
                last_space_available = ngx_cpystrn(last_space_available,
                                                   v[current_index]->data,
                                                   v[current_index]->len + 1);
                key->len += v[current_index++]->len;
            } else {
                *last_space_available = *p;
                last_space_available++;
                key->len++;
            }
        }

        *last_space_available = '\0';
    } else {
        *key = var->key_string;
    }

    return NGX_OK;
}
