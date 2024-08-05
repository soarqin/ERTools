#include "randomtable.h"
#include "common.h"

#include <fmt/ostream.h>
#include <fstream>
#include <algorithm>
#include <random>
#include <functional>
#include <filesystem>

std::string RandomTable::lastFilename_;

static void calcGroupWeight(RandomGroup *group) {
    group->totalWeight = 0;
    for (auto &entry : group->entries) {
        group->totalWeight += entry.weight;
    }
    for (auto &ref : group->groups) {
        calcGroupWeight(ref.group);
        group->totalWeight += ref.weight;
    }
}

template<class RANDENGINE>
static std::string randomGroupEntry(RandomGroup *group, const std::unordered_map<std::string, MutualExclusion> &mutualExclusions, std::unordered_set<std::string> &exclusions, RANDENGINE &re) {
    while (true) {
        if (group->totalWeight == 0) { return ""; }
        if (exclusions.find(group->name) != exclusions.end()) { return ""; }
        std::string res;
        int weight = re() % group->totalWeight;
        for (auto ite = group->entries.begin(); ite != group->entries.end(); ite++) {
            auto &e = *ite;
            if (weight < e.weight) {
                res = e.name;
                group->totalWeight -= e.weight;
                group->entries.erase(ite);
                break;
            }
            weight -= e.weight;
        }
        if (!res.empty()) {
            if (exclusions.find(res) != exclusions.end()) continue;
            return res;
        }
        for (auto ite = group->groups.begin(); ite != group->groups.end();) {
            auto &ref = *ite;
            if (weight < ref.weight) {
                res = randomGroupEntry(ref.group, mutualExclusions, exclusions, re);
                if (res.empty()) {
                    group->totalWeight -= ref.weight;
                    group->groups.erase(ite);
                    if (group->groups.empty()) break;
                    ite = group->groups.begin();
                    weight = re() % group->totalWeight;
                    continue;
                }
                auto ite2 = mutualExclusions.find(group->name);
                if (ite2 != mutualExclusions.end()) {
                    exclusions.insert(ite2->second.entries.begin(), ite2->second.entries.end());
                }
                auto ite3 = mutualExclusions.find(res);
                if (ite3 != mutualExclusions.end()) {
                    exclusions.insert(ite3->second.entries.begin(), ite3->second.entries.end());
                }
                if (--ref.maxCount <= 0) {
                    group->totalWeight -= ref.weight;
                    group->groups.erase(ite);
                }
                return res;
            }
            weight -= ref.weight;
            ite++;
        }
        return "";
    }
}

static void logGroup(RandomGroup *group, int indent = 0) {
    for (auto &entry : group->entries) {
        for (int i = 0; i < indent; i++) { printf("  "); }
        fmt::print("- {} {}\n", entry.weight, entry.name);
    }
    for (auto &ref : group->groups) {
        for (int i = 0; i < indent; i++) { printf("  "); }
        fmt::print("+ {},{} {}\n", ref.weight, ref.maxCount, ref.group->name);
        logGroup(ref.group, indent + 1);
    }
}

void RandomTable::load(const std::string &filename) {
    root_ = RandomGroup();
    groups_.clear();
    struct CurrentGroup {
        RandomGroup *group;
        size_t indent;
    };
    std::vector<CurrentGroup> indents = {{&root_, 0}};
    RandomGroup *lastGroup = &root_;
    std::function<void(std::string)> f = [this, &indents, &lastGroup, &f](const std::string &fn) {
        auto p = std::filesystem::path(fn);
        auto parent = p.parent_path().string();
        std::ifstream file(p);
        if (!file.is_open()) { return; }
        while (!file.eof()) {
            std::string line;
            std::getline(file, line);
            auto pos = line.find_first_not_of(" \t");
            if (pos == std::string::npos) { continue; }
            line = line.substr(pos);
            if (line.empty() || line[0] == '#' || line[0] == ';') { continue; }
            auto lastIndent = indents.back().indent;
            if (line.substr(0, 4) == "====") {
                lastGroup = nullptr;
                indents[0].group = nullptr;
                continue;
            }
            if (pos > lastIndent) {
                if (lastGroup) {
                    indents.emplace_back(CurrentGroup{lastGroup, pos});
                }
            } else {
                while (pos < lastIndent) {
                    indents.pop_back();
                    lastIndent = indents.back().indent;
                }
            }
            switch (line[0]) {
                case '@': {
                    auto incfn = line.substr(1);
                    stripString(incfn);
                    if (incfn.empty()) break;
                    auto newfn = parent;
                    newfn.push_back('/');
                    newfn.append(incfn);
                    auto path = std::filesystem::u8path(newfn);
                    f(path.string());
                    break;
                }
                case '+': {
                    auto sl = splitString(line.substr(1), ',');
                    if (sl.size() == 1) {
                        if (pos > 0) break;
                        lastGroup = &groups_[sl[0]];
                        if (lastGroup->name.empty()) lastGroup->name = sl[0];
                        break;
                    }
                    if (sl.size() == 3) {
                        auto w = std::stoi(sl[0]);
                        if (w <= 0) break;
                        auto &group = groups_[sl[2]];
                        if (group.name.empty()) group.name = sl[2];
                        lastGroup = &group;
                        auto g = indents.back().group;
                        if (g) {
                            auto &ref = g->groups.emplace_back();
                            ref.group = &group;
                            ref.weight = w;
                            ref.maxCount = std::stoi(sl[1]);
                        }
                    }
                    break;
                }
                case '-': {
                    lastGroup = nullptr;
                    auto sl = splitString(line.substr(1), ',');
                    if (sl.size() == 2) {
                        auto w = std::stoi(sl[0]);
                        if (w <= 0) break;
                        auto g = indents.back().group;
                        if (g) {
                            auto &entry = g->entries.emplace_back();
                            entry.name = sl[1];
                            entry.weight = w;
                        }
                    }
                    break;
                }
                case '^': {
                    auto sl = splitString(line.substr(1), ',');
                    if (sl.size() > 1) {
                        std::vector<std::string> entries;
                        for (const auto &S : sl) {
                            entries.push_back(S);
                        }
                        for (const auto &S : sl) {
                            mutualExclusions_[S].entries = entries;
                        }
                    }
                    break;
                }
            }
        }
        file.close();
    };
    f(filename);
    calcGroupWeight(&root_);
    lastFilename_ = filename;
    // logGroup(&root_);
}

void RandomTable::generate(std::vector<std::string> &result) const {
    auto gen = root_;
    std::mt19937 rd((std::random_device())());
    result.clear();
    std::unordered_set<std::string> exclusions;
    for (int i = 0; i < 25; i++) {
        auto s = randomGroupEntry(&gen, mutualExclusions_, exclusions, rd);
        result.emplace_back(s);
    }
    std::shuffle(result.begin(), result.end(), rd);
}
