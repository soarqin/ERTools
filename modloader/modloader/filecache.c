/*
 * Copyright (C) 2024, Soar Qin<soarchin@gmail.com>

 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "filecache.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <uthash.h>

typedef struct {
    wchar_t *base_path;
    wchar_t *native_path;

    UT_hash_handle hh;
} file_t;

static file_t *files = NULL;

file_t *file_new(const wchar_t *base_path, const wchar_t *native_path) {
    file_t *file = (file_t *)malloc(sizeof(file_t));
    if (file == NULL) {
        return NULL;
    }
    file->base_path = wcsdup(base_path);
    file->native_path = native_path && native_path[0] ? wcsdup(native_path) : L"";
    return file;
}

void file_free(file_t *file) {
    if (file == NULL) {
        return;
    }
    free(file->base_path);
    free(file->native_path);
    free(file);
}

void filecache_init() {
    files = NULL;
}

void filecache_uninit() {
    file_t *file, *tmp;
    HASH_ITER(hh, files, file, tmp) {
        HASH_DEL(files, file);
        file_free(file);
    }
}

const wchar_t *filecache_add(const wchar_t *path, const wchar_t *replace) {
    file_t *file = file_new(path, replace);
    if (file == NULL) {
        return NULL;
    }
    HASH_ADD_KEYPTR(hh, files, file->base_path, wcslen(file->base_path), file);
    return file->native_path;
}

const wchar_t *filecache_find(const wchar_t *path) {
    file_t *file;
    HASH_FIND(hh, files, path, wcslen(path), file);
    return file == NULL ? NULL : file->native_path;
}
