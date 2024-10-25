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

inline const char *typeToXsiType(const std::string &type) {
    if (type == "Grace")
        return "Grace";
    if (type == "Boss")
        return "Boss";
    if (type == "ItemPickup")
        return "ItemPickup";
    if (type == "Position")
        return "Position";
    if (type == "Flag")
        return "xsd:unsignedInt";
    return "";
}

inline std::wstring toWideString(const std::string &str) {
    return wxString::FromUTF8(str).ToStdWstring();
}

void SplitNode::buildDisplayName() {
    name = split.text().get();
    xsiType = split.attribute("xsi:type").as_string();
    const auto &e = gEnums(type, name);
    displayName = toWideString(e.name.empty() ? name : e.disp);
    fullDisplayName = toWideString(fmt::format("{}/{}/", when, type, e.name.empty() ? name : e.disp));
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
    gEnums.load(gameName_);
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
    if (changed_) {
        // makeBackup();
        try {
            doc_.save_file(filename_.c_str());
        } catch (const std::exception &e) {
            fmt::print("Error saving file: {}\n", e.what());
            return false;
        }
        changed_ = false;
    }
    return true;
}

const SplitNode *Lss::findOrAppendSplit(const std::string &when, const std::string &type, const std::string &name, bool &wasAppend) {
    for (auto &split: splits_) {
        if (split.name == name && split.when == when && split.type == type) {
            wasAppend = false;
            return &split;
        }
    }
    auto splitsNode = ensureSplitParent(when, type);
    auto splitNode = splitsNode.select_node(fmt::format("HierarchicalSplitViewModel[Split='{}']", name).c_str()).node();
    if (splitNode) {
        wasAppend = false;
        return nullptr;
    } else {
        splitNode = splitsNode.append_child("HierarchicalSplitViewModel").append_child("Split");
        const auto *xsiType = typeToXsiType(type);
        splitNode.append_attribute("xsi:type").set_value(xsiType);
        splitNode.text().set(name.c_str());
        SplitNode newNode;
        newNode.split = splitNode;
        newNode.when = when;
        newNode.type = type;
        newNode.buildDisplayName();
        splits_.emplace_back(std::move(newNode));
        changed_ = true;
        wasAppend = true;
        return &splits_.back();
    }
}

pugi::xml_node Lss::ensureSplitTree() {
    auto runNode = doc_.child("Run");
    auto autoSplitterSettingsNode = runNode.child("AutoSplitterSettings");
    if (!autoSplitterSettingsNode) {
        autoSplitterSettingsNode = runNode.append_child("AutoSplitterSettings");
    }
    auto mainViewModelNode = autoSplitterSettingsNode.child("MainViewModel");
    if (!mainViewModelNode) {
        mainViewModelNode = autoSplitterSettingsNode.append_child("MainViewModel");
        mainViewModelNode.append_attribute("xmlns:xsd").set_value("http://www.w3.org/2001/XMLSchema");
        mainViewModelNode.append_attribute("xmlns:xsi").set_value("http://www.w3.org/2001/XMLSchema-instance");
    }
    auto selectedGameNode = mainViewModelNode.child("SelectedGame");
    if (!selectedGameNode) {
        selectedGameNode = mainViewModelNode.append_child("SelectedGame");
        selectedGameNode.text().set(gameName_.c_str());
    }
    auto gameNameViewNodeName = fmt::format("{}ViewModel", gameName_);
    auto gameViewModelNode = mainViewModelNode.child(gameNameViewNodeName.c_str());
    if (!gameViewModelNode) {
        gameViewModelNode = mainViewModelNode.append_child(gameNameViewNodeName.c_str());
    }
    auto startAutomaticallyNode = gameViewModelNode.child("StartAutomatically");
    if (!startAutomaticallyNode) {
        startAutomaticallyNode = gameViewModelNode.append_child("StartAutomatically");
        startAutomaticallyNode.text().set("true");
    }
    auto lockIgtToZeroNode = gameViewModelNode.child("LockIgtToZero");
    if (!lockIgtToZeroNode) {
        lockIgtToZeroNode = gameViewModelNode.append_child("LockIgtToZero");
        lockIgtToZeroNode.text().set("false");
    }
    auto enabledRemoveSplitNode = gameViewModelNode.child("EnabledRemoveSplit");
    if (!enabledRemoveSplitNode) {
        enabledRemoveSplitNode = gameViewModelNode.append_child("EnabledRemoveSplit");
        enabledRemoveSplitNode.text().set("false");
    }
    auto splitsNode = gameViewModelNode.child("Splits");
    if (!splitsNode) {
        splitsNode = gameViewModelNode.append_child("Splits");
    }
    return splitsNode;
}

pugi::xml_node Lss::ensureSplitParent(const std::string &when, const std::string &type) {
    if (!splitTree_)
        splitTree_ = ensureSplitTree();
    auto splitsNode = splitTree_;
    auto timingTypeViewModelNode = splitsNode.select_node(fmt::format("HierarchicalTimingTypeViewModel[TimingType='{}']", when).c_str()).node();
    if (!timingTypeViewModelNode) {
        timingTypeViewModelNode = splitsNode.append_child("HierarchicalTimingTypeViewModel");
        timingTypeViewModelNode.append_child("TimingType").text().set(when.c_str());
    }
    auto childrenNode = timingTypeViewModelNode.child("Children");
    if (!childrenNode) {
        childrenNode = timingTypeViewModelNode.append_child("Children");
    }
    auto splitTypeViewModelNode = childrenNode.select_node(fmt::format("HierarchicalSplitTypeViewModel[{}SplitType='{}']", gameName_, type).c_str()).node();
    if (!splitTypeViewModelNode) {
        splitTypeViewModelNode = childrenNode.append_child("HierarchicalSplitTypeViewModel");
        splitTypeViewModelNode.append_child(fmt::format("{}SplitType", gameName_).c_str()).text().set(type.c_str());
    }
    childrenNode = splitTypeViewModelNode.child("Children");
    if (!childrenNode) {
        childrenNode = splitTypeViewModelNode.append_child("Children");
    }
    return childrenNode;
}

/*
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
*/

}
