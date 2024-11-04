/*
 * LSS Helper - LiveSplit lss file helper
 * Copyright (C) 2024  Soar Qin<soarchin@gmail.com>

 * This file is part of LSS Helper.

 * LSS Helper is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.

 * LSS Helper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with LSS Helper.  If not, see <http://www.gnu.org/licenses/>.
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