/*
 * Copyright (c) 2024 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "enums.h"

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <fstream>

namespace lss_helper {

void from_json(const nlohmann::json &j, EnumData &data) {
    j.at("name").get_to(data.name);
    j.at("display_name").get_to(data.disp);
    j.at("description").get_to(data.desc);
}

void from_json(const nlohmann::json &j, std::unordered_map<uint32_t, std::vector<EnumData>> &m) {
    for (const auto &item: j.items()) {
        auto &theEnum = m[std::strtoul(item.key().c_str(), nullptr, 0)];
        if (item.value().is_array()) {
            item.value().get_to(theEnum);
        } else {
            theEnum.resize(1);
            item.value().get_to(theEnum[0]);
        }
    }
}

bool Enums::load(const std::string &gameName) {
    enums_.clear();
    nlohmann::json j;
    std::ifstream ifs("enums.json");
    if (!ifs.is_open()) return false;
    ifs >> j;
    ifs.close();

    try {
        j[gameName].get_to(enums_);
    } catch (const std::exception &e) {
        fmt::print("{}", e.what());
        return false;
    }

    for (const auto &item: enums_)
        for (const auto &item2: item.second)
            for (const auto &item3: item2.second)
                mapper_.emplace(fmt::format("{}/{}", item.first, item3.name), item3);
    return true;
}

static const EnumData dummyEmpty;

const EnumData &Enums::operator()(const std::string &stype, uint32_t flagId) const {
    auto it = enums_.find(stype);
    if (it != enums_.end()) {
        auto it2 = it->second.find(flagId);
        if (it2 != it->second.end()) {
            return it2->second[0];
        }
    }
    return dummyEmpty;
}

const EnumData &Enums::operator()(const std::string &stype, const std::string &name) const {
    auto it = mapper_.find(fmt::format("{}/{}", stype, name));
    if (it != mapper_.end()) {
        return it->second;
    }
    return dummyEmpty;
}

}
