/*
 * Copyright (C) 2024, Soar Qin<soarchin@gmail.com>

 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#pragma once

#include <wchar.h>

typedef struct mod_t mod_t;

extern void mods_init();
extern void mods_uninit();
extern void mods_add(const char *name, const wchar_t *path);
extern const wchar_t *mods_file_search(const wchar_t *path);
extern const wchar_t *mods_file_search_prefixed(const wchar_t *path);
