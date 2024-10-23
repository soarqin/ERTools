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
                auto split = node3.node().child("Split");
                auto &snode = splits_.emplace_back();
                snode.when = timingType;
                snode.type = splitType;
                snode.xsiType = split.attribute("xsi:type").as_string();
                snode.name = split.text().get();
                snode.buildDisplayName();
            }
        }
    }

    pugi::xml_document doch;
    if (!doch.load_file((filename_ + L".helper").c_str())) return 0;
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
    loaded_ = true;
    return 0;
}

bool Lss::save() {
    pugi::xml_document doch;
    auto splitRootNode = doc_.select_node(fmt::format("/Run/AutoSplitterSettings/MainViewModel/{}ViewModel/Splits", gameName_).c_str()).node();
    if (!splitRootNode) return true;

    makeBackup();
    auto unusedSplits = doch.append_child("UnusedSplits");
    unusedSplits.append_attribute("GameName").set_value(gameName_.c_str());
    for (auto &split: splits_) {
        if (split.assigned != SplitNode::kFree) {
            auto timingTypeNode = splitRootNode.select_node(fmt::format("HierarchicalTimingTypeViewModel[TimingType='{}']", split.when).c_str()).node();
            if (!timingTypeNode) {
                auto tnode = splitRootNode.append_child("HierarchicalTimingTypeViewModel");
                tnode.append_child("TimingType").text().set(split.when.c_str());
                timingTypeNode = tnode;
            }
            auto splitTypeNode = timingTypeNode.select_node(fmt::format("Children/HierarchicalSplitTypeViewModel[{}SplitType='{}']", gameName_, split.type).c_str()).node();
            if (!splitTypeNode) {
                auto stnode = timingTypeNode.append_child("Children").append_child("HierarchicalSplitTypeViewModel");
                stnode.append_child(fmt::format("{}SplitType", gameName_).c_str()).text().set(split.type.c_str());
                splitTypeNode = stnode;
            }
            auto snode = splitTypeNode.select_node(fmt::format("Children/HierarchicalSplitViewModel[Split='{}']", split.name).c_str()).node();
            if (!snode) {
                snode = splitTypeNode.append_child("Children").append_child("HierarchicalSplitViewModel").append_child("Split");
                snode.append_attribute("xsi:type").set_value(split.xsiType.c_str());
                snode.text().set(split.name.c_str());
            }
        } else {
            auto snode = unusedSplits.append_child("Split");
            snode.append_attribute("When").set_value(split.when.c_str());
            snode.append_attribute("Type").set_value(split.type.c_str());
            snode.append_attribute("XsiType").set_value(split.xsiType.c_str());
            snode.append_attribute("Name").set_value(split.name.c_str());
            auto splitNode = splitRootNode.select_node(fmt::format("HierarchicalTimingTypeViewModel[TimingType='{}']/Children/HierarchicalSplitTypeViewModel[{}SplitType='{}']/Children/HierarchicalSplitViewModel[Split='{}']", split.when, gameName_, split.type, split.name).c_str()).node();
            if (splitNode) {
                auto parent = splitNode.parent();
                parent.remove_child(splitNode);
                if (parent.children().empty()) {
                    parent.parent().parent().remove_child(parent.parent());
                }
            }
        }
    }
    auto dochFilename = filename_ + L".helper";
    doch.save_file(dochFilename.c_str(), "  ");
    doc_.save_file(filename_.c_str(), "  ");
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
