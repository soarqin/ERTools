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
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace lss_helper {

Enums gEnums;

std::wstring utf8towc(const std::string &utf8str) {
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8str.c_str(), -1, nullptr, 0);
    if (len == 0) return {};
    std::wstring wstr(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8str.c_str(), -1, &wstr[0], len);
    return std::move(wstr);
}

void from_json(const nlohmann::json &j, EnumData &data) {
    j.at("name").get_to(data.name);
    j.at("display_name").get_to(data.disp);
    j.at("description").get_to(data.desc);
    data.wdisp = utf8towc(data.disp);
}

void from_json(const nlohmann::json &j, std::vector<EnumData> &m) {
    for (const auto &item: j.items()) {
        if (item.value().is_array()) {
            item.value().get_to(m);
            continue;
        }
        auto &theEnum = m.emplace_back();
        item.value().get_to(theEnum);
        theEnum.flagId = std::strtoul(item.key().c_str(), nullptr, 0);
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
            mapper_.emplace(fmt::format("{}/{}", item.first, item2.name), item2);
    return true;
}

const EnumData &Enums::operator()(const std::string &stype, const std::string &name) const {
    auto it = mapper_.find(fmt::format("{}/{}", stype, name));
    if (it != mapper_.end()) {
        return it->second;
    }
    static const EnumData dummyEmpty;
    return dummyEmpty;
}

const std::vector<EnumData> &Enums::operator[](const std::string &stype) const {
    auto it = enums_.find(stype);
    if (it != enums_.end()) {
        return it->second;
    }
    static const std::vector<EnumData> dummy;
    return dummy;
}

}
