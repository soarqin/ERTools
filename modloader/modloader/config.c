/*
 * Copyright (C) 2024, Soar Qin<soarchin@gmail.com>

 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "config.h"

#include <shlwapi.h>

static wchar_t modulePath_[1024];

void config_load(void *module) {
    GetModuleFileNameW((HMODULE)module, modulePath_, 1024);
    PathRemoveFileSpecW(modulePath_);
    PathRemoveBackslashW(modulePath_);
}

const wchar_t *module_path() { return modulePath_; }

void config_full_path(wchar_t * path, const wchar_t *filename) {
    lstrcpyW(path, modulePath_);
    PathAppendW(path, filename);
}
