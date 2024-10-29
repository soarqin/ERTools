/*
 * Copyright (C) 2024, Soar Qin<soarchin@gmail.com>

 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#pragma once

#include <wchar.h>

extern void config_load(void *module);

extern const wchar_t *module_path();
extern void config_full_path(wchar_t *path, const wchar_t *filename);
