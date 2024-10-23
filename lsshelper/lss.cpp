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

static Enums sEnums;

void SplitNode::buildDisplayName() {
    name = split.text().get();
    xsiType = split.attribute("xsi:type").as_string();
    const auto &e = sEnums(type, name);
    displayName = e.name.empty() ? name : e.disp;
    fullDisplayName = fmt::format("{}/{}/{}", when, type, displayName);
}

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
    sEnums.load(gameName_);
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
                auto &snode = splits_.emplace_back();
                snode.split = node3.node().child("Split");
                snode.when = timingType;
                snode.type = splitType;
                snode.buildDisplayName();
            }
        }
    }

    pugi::xml_document doch;
    if (!doch.load_file((filename_ + L".helper").c_str())) return 0;
/*
    auto unusedSplits = doch.child("UnusedSplits");
    if (!unusedSplits) return 0;
    auto gameName = unusedSplits.attribute("GameName").as_string();
    if (gameName != gameName_) return 0;
    for (auto split: unusedSplits.children("Split")) {
        auto &snode = splits_.emplace_back();
        snode.when = split.attribute("When").as_string();
        snode.type = split.attribute("Type").as_string();
        snode.xsiType = split.attribute("XsiType").as_string();
        snode.name = split.attribute("Name").as_string();
        snode.buildDisplayName();
    }
*/
    loaded_ = true;
    return 0;
}

bool Lss::save() {
    pugi::xml_document doch;
    auto splitRootNode = doc_.select_node(fmt::format("/Run/AutoSplitterSettings/MainViewModel/{}ViewModel/Splits", gameName_).c_str()).node();
    if (!splitRootNode) return true;

    auto assignments = doch.append_child("Assignments");
    assignments.append_attribute("GameName").set_value(gameName_.c_str());
    size_t sz = splits_.size();
    for (size_t i = 0; i < sz; i++) {
        auto &split = splits_[i];
        if (split.seg) {
            auto splitNode = splitRootNode.select_node(fmt::format("HierarchicalTimingTypeViewModel[TimingType='{}']/Children/HierarchicalSplitTypeViewModel[{}SplitType='{}']/Children/HierarchicalSplitViewModel[Split='{}']", split.when, gameName_, split.type, split.name).c_str()).node();
            if (splitNode) {
                auto snode = assignments.append_copy(split.split);
                size_t sz2 = segs_.size();
                for (size_t j = 0; j < sz2; j++) {
                    if (segs_[j].seg == split.seg) {
                        snode.append_attribute("SegIndex").set_value(j);
                        snode.append_attribute("SegName").set_value(segs_[j].seg.child("Name").text().get());
                        break;
                    }
                }
                snode.append_attribute("When").set_value(split.when.c_str());
                snode.append_attribute("Type").set_value(split.type.c_str());
            }
        }
    }
    auto dochFilename = filename_ + L".helper";
    doch.save_file(dochFilename.c_str(), "  ");
    // doc_.save_file(filename_.c_str(), "  ");
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
