#include "randomtable.h"
#include "common.h"

#include <fmt/ostream.h>
#include <fstream>
#include <algorithm>
#include <random>
#include <functional>
#include <filesystem>

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
            auto ite = mutualExclusions.find(res);
            if (ite != mutualExclusions.end()) {
                exclusions.insert(ite->second.entries.begin(), ite->second.entries.end());
            }
            return res;
        }
        for (auto ite = group->groups.begin(); ite != group->groups.end();) {
            auto &ref = *ite;
            if (weight < ref.weight) {
                res = randomGroupEntry(ref.group, mutualExclusions, exclusions, re);
                if (res.empty()) {
                    group->totalWeight -= ref.weight;
                    ite = group->groups.erase(ite);
                    weight -= ref.weight;
                    continue;
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
        auto p = std::filesystem::u8path(fn);
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
                    f(newfn);
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
                        auto &group = groups_[sl[2]];
                        if (group.name.empty()) group.name = sl[2];
                        lastGroup = &group;
                        auto g = indents.back().group;
                        if (g) {
                            auto &ref = g->groups.emplace_back();
                            ref.group = &group;
                            ref.weight = std::stoi(sl[0]);
                            ref.maxCount = std::stoi(sl[1]);
                        }
                    }
                    break;
                }
                case '-': {
                    lastGroup = nullptr;
                    auto sl = splitString(line.substr(1), ',');
                    if (sl.size() == 2) {
                        auto g = indents.back().group;
                        if (g) {
                            auto &entry = g->entries.emplace_back();
                            entry.name = sl[1];
                            entry.weight = std::stoi(sl[0]);
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
