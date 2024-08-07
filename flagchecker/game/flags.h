#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace flagchecker::game {

class Flags {
    struct FlagDef {
        std::string name;
        uint32_t id = 0;
        bool enabled = false;
    };
    struct FlagCat {
        std::string name;
        std::vector<FlagDef> flags;
    };

public:
    bool loadFromFile(const std::string &filename);
    void update();
    [[nodiscard]] const std::vector<FlagCat> &flags() const {
        return flags_;
    }

private:
    std::vector<FlagCat> flags_;
};

extern Flags gFlags;

}
