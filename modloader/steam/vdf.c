/*
 * Copyright (C) 2024, Soar Qin<soarchin@gmail.com>

 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "vdf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct vdf_node_t {
    char *key;
    bool is_leaf;
    union {
        char *value;
        struct {
            struct vdf_node_t **children;
            int count;
            int capacity;
        };
    };
} vdf_node_t;

void skip_spaces(char **buffer) {
    char *pos = *buffer;
    while (*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r' || *pos == '\f' || *pos == '\v') {
        pos++;
    }
    *buffer = pos;
}

vdf_node_t *vdf_open(const wchar_t *path) {
    FILE *f = _wfopen(path, L"rb");
    vdf_node_t *vdf;
    char *buffer;
    size_t size;
    const char *key;
    int key_len;
    if (!f) {
        return NULL;
    }
    vdf = (vdf_node_t *)malloc(sizeof(vdf_node_t));
    if (!vdf) {
        fclose(f);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = (char *)malloc(size + 1);
    if (!buffer) {
        fclose(f);
        free(vdf);
        return NULL;
    }
    fread(buffer, 1, size, f);
    buffer[size] = '\0';

    /* TODO: load vdf content */

    free(buffer);
    return vdf;
}

void vdf_free(vdf_node_t *vdf) {
    if (vdf->key) {
        free(vdf->key);
        vdf->key = NULL;
    }
    if (vdf->is_leaf) {
        if (vdf->value) {
            free(vdf->value);
            vdf->value = NULL;
        }
    } else {
        for (size_t i = 0; i < vdf->count; i++)
            vdf_free(vdf->children[i]);
        free(vdf->children);
        vdf->children = NULL;
        vdf->count = 0;
        vdf->capacity = 0;
    }
    free(vdf);
}

vdf_node_t *vdf_get_child(vdf_node_t *vdf, const char *key) {
    if (vdf->is_leaf) {
        return NULL;
    }
    for (size_t i = 0; i < vdf->count; i++) {
        if (strcmp(vdf->children[i]->key, key) == 0) {
            return vdf->children[i];
        }
    }
    return NULL;
}

void vdf_iterate(vdf_node_t *vdf,
                 void (*child_callback)(const char *key, vdf_node_t **node, int count, void *data),
                 void (*value_callback)(const char *key, const char *value, void *data),
                 void *data) {
    for (size_t i = 0; i < vdf->count; i++) {
        vdf_node_t *child = vdf->children[i];
        if (child->is_leaf) {
            if (value_callback) value_callback(child->key, child->value, data);
        } else {
            if (child_callback) child_callback(child->key, child->children, child->count, data);
        }
    }
}

bool vdf_is_leaf(vdf_node_t *vdf) {
    return vdf->is_leaf;
}

const char *vdf_get_key(vdf_node_t *vdf) {
    return vdf->key;
}

const char *vdf_get_value(vdf_node_t *vdf) {
    return vdf->value;
}

int vdf_get_child_count(vdf_node_t *vdf) {
    return vdf->count;
}

vdf_node_t *vdf_get_child_by_index(vdf_node_t *vdf, int index) {
    if (index < 0 || index >= vdf->count) {
        return NULL;
    }
    return vdf->children[index];
}
