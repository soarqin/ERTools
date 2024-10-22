/*
 * Copyright (c) 2024 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#pragma once

#include <pugixml.hpp>
#include <vector>
#include <tuple>
#include <cwchar>

namespace lss_helper {

struct SegNode {
    pugi::xml_node seg;
    pugi::xml_node split;

    inline void assign(pugi::xml_node sp) const {
        const_cast<SegNode*>(this)->split = sp;
    }
};

struct SplitType {
    std::string type;
    std::vector<pugi::xml_node> nodes;
};

struct SplitTiming {
    std::string type;
    std::vector<SplitType> splitTypes;
};

class Lss {
public:
    int open(const wchar_t *filename);
    [[nodiscard]] inline bool loaded() const { return loaded_; }
    [[nodiscard]] inline const std::vector<SegNode> &segs() const { return segs_; }
    [[nodiscard]] inline const std::vector<SplitTiming> &splits() const { return splits_; }

private:
    pugi::xml_document doc_;

    bool loaded_ = false;
    std::vector<SegNode> segs_;
    std::vector<SplitTiming> splits_;
};

}
