#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

struct RandomGroupRef;

struct RandomEntry {
    std::string name;
    int weight = 0;
};

struct RandomGroup {
    std::string name;
    std::vector<RandomGroupRef> groups;
    std::vector<RandomEntry> entries;
    int totalWeight = 0;
};

struct RandomGroupRef {
    RandomGroup *group = nullptr;
    int weight = 0;
    int maxCount = 0;
};

struct MutualExclusion {
    std::vector<std::string> entries;
};

class RandomTable {
public:
    void load(const std::string &filename);
    void generate(std::vector<std::string> &result) const;

private:
    RandomGroup root_;
    std::unordered_map<std::string, RandomGroup> groups_;
    std::unordered_map<std::string, MutualExclusion> mutualExclusions_;
};
