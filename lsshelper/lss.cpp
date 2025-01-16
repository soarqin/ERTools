/*
 * LSS Helper - LiveSplit lss file helper
 * Copyright (C) 2024  Soar Qin<soarchin@gmail.com>

 * This file is part of LSS Helper.

 * LSS Helper is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.

 * LSS Helper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with LSS Helper.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lss.h"

#include "util.h"

#if defined(NDEBUG)
#include <wx/msgdlg.h>
#endif
#include <wx/datetime.h>
#include <pugixml.hpp>
#include <fmt/format.h>

namespace lss_helper {

inline std::string mapGameName(const std::string &name) {
    if (name == "Elden Ring")
        return "EldenRing";
    if (name == "Dark Souls III" || name == "Dark Souls 3")
        return "DarkSouls3";
    return "";
}

inline const char *typeToXsiTypeER(const std::string &type) {
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

inline const char *typeToXsiTypeDS3(const std::string &type) {
    if (type == "Bonfire")
        return "Bonfire";
    if (type == "Boss")
        return "Boss";
    if (type == "ItemPickup")
        return "ItemPickup";
    if (type == "Position")
        return "VectorSize";
    if (type == "Flag")
        return "FlagDescription";
    if (type == "Attribute")
        return "Attribute";
    return "";
}

inline std::wstring toWideString(const std::string &str) {
    return wxString::FromUTF8(str).ToStdWstring();
}

void SplitNode::buildDisplayNameER() {
    if (type == "Position") {
        identifier = fmt::format("{},{},{},{},{},{},{}", split.child("Area").text().get(), split.child("Block").text().get(), split.child("Region").text().get(), split.child("Size").text().get(), split.child("X").text().get(), split.child("Y").text().get(), split.child("Z").text().get());
        displayName = toWideString(identifier);
        fullDisplayName = toWideString(fmt::format("{}/{}/{}", when, type, identifier));
    } else if (type == "Flag") {
        identifier = split.text().get();
        displayName = toWideString(identifier);
        fullDisplayName = toWideString(fmt::format("{}/{}/{}", when, type, identifier));
    } else {
        identifier = split.text().get();
        const auto &e = gEnums(type, identifier);
        displayName = toWideString(e.name.empty() ? identifier : e.disp);
        fullDisplayName = toWideString(fmt::format("{}/{}/{}", when, type, e.name.empty() ? identifier : e.disp));
    }
}

void SplitNode::buildDisplayNameDS3() {
    if (type == "Position") {
        auto pos = split.child("Position");
        identifier = fmt::format("{},{},{},{},{}", split.child("Description").text().get(), pos.child("X").text().get(), pos.child("Y").text().get(), pos.child("Z").text().get(), split.child("Size").text().get());
        displayName = toWideString(identifier);
        fullDisplayName = toWideString(fmt::format("{}/{}/{}", when, type, identifier));
    } else if (type == "Flag") {
        identifier = fmt::format("{},{}", split.child("Flag").text().get(), split.child("Description").text().get());
        displayName = toWideString(identifier);
        fullDisplayName = toWideString(fmt::format("{}/{}/{}", when, type, identifier));
    } else if (type == "Attribute") {
        std::string ns;
        split.find_attribute([&ns](const pugi::xml_attribute &attr) {
            if (strncmp(attr.name(), "xmlns:", 6) != 0) return false;
            ns = attr.name() + 6;
            return true;
        });
        auto node1 = split.child((ns + ":AttributeType").c_str());
        auto node2 = split.child((ns + ":Level").c_str());
        identifier = fmt::format("{},{}", node1.text().get(), node2.text().get());
        displayName = toWideString(identifier);
        fullDisplayName = toWideString(fmt::format("{}/{}/{}", when, type, identifier));
    } else {
        identifier = split.text().get();
        const auto &e = gEnums(type, identifier);
        displayName = toWideString(e.name.empty() ? identifier : e.disp);
        fullDisplayName = toWideString(fmt::format("{}/{}/{}", when, type, e.name.empty() ? identifier : e.disp));
    }
}

int Lss::open(const wchar_t *filename) {
    loaded_ = false;
    pugi::xml_document doc;
    auto result = doc.load_file(filename);
    if (!result) {
        return -1;
    }
    gameName_ = mapGameName(doc.child("Run").child("GameName").text().get());
    if (gameName_.empty()) {
        return -2;
    }
    segs_.clear();
    splits_.clear();
    doc_ = std::move(doc);
    gEnums.load(gameName_);
    filename_ = filename;
    for (auto node: doc_.select_nodes("/Run/Segments/Segment")) {
        auto segment = node.node();
        auto &sn = segs_.emplace_back();
        sn.seg = segment;
        auto name = segment.child("Name").text().get();
#if !defined(NDEBUG)
        fmt::print(stdout, "Segment: {}\n", name);
#endif
    }
    if (gameName_ == "EldenRing") {
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
                    snode.buildDisplayNameER();
                }
            }
        }
    } else {
        auto xpath = fmt::format("/Run/AutoSplitterSettings/MainViewModel/{}ViewModel/SplitsViewModel/Splits/SplitTimingViewModel", gameName_);
        for (auto node: doc_.select_nodes(xpath.c_str())) {
            auto timingType = node.node().child("TimingType").text().get();
            for (auto node2: node.node().select_nodes("Children/SplitTypeViewModel")) {
                auto splitType = node2.node().child("SplitType").text().get();
                for (auto node3: node2.node().select_nodes("Children/SplitViewModel")) {
                    auto &snode = splits_.emplace_back();
                    snode.split = node3.node().child("Split");
                    snode.when = timingType;
                    snode.type = splitType;
                    snode.buildDisplayNameDS3();
                }
            }
        }
    }

    changed_ = false;
    loaded_ = true;

    pugi::xml_document doch;
    if (!doch.load_file((filename_ + L".helper").c_str())) return 0;
    auto assignments = doch.child("Assignments");
    if (!assignments) return 0;
    if (assignments.attribute("GameName").as_string() != gameName_) return 0;
    for (auto node: assignments.children()) {
        auto segIndex = node.attribute("SegIndex").as_uint();
        if (segIndex >= segs_.size()) continue;
        const auto *seg = &segs_[segIndex];
        if (!seg->seg) continue;
        auto segName = node.attribute("SegName").as_string();
        if (strcmp(segName, seg->seg.child("Name").text().get()) != 0) {
            seg = nullptr;
            for (auto &seg2: segs_) {
                if (seg2.seg && strcmp(segName, seg2.seg.child("Name").text().get()) == 0) {
                    seg = &seg2;
                    break;
                }
            }
            if (!seg) continue;
        }
        auto splitName = node.text().get();
        auto when = node.attribute("When").as_string();
        auto type = node.attribute("Type").as_string();
        auto snode = find(when, type, splitName);
        if (snode) {
            snode->assign(seg->seg);
            seg->assign(snode->split, snode->fullDisplayName);
        }
    }
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
            auto splitNode = splitRootNode.select_node(fmt::format("HierarchicalTimingTypeViewModel[TimingType='{}']/Children/HierarchicalSplitTypeViewModel[{}SplitType='{}']/Children/HierarchicalSplitViewModel[Split='{}']", split.when, gameName_, split.type, split.identifier).c_str()).node();
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
    try {
        doch.save_file(dochFilename.c_str(), "  ");
    } catch (const std::exception &e) {
#if !defined(NDEBUG)
        fmt::print("Error saving helper file: {}\n", e.what());
#else
        wxMessageBox(fmt::format("Error saving helper file: {}", e.what()), "Error", wxICON_ERROR);
#endif
        return false;
    }
    if (changed_) {
        // makeBackup();
        try {
            doc_.save_file(filename_.c_str());
        } catch (const std::exception &e) {
#if !defined(NDEBUG)
            fmt::print("Error saving lss file: {}\n", e.what());
#else
            wxMessageBox(fmt::format("Error saving lss file: {}", e.what()), "Error", wxICON_ERROR);
#endif
            return false;
        }
        changed_ = false;
    }
    return true;
}

const SegNode &Lss::insertNewSegment(const std::string &name, int index) {
    auto segmentsNode = ensureSegmentParent();
    auto toAppend = index < 0 || size_t(index) >= segs_.size();
    auto segmentNode = toAppend ? segmentsNode.append_child("Segment") : segmentsNode.insert_child_before("Segment", segs_[index].seg);
    segmentNode.append_child("Name").text().set(name.c_str());
    changed_ = true;
    if (toAppend) {
        auto &seg = segs_.emplace_back();
        seg.seg = segmentNode;
        return seg;
    }
    segs_.emplace(segs_.begin() + index, SegNode { segmentNode });
    return segs_[index];
}

void Lss::moveSegmentDown(int index) {
    auto &seg = segs_[index].seg;
    auto &seg2 = segs_[index + 1].seg;
    seg.remove_child("SplitTimes");
    seg.parent().insert_move_after(seg, seg2);
    seg2.remove_child("SplitTimes");
    std::swap(segs_[index], segs_[index + 1]);
    changed_ = true;
}

void Lss::deleteSegment(int index) {
    auto &seg = segs_[index];
    if (seg.split) {
        for (auto &split: splits_) {
            if (split.seg == seg.seg) {
                split.assign(pugi::xml_node());
                break;
            }
        }
    }
    seg.seg.parent().remove_child(segs_[index].seg);
    segs_.erase(segs_.begin() + index);
    changed_ = true;
}

void Lss::deleteSplit(int index) {
    auto &split = splits_[index];
    if (split.seg) {
        for (auto &seg: segs_) {
            if (seg.split == split.split) {
                seg.assign(pugi::xml_node(), L"");
                break;
            }
        }
    }
    split.split.parent().parent().remove_child(split.split.parent());
    splits_.erase(splits_.begin() + index);
    changed_ = true;
}

const SplitNode *Lss::findOrAppendSplit(const std::string &when, const std::string &type, const std::string &identifier, bool &wasAppend) {
    for (auto &split: splits_) {
        if (split.identifier == identifier && split.when == when && split.type == type) {
            wasAppend = false;
            return &split;
        }
    }
    auto splitsNode = ensureSplitParent(when, type);
    bool isER = gameName_ == "EldenRing";
    if (isER) {
        auto splitNode = splitsNode.select_node(fmt::format("HierarchicalSplitViewModel[Split='{}']", identifier).c_str()).node();
        if (splitNode) {
            wasAppend = false;
            return nullptr;
        }
        splitNode = splitsNode.append_child("HierarchicalSplitViewModel").append_child("Split");
        const auto *xsiType = typeToXsiTypeER(type);
        splitNode.append_attribute("xsi:type").set_value(xsiType);
        if (type == "Position") {
            auto res = splitString(identifier, ',');
            if (res.size() < 7) return nullptr;
            splitNode.append_child("Area").text().set(res[0].c_str());
            splitNode.append_child("Block").text().set(res[1].c_str());
            splitNode.append_child("Region").text().set(res[2].c_str());
            splitNode.append_child("Size").text().set(res[3].c_str());
            splitNode.append_child("X").text().set(res[4].c_str());
            splitNode.append_child("Y").text().set(res[5].c_str());
            splitNode.append_child("Z").text().set(res[6].c_str());
        } else {
            splitNode.text().set(identifier.c_str());
        }
        SplitNode newNode;
        newNode.split = splitNode;
        newNode.when = when;
        newNode.type = type;
        newNode.buildDisplayNameER();
        splits_.emplace_back(std::move(newNode));
        changed_ = true;
        wasAppend = true;
        return &splits_.back();
    }
    auto splitNode = splitsNode.select_node(fmt::format("SplitViewModel[Split='{}']", identifier).c_str()).node();
    if (splitNode) {
        wasAppend = false;
        return nullptr;
    }
    splitNode = splitsNode.append_child("SplitViewModel").append_child("Split");
    const auto *xsiType = typeToXsiTypeDS3(type);
    if (type == "Position" || type == "Flag"){
        splitNode.append_attribute("xsi:type").set_value(xsiType);
    } else {
        const char *nameSpace = "q1";
        splitNode.append_attribute(fmt::format("xmlns:{}", nameSpace).c_str()).set_value(gameName_.c_str());
        splitNode.append_attribute("xsi:type").set_value(fmt::format("{}:{}", nameSpace, xsiType).c_str());
    }
    if (type == "Position") {
        auto res = splitString(identifier, ',');
        if (res.size() < 5) return nullptr;
        splitNode.append_child("Size").text().set(res[4].c_str());
        auto posNode = splitNode.append_child("Position");
        posNode.append_child("X").text().set(res[1].c_str());
        posNode.append_child("Y").text().set(res[2].c_str());
        posNode.append_child("Z").text().set(res[3].c_str());
        splitNode.append_child("Description").text().set(res[0].c_str());
    } else if (type == "Flag") {
        auto res = splitString(identifier, ',');
        if (res.size() < 3) return nullptr;
        splitNode.append_child("Flag").text().set(res[0].c_str());
        splitNode.append_child("Description").text().set(res[1].c_str());
        splitNode.append_child("State").text().set("false");
    } else if (type == "Attribute") {
        auto res = splitString(identifier, ',');
        if (res.size() < 2) return nullptr;
        auto ns = splitNode.append_child("AttributeType");
        ns.text().set(res[0].c_str());
        splitNode.append_child("Level").text().set(res[1].c_str());
    } else {
        splitNode.text().set(identifier.c_str());
    }
    SplitNode newNode;
    newNode.split = splitNode;
    newNode.when = when;
    newNode.type = type;
    newNode.buildDisplayNameDS3();
    splits_.emplace_back(std::move(newNode));
    changed_ = true;
    wasAppend = true;
    return &splits_.back();
}

const SplitNode *Lss::find(const std::string &when, const std::string &type, const std::string &name) {
    for (auto &split: splits_) {
        if (split.identifier == name && split.when == when && split.type == type) {
            return &split;
        }
    }
    return nullptr;
}

pugi::xml_node Lss::ensureSegmentParent() {
    auto runNode = doc_.child("Run");
    auto segmentsNode = runNode.child("Segments");
    if (!segmentsNode) {
        segmentsNode = runNode.append_child("Segments");
    }
    return segmentsNode;
}

pugi::xml_node Lss::ensureSplitTree() {
    bool isER = gameName_ == "EldenRing";
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
    if (isER) {
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
    auto splitsViewModelNode = gameViewModelNode.child("SplitsViewModel");
    if (!splitsViewModelNode) {
        splitsViewModelNode = gameViewModelNode.append_child("SplitsViewModel");
    }
    auto splitsNode = splitsViewModelNode.child("Splits");
    if (!splitsNode) {
        splitsNode = splitsViewModelNode.append_child("Splits");
    }
    return splitsNode;
}

pugi::xml_node Lss::ensureSplitParent(const std::string &when, const std::string &type) {
    if (!splitTree_)
        splitTree_ = ensureSplitTree();
    bool isER = gameName_ == "EldenRing";
    if (isER) {
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
    auto splitsNode = splitTree_;
    auto splitTimingViewModelNode = splitsNode.select_node(fmt::format("SplitTimingViewModel[TimingType='{}']", when).c_str()).node();
    if (!splitTimingViewModelNode) {
        splitTimingViewModelNode = splitsNode.append_child("SplitTimingViewModel");
        splitTimingViewModelNode.append_child("TimingType").text().set(when.c_str());
    }
    auto childrenNode = splitTimingViewModelNode.child("Children");
    if (!childrenNode) {
        childrenNode = splitTimingViewModelNode.append_child("Children");
    }
    auto splitTypeViewModelNode = childrenNode.select_node(fmt::format("SplitTypeViewModel[SplitType='{}']", type).c_str()).node();
    if (!splitTypeViewModelNode) {
        splitTypeViewModelNode = childrenNode.append_child("SplitTypeViewModel");
        splitTypeViewModelNode.append_child("SplitType").text().set(type.c_str());
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
