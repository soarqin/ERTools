/*
 * Copyright (c) 2024 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "lss.h"

#include <pugixml.hpp>
#include <fmt/format.h>

namespace lss_helper {

inline std::string mapGameName(const std::string &name) {
    if (name == "Elden Ring")
        return "EldenRing";
    return "";
}

int Lss::open(const wchar_t *filename) {
    loaded_ = false;
    auto result = doc_.load_file(filename);
    if (!result) {
        return -1;
    }
    auto gameName = mapGameName(doc_.child("Run").child("GameName").text().get());
    if (gameName.empty()) {
        return -2;
    }
    for (auto node: doc_.select_nodes("/Run/Segments/Segment")) {
        auto segment = node.node();
        auto &sn = segs_.emplace_back();
        sn.seg = segment;
        auto name = segment.child("Name").text().get();
        fmt::print("Segment: {}\n", name);
    }
    auto xpath = fmt::format("/Run/AutoSplitterSettings/MainViewModel/{}ViewModel/Splits/HierarchicalTimingTypeViewModel", gameName);
    auto nodeName = fmt::format("{}SplitType", gameName);
    for (auto node: doc_.select_nodes(xpath.c_str())) {
        auto timingType = node.node().child("TimingType").text().get();
        auto &st = splits_.emplace_back();
        st.type = timingType;
        for (auto node2: node.node().select_nodes("Children/HierarchicalSplitTypeViewModel")) {
            auto splitType = node2.node().child(nodeName.c_str()).text().get();
            auto &stype = st.splitTypes.emplace_back();
            stype.type = splitType;
            for (auto node3: node2.node().select_nodes("Children/HierarchicalSplitViewModel")) {
                auto split = node3.node().child("Split");
                auto name = split.text().get();
                stype.nodes.emplace_back(split);
                fmt::print("Split: {} ({}/{})\n", name, timingType, splitType);
            }
        }
    }
    loaded_ = true;
    return 0;
}

}
