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
    pugi::xml_node split;
    std::wstring splitName;

    inline void assign(const pugi::xml_node &n, const std::wstring &name) const {
        const_cast<SegNode*>(this)->split = n;
        const_cast<SegNode*>(this)->splitName = name;
    }
};

struct SplitNode {
    std::string when;
    std::string type;
    std::string xsiType;
    std::string name;
    std::wstring displayName;
    std::wstring fullDisplayName;

    pugi::xml_node split;
    pugi::xml_node seg;

    inline void assign(const pugi::xml_node &n) const {
        const_cast<SplitNode*>(this)->seg = n;
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
    inline void setChanged() { changed_ = true; }
    const SegNode &insertNewSegment(const std::string &name, int index = -1);
    void moveSegmentDown(int index);
    void deleteSegment(int index);
    void deleteSplit(int index);
    const SplitNode *findOrAppendSplit(const std::string &when, const std::string &type, const std::string &name, bool &wasAppend);
    const SplitNode *find(const std::string &when, const std::string &type, const std::string &name);

private:
    pugi::xml_node ensureSegmentParent();
    pugi::xml_node ensureSplitTree();
    pugi::xml_node ensureSplitParent(const std::string &when, const std::string &type);
/*
    void makeBackup();
*/

private:
    pugi::xml_document doc_;
    pugi::xml_node splitTree_;

    std::wstring filename_;

    bool loaded_ = false;
    bool changed_ = false;
    std::string gameName_;
    std::vector<SegNode> segs_;
    std::vector<SplitNode> splits_;
};

}
