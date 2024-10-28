/*
 * Copyright (c) 2024 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#pragma once

#include <string>
#include <vector>

namespace lss_helper {

inline std::vector<std::string> splitString(const std::string &str, char sep) {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = str.find(sep);
    while (end != std::string::npos) {
        result.emplace_back(str.substr(start, end - start));
        start = end + 1;
        end = str.find(sep, start);
    }
    result.emplace_back(str.substr(start, end));
    return result;
}

}