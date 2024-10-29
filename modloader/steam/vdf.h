/*
 * Copyright (C) 2024, Soar Qin<soarchin@gmail.com>

 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#pragma once

#include <wchar.h>
#include <stdbool.h>

typedef struct vdf_node_t vdf_node_t;

extern vdf_node_t *vdf_open(const wchar_t *path);
extern void vdf_free(vdf_node_t *vdf);
extern vdf_node_t *vdf_get_child(vdf_node_t *vdf, const char *key);
extern void vdf_iterate(vdf_node_t *vdf,
                        void (*child_callback)(const char *key, vdf_node_t **node, int count, void *data),
                        void (*value_callback)(const char *key, const char *value, void *data),
                        void *data);
extern bool vdf_is_leaf(vdf_node_t *vdf);
extern const char *vdf_get_key(vdf_node_t *vdf);
extern const char *vdf_get_value(vdf_node_t *vdf);
extern int vdf_get_child_count(vdf_node_t *vdf);
extern vdf_node_t *vdf_get_child_by_index(vdf_node_t *vdf, int index);
