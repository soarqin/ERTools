/*
 * Copyright (C) 2024, Soar Qin<soarchin@gmail.com>

 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "mod.h"

#include "config.h"
#include "filecache.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

static wchar_t game_folder[MAX_PATH];
static uint32_t game_folder_length;
wchar_t game_folder_pre[MAX_PATH];

typedef struct mod_t {
    char *name;
    wchar_t *base_path;
} mod_t;

mod_t *mods = NULL;
int mod_count = 0;
int mod_capacity = 0;

void mods_init() {
    GetCurrentDirectoryW(MAX_PATH, game_folder_pre);
    PathCanonicalizeW(game_folder, game_folder_pre);
    PathRemoveBackslashW(game_folder);
    game_folder_length = wcslen(game_folder);

    filecache_init();
    mods = NULL;
    mods_add("test", L"mod");
}

void mods_uninit() {
    for (int i = 0; i < mod_count; i++) {
        free(mods[i].name);
        free(mods[i].base_path);
    }
    free(mods);
    mods = NULL;
    mod_count = 0;
    mod_capacity = 0;
    filecache_uninit();
}

void mods_add(const char *name, const wchar_t *path) {
    mod_t *mod;
    if (mod_count >= mod_capacity) {
        mod_t *new_mods;
        mod_capacity = mod_capacity == 0 ? 8 : mod_capacity * 2;
        new_mods = (mod_t *)realloc(mods, mod_capacity * sizeof(mod_t));
        if (new_mods == NULL) {
            return;
        }
        mods = new_mods;
    }
    mod = &mods[mod_count++];
    mod->name = strdup(name);
    mod->base_path = wcsdup(path);
}

const wchar_t *mods_file_search(const wchar_t *path) {
    wchar_t cpath[MAX_PATH];
    const wchar_t *res;
    if (path[0] == '\\' || path[0] == '/')
        wcscpy(cpath, path);
    else
        _snwprintf(cpath, MAX_PATH, L"\\%s", path);
    for (int n = (int)wcslen(cpath) - 1; n >= 0; n--) {
        if (cpath[n] == L'/') cpath[n] = L'\\';
    }
    res = filecache_find(cpath);
    if (res != NULL) {
        return res[0] == 0 ? NULL : res;
    }
    for (int i = 0; i < mod_count; i++) {
        wchar_t full_path[MAX_PATH];
        _snwprintf(full_path, MAX_PATH, L"%s%s", mods[i].base_path, cpath);
        if (PathFileExistsW(full_path)) {
            return filecache_add(path, full_path);
        }
    }
    filecache_add(path, L"");
    return NULL;
}

const wchar_t *mods_file_search_prefixed(const wchar_t *path) {
    const wchar_t *cpath;
    const wchar_t *res;
    if (wcsnicmp(path, game_folder, game_folder_length) != 0) return NULL;
    cpath = path + game_folder_length;
    res = filecache_find(cpath);
    if (res != NULL) {
        return res[0] == 0 ? NULL : res;
    }
    for (int i = 0; i < mod_count; i++) {
        wchar_t full_path[MAX_PATH];
        _snwprintf(full_path, MAX_PATH, L"%s%s", mods[i].base_path, cpath);
        if (PathFileExistsW(full_path)) {
            return filecache_add(path, full_path);
        }
    }
    filecache_add(path, L"");
    return NULL;
}
