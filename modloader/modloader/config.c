/*
 * Copyright (C) 2024, Soar Qin<soarchin@gmail.com>

 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "config.h"

#include "mod.h"

#include <ini.h>

#include <shlwapi.h>
#include <string.h>

static wchar_t modulePath_[MAX_PATH];

static void try_load_dll(const char *name, const wchar_t *path) {
    HMODULE dll;
    if (wcschr(path, L':') == NULL && path[0] != L'\\' && path[0] != L'/') {
        wchar_t full_path[MAX_PATH];
        config_full_path(full_path, path);
        dll = LoadLibraryW(full_path);
    } else {
        dll = LoadLibraryW(path);
    }
    if (dll == NULL) {
        fwprintf(stderr, L"Cannot load dll %hs (from `%ls`)\n", name, path);
    } else {
        fwprintf(stderr, L"Loaded dll %hs (from `%ls`)\n", name, path);
    }
}

static int ini_read_cb(void *user, const char *section,
                       const char *name, const char *value) {
    wchar_t path[MAX_PATH];
    if (strcmp(section, "dlls") == 0) {
        MultiByteToWideChar(CP_UTF8, 0, value, -1, path, MAX_PATH);
        try_load_dll(name, path);
    }
    if (strcmp(section, "mods") == 0) {
        MultiByteToWideChar(CP_UTF8, 0, value, -1, path, MAX_PATH);
        mods_add(name, path);
    }
    return 1;
}

void config_load(void *module) {
    wchar_t config_path[MAX_PATH] = L"";
    FILE *config_file = NULL;
    GetEnvironmentVariableW(L"MODLOADER_CONFIG", config_path, MAX_PATH);
    GetModuleFileNameW((HMODULE)module, modulePath_, MAX_PATH);
    PathRemoveFileSpecW(modulePath_);
    PathRemoveBackslashW(modulePath_);
    if (config_path[0] == L'\0') {
        config_full_path(config_path, L"YAERModLoader.ini");
    } else {
        if (wcschr(config_path, L':') == NULL && config_path[0] != L'\\' && config_path[0] != L'/') {
            wchar_t temp[MAX_PATH];
            wcscpy(temp, config_path);
            config_full_path(config_path, temp);
        }
    }
    config_file = _wfopen(config_path, L"r");
    if (config_file == NULL) return;
    ini_parse_file(config_file, ini_read_cb, NULL);
    fclose(config_file);
}

const wchar_t *module_path() { return modulePath_; }

void config_full_path(wchar_t *path, const wchar_t *filename) {
    wcscpy(path, modulePath_);
    PathAppendW(path, filename);
}
