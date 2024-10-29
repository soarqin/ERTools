/*
 * Copyright (C) 2024, Soar Qin<soarchin@gmail.com>

 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "gamehook.h"

#include "mod.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "detours.h"

#include <shlwapi.h>
#include <wchar.h>
#include <stdio.h>

typedef struct {
    void *unk;
    wchar_t *string;
    void *unk2;
    uint64_t length;
    uint64_t capacity;
} wstring_impl_t;

wchar_t *wstring_impl_str(wstring_impl_t *str) {
    if (sizeof(wchar_t) * str->capacity >= 15) {
        return str->string;
    }
    return (wchar_t *)&str->string;
}

typedef enum {
    READ = 0,
    WRITE = 1,
    WRITE_OVERWRITE = 2,
    READ_WRITE = 3,

    // Custom mode specific to From Software's implementation
    READ_EBL = 9,
} AKOpenMode;

typedef void *(__cdecl *map_archive_path_t)(wstring_impl_t *path, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5, uint64_t p6);
typedef HANDLE (WINAPI *CreateFileW_t)(LPCWSTR lpFileName,
                                       DWORD dwDesiredAccess,
                                       DWORD dwShareMode,
                                       LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                       DWORD dwCreationDisposition,
                                       DWORD dwFlagsAndAttributes,
                                       HANDLE hTemplateFile);
typedef void *(__cdecl *ak_file_location_resolver_open_t)(uint64_t p1, wchar_t *path, AKOpenMode openMode, uint64_t p4, uint64_t p5, uint64_t p6);


static map_archive_path_t old_map_archive_path = NULL;
static CreateFileW_t old_CreateFileW = NULL;
static ak_file_location_resolver_open_t old_ak_file_location_resolver_open = NULL;

void *__cdecl map_archive_path(wstring_impl_t *path, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5, uint64_t p6) {
    void *res = old_map_archive_path(path, p2, p3, p4, p5, p6);
    wchar_t *str;
    if (path == NULL) return res;
    str = wstring_impl_str(path);
    if (wcsncmp(str, L"data", 4) == 0 && wcsncmp(str + 5, L":/", 2) == 0) {
        const wchar_t *replace = mods_file_search(str + 6);
        if (replace == NULL) return res;
        memcpy(str, L"./////", 6 * sizeof(wchar_t));
#if !defined(NDEBUG)
        fprintf(stdout, "map filename: %ls\n", str);
#endif
    }
    return res;
}

HANDLE WINAPI CreateFile_hooked(LPCWSTR lpFileName,
                                DWORD dwDesiredAccess,
                                DWORD dwShareMode,
                                LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                DWORD dwCreationDisposition,
                                DWORD dwFlagsAndAttributes,
                                HANDLE hTemplateFile) {
    const wchar_t *replace;
    replace = mods_file_search_prefixed(lpFileName);
#if !defined(NDEBUG)
    if (replace) {
        fprintf(stdout, "REPLACE: %ls\n", replace);
    } else {
        fprintf(stdout, "ORIGIN: %ls\n", lpFileName);
    }
#endif
    return old_CreateFileW(replace == NULL ? lpFileName : replace, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

const wchar_t *prefixes[3] = {
    L"sd/",
    L"sd/enus/",
    L"sd/ja/",
};

void *__cdecl ak_file_location_resolver_open(uint64_t p1, wchar_t *path, AKOpenMode openMode, uint64_t p4, uint64_t p5, uint64_t p6) {
    const wchar_t *replace, *ext;
    if (wcsncmp(path, L"sd:/", 4) != 0)
        return old_ak_file_location_resolver_open(p1, path, openMode, p4, p5, p6);
    replace = path + 4;
    ext = PathFindExtensionW(replace);
    if (ext != NULL && wcsicmp(ext, L".wem") == 0) {
        wchar_t new_path[MAX_PATH];
        _snwprintf(new_path, MAX_PATH, L"wem/%c%c/%s", replace[0], replace[1], replace);
        const wchar_t *new_replace = mods_file_search(new_path);
        if (new_replace != NULL) {
            return old_ak_file_location_resolver_open(p1, (wchar_t *)new_replace, openMode, p4, p5, p6);
        }
    }
    for (int i = 0; i < 3; i++) {
        wchar_t new_path[MAX_PATH];
        _snwprintf(new_path, MAX_PATH, L"%s%s", prefixes[i], replace);
        const wchar_t *new_replace = mods_file_search(new_path);
        if (new_replace != NULL) {
            return old_ak_file_location_resolver_open(p1, (wchar_t *)new_replace, openMode, p4, p5, p6);
        }
    }
    return old_ak_file_location_resolver_open(p1, path, openMode, p4, p5, p6);
}

static void *get_module_image_base(size_t *size) {
    HMODULE hModule = GetModuleHandleW(NULL);
    if (hModule == NULL) {
        return NULL;
    }
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        return NULL;
    }
    PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS64)((DWORD_PTR)dosHeader + dosHeader->e_lfanew);
    if (ntHeader->Signature != IMAGE_NT_SIGNATURE) {
        return NULL;
    }
    *size = ntHeader->OptionalHeader.SizeOfImage;
    return hModule;
}

static uint8_t *sig_scan(void *base, size_t size, const uint8_t *pattern, size_t pattern_size) {
    size_t end = size - pattern_size;
    uint8_t *u8_base = (uint8_t *)base;
    for (size_t i = 0; i < end; i++) {
        if (memcmp(pattern, u8_base + i, pattern_size) != 0) continue;
        return u8_base + i;
    }
    return NULL;
}

bool gamehook_install() {
    void *imageBase;
    size_t imageSize;
    uint8_t *addr;
    static const uint8_t map_archive_aob[] = {0x48, 0x83, 0x7b, 0x20, 0x08, 0x48, 0x8d, 0x4b, 0x08, 0x72, 0x03, 0x48, 0x8b, 0x09, 0x4c, 0x8b, 0x4b, 0x18, 0x41, 0xb8, 0x05, 0x00,
                                              0x00, 0x00, 0x4d, 0x3b, 0xc8};
    static const uint8_t ak_file_location_resolver_aob[] = {0x4c, 0x89, 0x74, 0x24, 0x28, 0x48, 0x8b, 0x84, 0x24, 0x90, 0x00, 0x00, 0x00, 0x48, 0x89, 0x44, 0x24, 0x20, 0x4c, 0x8b,
                                                            0xce, 0x45, 0x8b, 0xc4, 0x49, 0x8b, 0xd7, 0x48, 0x8b, 0xcd, 0xe8};

    DetourTransactionBegin();
    imageBase = get_module_image_base(&imageSize);
    addr = sig_scan(imageBase, imageSize, map_archive_aob, sizeof(map_archive_aob));
    if (!addr) {
        return false;
    }
    addr += *(int32_t *)(addr - 4);
    while (*(uint8_t *)addr == 0xE9) {
        addr += *(int32_t *)(addr + 1) + 5;
    }
    old_map_archive_path = (map_archive_path_t)addr;
    DetourAttach((PVOID *)&old_map_archive_path, map_archive_path);
    old_CreateFileW = CreateFileW;
    DetourAttach((PVOID *)&old_CreateFileW, CreateFile_hooked);
    //MH_CreateHook(addr, (void *)&map_archive_path, (void **)&old_map_archive_path);
    //MH_CreateHook(CreateFileW, (void *)&CreateFile_hooked, (void **)&old_CreateFileW);

    addr = sig_scan(imageBase, imageSize, ak_file_location_resolver_aob, sizeof(ak_file_location_resolver_aob));
    if (!addr) {
        return false;
    }
    addr += *(int32_t *)(addr + 31) + 35;
    while (*(uint8_t *)addr == 0xE9) {
        addr += *(int32_t *)(addr + 1) + 5;
    }
    old_ak_file_location_resolver_open = (ak_file_location_resolver_open_t)addr;
    DetourAttach((PVOID *)&old_ak_file_location_resolver_open, ak_file_location_resolver_open);
    DetourTransactionCommit();
    //MH_CreateHook(addr, (void *)&ak_file_location_resolver_open, (void **)&old_ak_file_location_resolver_open);
    //MH_EnableHook(MH_ALL_HOOKS);
    return true;
}

void gamehook_uninstall() {
    //MH_DisableHook(MH_ALL_HOOKS);
    //MH_Uninitialize();
    DetourTransactionBegin();
    DetourDetach((PVOID *)&old_map_archive_path, map_archive_path);
    DetourDetach((PVOID *)&old_CreateFileW, CreateFile_hooked);
    DetourDetach((PVOID *)&old_ak_file_location_resolver_open, ak_file_location_resolver_open);
    DetourTransactionCommit();
}
