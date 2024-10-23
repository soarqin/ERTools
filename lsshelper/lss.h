/*
 * Copyright (c) 2024 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#pragma once

#include "enums.h"

#include <pugixml.hpp>
#include <vector>
#include <tuple>
#include <cwchar>

namespace lss_helper {

struct SegNode {
    pugi::xml_node seg;
    std::string split;

    inline void assign(const std::string &sp) const {
        const_cast<SegNode*>(this)->split = sp;
    }
};

struct SplitNode {
    enum {
        kFree = 0,
        kAssigned = 1,
        kReserved = 2,
    };
    std::string when;
    std::string type;
    std::string xsiType;
    std::string name;
    std::string displayName;
    std::string fullDisplayName;

    int assigned = kFree;

    inline void setAssigned(int a) const {
        const_cast<SplitNode*>(this)->assigned = a;
    }
    void buildDisplayName();
};

class Lss {
public:
    int open(const wchar_t *filename);
    bool save();
    [[nodiscard]] inline bool loaded() const { return loaded_; }
    [[nodiscard]] inline const std::string &gameName() const { return gameName_; }
    [[nodiscard]] inline const std::vector<SegNode> &segs() const { return segs_; }
    [[nodiscard]] inline const std::vector<SplitNode> &splits() const { return splits_; }

private:
    void makeBackup();

private:
    pugi::xml_document doc_;

    std::wstring filename_;

    bool loaded_ = false;
    std::string gameName_;
    std::vector<SegNode> segs_;
    std::vector<SplitNode> splits_;
};

}
