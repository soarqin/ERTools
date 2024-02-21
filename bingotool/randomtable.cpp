#include "randomtable.h"

#include <fmt/ostream.h>
#include <fstream>
#include <algorithm>
#include <random>
#include <functional>

static void stripString(std::string &str) {
    str.erase(0, str.find_first_not_of(" \t"));
    str.erase(str.find_last_not_of(" \t") + 1);
}

static std::vector<std::string> splitString(const std::string &str, char sep) {
    std::vector<std::string> result;
    std::string::size_type pos1 = 0, pos2 = str.find(sep);
    while (pos2 != std::string::npos) {
        auto s = str.substr(pos1, pos2 - pos1);
        stripString(s);
        result.emplace_back(std::move(s));
        pos1 = pos2 + 1;
        pos2 = str.find(sep, pos1);
    }
    if (pos1 < str.length()) {
        auto s = str.substr(pos1);
        stripString(s);
        result.emplace_back(std::move(s));
    }
    return result;
}

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
static const std::string randomGroupEntry(RandomGroup *group, RANDENGINE &re) {
    int weight = re() % group->totalWeight;
    for (auto ite = group->entries.begin(); ite != group->entries.end(); ite++) {
        auto &e = *ite;
        if (weight < e.weight) {
            auto res = e.name;
            group->totalWeight -= e.weight;
            group->entries.erase(ite);
            return res;
        }
        weight -= e.weight;
    }
    for (auto ite = group->groups.begin(); ite != group->groups.end(); ite++) {
        auto &ref = *ite;
        if (weight < ref.weight) {
            const auto res = randomGroupEntry(ref.group, re);
            if (--ref.maxCount <= 0) {
                group->totalWeight -= ref.weight;
                group->groups.erase(ite);
            }
            return res;
        }
        weight -= ref.weight;
    }
    return "";
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

void RandomTable::load(const char *filename) {
    root_ = RandomGroup();
    groups_.clear();
    struct CurrentGroup {
        RandomGroup *group;
        size_t indent;
    };
    std::vector<CurrentGroup> indents = {{&root_, 0}};
    RandomGroup *lastGroup = &root_;
    std::function<void(std::string)> f = [this, &indents, &lastGroup, &f](const std::string &fn) {
        std::ifstream file("data/" + fn);
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
                    f(incfn);
                    break;
                }
                case '+': {
                    auto sl = splitString(line.substr(1), ',');
                    if (sl.size() == 1) {
                        if (pos > 0) break;
                        lastGroup = &groups_[sl[0]];
                        if (lastGroup->name.empty()) lastGroup->name = sl[2];
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
            }
        }
        file.close();
    };
    f(filename);
    calcGroupWeight(&root_);
    // logGroup(&root_);
}

void RandomTable::generate(const char *filename, std::vector<std::string> &result) const {
    auto gen = root_;
    std::mt19937 rd((std::random_device())());
    std::ofstream ofs("data/squares.txt");
    for (int i = 0; i < 25; i++) {
        const auto s = randomGroupEntry(&gen, rd);
        fmt::print(ofs, "{}\n", s);
        result.emplace_back(s);
    }
    ofs.close();
}
