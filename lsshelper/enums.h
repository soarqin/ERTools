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

#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>

namespace lss_helper {

struct EnumData {
    uint32_t flagId = 0;
    std::string name;
    std::string disp;
    std::string desc;
    std::wstring wdisp;
};

class Enums {
public:
    static void setLanguagePrefix(const std::wstring &prefix);
    bool load(const std::string &gameName);
    [[nodiscard]] const EnumData &operator()(const std::string &stype, const std::string &name) const;
    [[nodiscard]] const std::vector<EnumData> &operator[](const std::string &stype) const;

private:
    std::unordered_map<std::string, std::vector<EnumData>> enums_;
    std::unordered_map<std::string, const EnumData&> mapper_;
};

extern Enums gEnums;

}