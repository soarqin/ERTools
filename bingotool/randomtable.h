#pragma once

#include <vector>
#include <unordered_map>
#include <string>

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

class RandomTable {
public:
    void load(const char *filename);
    void generate(const char *filename, std::vector<std::string> &result) const;

private:
    RandomGroup root_;
    std::unordered_map<std::string, RandomGroup> groups_;
};
