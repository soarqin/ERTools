/*
 * Copyright (c) 2024 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "lss.h"

#include <wx/filefn.h>
#include <wx/datetime.h>
#include <pugixml.hpp>
#include <fmt/format.h>
#include <fmt/xchar.h>

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
    gameName_ = mapGameName(doc_.child("Run").child("GameName").text().get());
    if (gameName_.empty()) {
        return -2;
    }
    filename_ = filename;
    for (auto node: doc_.select_nodes("/Run/Segments/Segment")) {
        auto segment = node.node();
        auto &sn = segs_.emplace_back();
        sn.seg = segment;
        auto name = segment.child("Name").text().get();
        fmt::print("Segment: {}\n", name);
    }
    auto xpath = fmt::format("/Run/AutoSplitterSettings/MainViewModel/{}ViewModel/Splits/HierarchicalTimingTypeViewModel", gameName_);
    auto nodeName = fmt::format("{}SplitType", gameName_);
    for (auto node: doc_.select_nodes(xpath.c_str())) {
        auto timingType = node.node().child("TimingType").text().get();
        for (auto node2: node.node().select_nodes("Children/HierarchicalSplitTypeViewModel")) {
            auto splitType = node2.node().child(nodeName.c_str()).text().get();
            for (auto node3: node2.node().select_nodes("Children/HierarchicalSplitViewModel")) {
                auto split = node3.node().child("Split");
                auto name = split.text().get();
                auto &snode = splits_.emplace_back();
                snode.when = timingType;
                snode.type = splitType;
                snode.split = split;
                fmt::print("Split: {} ({}/{})\n", name, timingType, splitType);
            }
        }
    }
    loaded_ = true;
    return 0;
}

bool Lss::save() {
    makeBackup();
    return false;
}

void Lss::makeBackup() {
    auto timestr = wxDateTime::Now().Format("%y%m%d_%H%M%S");
    auto backupFilename = fmt::format(L"{}.{}.bak", filename_, timestr.wc_str());
    if (wxFileExists(backupFilename)) {
        int n = 0;
        do {
            backupFilename = fmt::format(L"{}.{}.{}.bak", filename_, timestr.wc_str(), ++n);
        } while (wxFileExists(backupFilename));
    }
    doc_.save_file(backupFilename.c_str(), "  ");
}

}
