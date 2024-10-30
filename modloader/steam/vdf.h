/* Codes from: [libofdf](https://github.com/Jan200101/libofdf), licensed under MIT */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

enum vdf_data_type {
    VDF_TYPE_NONE,
    VDF_TYPE_ARRAY,
    VDF_TYPE_STRING,
    VDF_TYPE_INT
};

struct vdf_object;

union vdf_data {
    struct {
        struct vdf_object **data_value;
        size_t len;
    } data_array;

    struct {
        char *str;
        size_t len;
    } data_string;

    int64_t data_int;
};

struct vdf_object {
    char *key;
    struct vdf_object *parent;

    enum vdf_data_type type;
    union vdf_data data;

    char *conditional;
};

struct vdf_object *vdf_parse_buffer(const char *, size_t);
struct vdf_object *vdf_parse_file(const wchar_t *);

size_t vdf_object_get_array_length(const struct vdf_object *);
struct vdf_object *vdf_object_index_array(const struct vdf_object *, size_t);
struct vdf_object *vdf_object_index_array_str(const struct vdf_object *, const char *);

const char *vdf_object_get_string(const struct vdf_object *);
int64_t vdf_object_get_int(const struct vdf_object *);

void vdf_print_object(struct vdf_object *);
void vdf_free_object(struct vdf_object *);

#ifdef __cplusplus
}
#endif
