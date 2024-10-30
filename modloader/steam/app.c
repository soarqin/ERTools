/*
 * Copyright (C) 2024, Soar Qin<soarchin@gmail.com>

 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "app.h"

#include "vdf.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

bool app_find_game_path(uint32_t app_id, wchar_t *path) {
    HKEY key;
    int i;
    wchar_t steam_path[MAX_PATH];
    wchar_t library_path[MAX_PATH];
    struct vdf_object *library_folders;
    DWORD valtype = REG_SZ;
    DWORD cbdata = MAX_PATH * sizeof(wchar_t);
    if (RegOpenKeyW(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", &key) != ERROR_SUCCESS) {
        return false;
    }
    if (RegQueryValueExW(key, L"SteamPath", NULL, &valtype, (LPBYTE)steam_path, &cbdata) != ERROR_SUCCESS) {
        RegCloseKey(key);
        return false;
    }
    RegCloseKey(key);
    _snwprintf(library_path, MAX_PATH, L"%ls\\steamapps\\libraryfolders.vdf", steam_path);
    library_folders = vdf_parse_file(library_path);
    if (library_folders == NULL) {
        return false;
    }
    if (strcmp(library_folders->key, "libraryfolders") != 0 || library_folders->type != VDF_TYPE_ARRAY) {
        vdf_free_object(library_folders);
        return false;
    }
    for (i = (int)vdf_object_get_array_length(library_folders) - 1; i >= 0; i--) {
        char app_id_str[16];
        struct vdf_object *sub = vdf_object_index_array(library_folders, i);
        struct vdf_object *sub2, *sub3, *acf;
        if (sub == NULL || sub->type != VDF_TYPE_ARRAY) continue;
        sub2 = vdf_object_index_array_str(sub, "path");
        if (sub2 == NULL || sub2->type != VDF_TYPE_STRING) continue;
        sub3 = vdf_object_index_array_str(sub, "apps");
        if (sub3 == NULL || sub3->type != VDF_TYPE_ARRAY) continue;
        _snprintf(app_id_str, 16, "%u", app_id);
        sub3 = vdf_object_index_array_str(sub3, app_id_str);
        if (sub3 == NULL || (sub3->type != VDF_TYPE_STRING && sub3->type != VDF_TYPE_INT)) continue;
        _snwprintf(library_path, MAX_PATH, L"%hs\\steamapps\\appmanifest_%u.acf", vdf_object_get_string(sub2), app_id);
        acf = vdf_parse_file(library_path);
        if (acf == NULL) continue;
        if (strcmp(acf->key, "AppState") != 0 || acf->type != VDF_TYPE_ARRAY) {
            vdf_free_object(acf);
            continue;
        }
        sub3 = vdf_object_index_array_str(acf, "installdir");
        if (sub3 == NULL || sub3->type != VDF_TYPE_STRING) {
            vdf_free_object(acf);
            continue;
        }
        _snwprintf(path, MAX_PATH, L"%hs\\steamapps\\common\\%hs", vdf_object_get_string(sub2), vdf_object_get_string(sub3));
        vdf_free_object(acf);
        break;
    }
    vdf_free_object(library_folders);
    return true;
}
