/*
 * Copyright (C) 2024, Soar Qin<soarchin@gmail.com>

 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#pragma once

#include <wchar.h>

extern void filecache_init();
extern void filecache_uninit();
extern const wchar_t *filecache_add(const wchar_t *path, const wchar_t *replace);
extern const wchar_t *filecache_find(const wchar_t *path);
