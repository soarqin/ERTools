/*
 * Copyright (C) 2024, Soar Qin<soarchin@gmail.com>

 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "config.h"

#include <shlwapi.h>

namespace er::modloader {

Config gConfig;

void Config::load(void *module) {
    wchar_t path[1024];
    GetModuleFileNameW(HMODULE(module), path, 1024);
    PathRemoveFileSpecW(path);
    modulePath_ = path;
}

std::wstring Config::fullPath(const std::wstring &filename) const {
    wchar_t path[1024];
    lstrcpyW(path, modulePath_.c_str());
    PathAppendW(path, filename.c_str());
    return path;
}

}
