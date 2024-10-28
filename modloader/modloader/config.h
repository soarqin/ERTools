/*
 * Copyright (C) 2024, Soar Qin<soarchin@gmail.com>

 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#pragma once

#include <string>

namespace er::modloader {

class Config {
public:
    void load(void *module);

    [[nodiscard]] const std::wstring &modulePath() const {
        return modulePath_;
    }
    [[nodiscard]] std::wstring fullPath(const std::wstring &filename) const;

private:
    std::wstring modulePath_;
};

extern Config gConfig;

}
