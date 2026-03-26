/*
 * Copyright (c) Tatsuya Kamijo
 * Copyright (c) Bengo4.com, Inc.
 */

#include "ngx_keyval.h"

char *
ngx_keyval_conf_set_variable(ngx_conf_t *cf, ngx_command_t *cmd, void *conf,
    ngx_keyval_conf_t *config, void *tag,
    ngx_keyval_variable_t **var,
    ngx_keyval_get_variable_index get_variable_index)
{
    ngx_str_t *value;
    int final_pos = 0;
    int num_vars = 0;
    size_t size_buffer_variable_name = 0, size_buffer_intermediate_string = 0;
    u_char *string = NULL, *variable_name = NULL;

    if (!config || !tag) {
        return "missing required parameter";
    }

    value = cf->args->elts;

    size_buffer_variable_name = value[1].len;
    size_buffer_intermediate_string = value[1].len;

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

    if (config->variables == NULL) {
        config->variables = ngx_array_create(cf->pool, 4,
                                             sizeof(ngx_keyval_variable_t));
        if (config->variables == NULL) {
            return "failed to allocate";
        }
    }

    *var = ngx_array_push(config->variables);
    if (*var == NULL) {
        return "failed to allocate item";
    }

    (*var)->indexes = ngx_array_create(cf->pool, 4, sizeof(ngx_int_t));
    if ((*var)->indexes == NULL) {
        return "failed to allocate";
    }

    if (ngx_strlchr(value[1].data, value[1].data + value[1].len, '$')
        == NULL)
    {
        (*var)->key_string = value[1];
    } else {
        string = value[1].data;

        (*var)->key_string.len = 0;
        (*var)->key_string.data = ngx_pnalloc(cf->pool,
                                              size_buffer_intermediate_string);
        if ((*var)->key_string.data == NULL) {
            return "failed to allocate memory for intermediate string";
        }

        variable_name = ngx_pnalloc(cf->pool, size_buffer_variable_name);
        if (variable_name == NULL) {
            return "failed to allocate memory for variable name buffer";
        }

        while (*string != '\0') {
            if (*string == '$') {
                int variable_name_str_index = 0;
                ngx_int_t *index;
                ngx_str_t str;

                (*var)->key_string.data[final_pos++] = '$';
                (*var)->key_string.len++;
                string++;

                while (*string != '\0'
                       && ((*string >= 'A' && *string <= 'Z')
                           || (*string >= 'a' && *string <= 'z')
                           || (*string >= '0' && *string <= '9')
                           || *string == '_'))
                {
                    variable_name[variable_name_str_index] = *string;
                    variable_name_str_index++;
                    string++;
                }

                variable_name[variable_name_str_index] = '\0';

                str.data = variable_name;
                str.len = ngx_strlen(variable_name);

                index = ngx_array_push((*var)->indexes);
                if (index == NULL) {
                    return "failed to allocate item";
                }
                *index = get_variable_index(cf, &str);
                if (*index == NGX_ERROR) {
                    return "failed to get variable index";
                }

                num_vars++;
            } else {
                (*var)->key_string.len++;
                (*var)->key_string.data[final_pos++] = *string;
                string++;
            }
        }

        (*var)->key_string.data[final_pos] = '\0';
    }

    (*var)->variable = value[2];

    (*var)->zone = ngx_keyval_conf_zone_get(cf, cmd, config, &value[3]);
    if ((*var)->zone == NULL) {
        return "zone not found";
    }

    if ((*var)->zone->type == NGX_KEYVAL_ZONE_SHM) {
        (*var)->zone->shm = ngx_shared_memory_add(cf, &value[3], 0, tag);
        if ((*var)->zone->shm == NULL) {
            return "failed to allocate shared memory";
        }
    } else if ((*var)->zone->type != NGX_KEYVAL_ZONE_REDIS) {
        return "invalid zone type";
    }

    return NGX_CONF_OK;
}

void *
ngx_keyval_create_main_conf(ngx_conf_t *cf)
{
    ngx_keyval_conf_t *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_keyval_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->variables = NULL;
    conf->zones = NULL;

    return conf;
}
