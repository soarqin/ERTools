#include "flags.h"

#include "eldenring.h"

#include <nlohmann/json.hpp>
#include <fstream>

namespace flagchecker::game {

Flags gFlags;

bool Flags::loadFromFile(const std::string &filename) {
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(std::ifstream {filename});
        for (auto &item: j["flags"].items()) {
            std::vector<FlagDef> flags;
            for (auto &sub: item.value().items()) {
                flags.emplace_back(FlagDef {sub.key(), sub.value().get<uint32_t>(), false});
            }
            flags_.emplace_back(FlagCat {item.key(), std::move(flags)});
        }
    } catch (...) {
        return false;
    }
    return true;
}

void Flags::update() {
    if (!gEldenRing.resolved())
        return;
    for (auto &cat: flags_) {
        for (auto &flag: cat.flags) {
            flag.enabled = gEldenRing.readFlag(flag.id);
        }
    }
}

}
