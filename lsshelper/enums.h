/*
 * Copyright (c) 2024 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>

namespace lss_helper {

struct EnumData {
    std::string name;
    std::string disp;
    std::string desc;
};

class Enums {
public:
    bool load(const std::string &gameName);
    [[nodiscard]] const EnumData &operator()(const std::string &stype, uint32_t flagId) const;
    [[nodiscard]] const EnumData &operator()(const std::string &stype, const std::string &name) const;

private:
    std::unordered_map<std::string, std::unordered_map<uint32_t, std::vector<EnumData>>> enums_;
    std::unordered_map<std::string, const EnumData&> mapper_;
};

}