/*
 * Copyright (C) 2024, Soar Qin<soarchin@gmail.com>

 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#pragma once

#include <cstdint>

namespace er::modloader {

class GameHook {
public:
    GameHook() = delete;
    ~GameHook() = delete;
    static bool install();
    static void uninstall();

};

}
