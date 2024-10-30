/*
 * Copyright (C) 2024, Soar Qin<soarchin@gmail.com>

 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#pragma once

#include <wchar.h>
#include <stdint.h>
#include <stdbool.h>

extern bool app_find_game_path(uint32_t app_id, wchar_t *path);
