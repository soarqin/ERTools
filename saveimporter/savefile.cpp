/*
 * Copyright (c) 2022 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "savefile.h"
#include "mem.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstring>

inline void readWString(std::wstring &output, const uint16_t *input) {
    while (*input) {
        output.push_back(*input++);
    }
}

inline void findUserIDOffset(CharSlot *slot, const std::function<void(int offset)> &func) {
    struct FindParam {
        CharSlot *slot;
        const std::function<void(int offset)> &func;
    } param = {slot, func};
    kmpSearch(slot->data.data() + 0x1E0000, slot->data.size() - 0x1E0000, "\x4D\x4F\x45\x47\x00\x26\x04\x21", 8, [](int offset, void *data) {
        offset += 0x1E0000;
        auto *slot = ((FindParam*)data)->slot;
        offset += *(int*)&slot->data[offset - 4];
        offset += 4;
        if (memcmp(&slot->data[offset], "\x46\x4F\x45\x47\x00\x26\x04\x21", 8) != 0) return;
        offset += *(int*)&slot->data[offset - 4];
        offset += *(int*)&slot->data[offset] + 4 + 0x20078;
        ((FindParam*)data)->func(offset);
    }, &param);
}

inline size_t findLevelOffset(const std::vector<uint8_t> &data) {
    const auto *ptr = data.data();
    size_t sz = data.size() - 0x30;
    for (size_t i = 0; i < sz; ++i) {
        if (*(int*)(ptr + i) + *(int*)(ptr + i + 4) + *(int*)(ptr + i + 8) + *(int*)(ptr + i + 12) +
            *(int*)(ptr + i + 16) + *(int*)(ptr + i + 20) + *(int*)(ptr + i + 24) + *(int*)(ptr + i + 28)
            == 79 + *(int*)(ptr + i + 44)) {
            return i + 44;
        }
    }
    return 0;
}

SaveFile::SaveFile(const std::wstring &filename) {
    std::ifstream stream(std::filesystem::path(filename), std::ios::in | std::ios::binary);
    if (!stream.is_open()) return;
    load(stream);
    stream.close();
    if (ok_) filename_ = filename;
}

SaveFile::SaveFile(const std::string &data, const std::wstring &saveAs) {
    std::istringstream stream(data, std::ios::in | std::ios::binary);
    load(stream);
    std::ofstream ofs(std::filesystem::path(saveAs), std::ios::out | std::ios::binary);
    if (ofs.is_open()) {
        ofs.write((const char*)data.data(), data.size());
        ofs.close();
    }
    if (ok_) filename_ = saveAs;
}

void SaveFile::load(std::istream &stream) {
    ok_ = false;
    uint32_t magic = 0;
    stream.read((char*)&magic, sizeof(uint32_t));
    if (magic == 0x34444E42) {
        saveType_ = Steam;
        ok_ = true;
        stream.seekg(0, std::ios::beg);
        header_.resize(0x300);
        stream.read((char*)header_.data(), 0x300);
        int slotCount = *(int*)&header_[12];
        slots_.resize(slotCount);
        for (int i = 0; i < slotCount; ++i) {
            int slotSize = *(int*)&header_[0x48 + i * 0x20];
            int slotOffset = *(int*)&header_[0x50 + i * 0x20];
            int nameOffset = *(int*)&header_[0x54 + i * 0x20];
            std::wstring slotFilename;
            readWString(slotFilename, (const uint16_t*)&header_[nameOffset]);
            std::unique_ptr<SaveSlot> slot;
            if (slotSize == 0x280010) {
                slot = std::make_unique<CharSlot>();
                slot->slotType = SaveSlot::Character;
            } else if (slotSize == 0x60010){
                slot = std::make_unique<SummarySlot>();
                slot->slotType = SaveSlot::Summary;
            } else {
                slot = std::make_unique<SaveSlot>();
                slot->slotType = SaveSlot::Other;
            }
            slot->offset = slotOffset;
            slot->filename = slotFilename;
            stream.seekg(slotOffset, std::ios::beg);
            stream.read((char*)slot->md5hash, 16);
            slotSize -= 16;
            slot->data.resize(slotSize);
            stream.read((char*)slot->data.data(), slotSize);
            slots_[i] = std::move(slot);
        }
    } else if (magic == 0x2C9C01CB) {
        saveType_ = PS4;
        ok_ = true;
        stream.seekg(0, std::ios::beg);
        header_.resize(0x70);
        stream.read((char*)header_.data(), 0x70);
        slots_.resize(12);
        for (int i = 0; i < 12; ++i) {
            std::unique_ptr<SaveSlot> slot;
            int slotSize;
            if (i < 10) {
                slot = std::make_unique<CharSlot>();
                slot->slotType = SaveSlot::Character;
                slotSize = 0x280000;
            } else if (i < 11){
                slot = std::make_unique<SummarySlot>();
                slot->slotType = SaveSlot::Summary;
                slotSize = 0x60000;
            } else {
                slot = std::make_unique<SaveSlot>();
                slot->slotType = SaveSlot::Other;
                slotSize = 0x240010;
            }
            slot->offset = static_cast<uint32_t>(stream.tellg());
            slot->data.resize(slotSize);
            stream.read((char*)slot->data.data(), slotSize);
            slots_[i] = std::move(slot);
        }
    } else {
        return;
    }
    for (auto &slot: slots_) {
        switch (slot->slotType) {
            case SaveSlot::Character: {
                auto *slot2 = (CharSlot *)slot.get();
                slot2->useridOffset = 0;
                slot2->userid = 0;
                findUserIDOffset(slot2, [slot2](int offset) {
                    slot2->useridOffset = offset;
                    slot2->userid = *(uint64_t*)&slot2->data[offset];
                });
                auto offset = findLevelOffset(slot2->data);
                if (offset >= 0x2C) {
                    slot2->level = *(uint16_t*)(slot2->data.data() + offset);
                    memcpy(slot2->stats, slot2->data.data() + offset - 0x2C, 8 * sizeof(uint32_t));
                } else {
                    slot2->level = 0;
                    memset(slot2->stats, 0, sizeof(slot2->stats));
                }
                break;
            }
            case SaveSlot::Summary: {
                auto *slot2 = (SummarySlot *)slot.get();
                slot2->userid = *(uint64_t *)&slot2->data[4];
                for (size_t i = 0; i < slots_.size(); ++i) {
                    if (slots_[i]->slotType == SaveSlot::Character) {
                        auto level = *(uint16_t*)&slot2->data[0x195E + 0x24C * i + 0x22];
                        if (level == 0) continue;
                        uint8_t available = *(uint8_t*)&slot2->data[0x1954 + i];
                        auto *slot3 = (CharSlot *)slots_[i].get();
                        readWString(slot3->charname, (const uint16_t *)&slot2->data[0x195E + 0x24C * i]);
                        slot3->level = level;
                        slot3->available = available;
                    }
                }
                break;
            }
            default:
                break;
        }
    }
}

void SaveFile::resign(uint64_t userid) {
    std::fstream fs(std::filesystem::path(filename_), std::ios::in | std::ios::out | std::ios::binary);
    if (!fs.is_open()) {
        return;
    }
    for (auto &slot: slots_) {
        auto &data = slot->data;
        if (slot->slotType == SaveSlot::Character) {
            auto *slot2 = (CharSlot*)slot.get();
            slot2->userid = userid;
            *(uint64_t*)&data[slot2->useridOffset] = userid;
        } else if (slot->slotType == SaveSlot::Summary) {
            auto *slot2 = (SummarySlot*)slot.get();
            slot2->userid = userid;
            *(uint64_t*)&data[4] = userid;
        }
        fs.seekg(slot->offset, std::ios::beg);
        md5Hash(data.data(), data.size(), slot->md5hash);
        if (saveType_ == Steam)
            fs.write((const char*)slot->md5hash, 16);
        fs.write((const char*)data.data(), data.size());
    }
    fs.close();
}

void SaveFile::patchSlotTime(int slot, uint32_t millisec) {
    {
        auto &s = slots_[slot];
        if (s->slotType != SaveSlot::Character) { return; }
        *(uint32_t *)(s->data.data() + 8) = millisec;
    }
    for (auto &s: slots_) {
        if (s->slotType == SaveSlot::Summary) {
            *(uint16_t *)&s->data[0x195E + 0x24C * slot + 0x26] = millisec / 1000;
            break;
        }
    }
}

bool SaveFile::exportToFile(const std::wstring &filename, int slot) {
    if (slot < 0 || slot >= slots_.size() || slots_[slot]->slotType != SaveSlot::Character) { return false; }
    std::ofstream ofs(std::filesystem::path(filename), std::ios::out | std::ios::binary);
    if (!ofs.is_open()) { return false; }
    const auto &data = slots_[slot]->data;
    ofs.write((const char*)data.data(), data.size());
    ofs.close();
    return true;
}

bool SaveFile::importFromFile(const std::wstring &filename, int slot) {
    if (slot < 0 || slot >= slots_.size() || slots_[slot]->slotType != SaveSlot::Character) { return false; }
    std::ifstream ifs(std::filesystem::path(filename), std::ios::in | std::ios::binary);
    if (!ifs.is_open()) { return false; }
    ifs.seekg(0, std::ios::end);
    auto &data = slots_[slot]->data;
    if (ifs.tellg() != data.size()) { ifs.close(); return false; }
    ifs.seekg(0, std::ios::beg);
    ifs.read((char*)data.data(), data.size());
    ifs.close();

    auto *slot2 = (CharSlot*)slots_[slot].get();
    slot2->useridOffset = 0;
    slot2->userid = 0;
    findUserIDOffset(slot2, [slot2](int offset) {
        slot2->useridOffset = offset;
        slot2->userid = *(uint64_t*)&slot2->data[offset];
    });

    std::fstream fs(std::filesystem::path(filename_), std::ios::in | std::ios::out | std::ios::binary);

    auto offset = findLevelOffset(data);
    if (offset >= 0x2C) {
        slot2->level = *(uint16_t*)(slot2->data.data() + offset);
        memcpy(slot2->stats, slot2->data.data() + offset - 0x2C, 8 * sizeof(uint32_t));
    } else {
        slot2->level = 0;
        memset(slot2->stats, 0, sizeof(slot2->stats));
    }
    if (offset > 0) {
        for (auto &s: slots_) {
            if (s->slotType == SaveSlot::Summary) {
                /* use global userid */
                slot2->userid = ((SummarySlot*)s.get())->userid;
                *(uint64_t*)&slot2->data[slot2->useridOffset] = slot2->userid;
                /* copy level and name to summary */
                *(uint16_t*)&s->data[0x195E + 0x24C * slot + 0x22] = slot2->level;
                memcpy(&s->data[0x195E + 0x24C * slot], &data[offset + 0x34], 0x22);
                md5Hash(s->data.data(), s->data.size(), s->md5hash);
                fs.seekg(s->offset, std::ios::beg);
                if (saveType_ == Steam)
                    fs.write((const char*)s->md5hash, 16);
                fs.write((const char*)s->data.data(), s->data.size());
                break;
            }
        }
    }
    fs.seekg(slot2->offset, std::ios::beg);
    md5Hash(data.data(), data.size(), slot2->md5hash);
    if (saveType_ == Steam)
        fs.write((const char*)slot2->md5hash, 16);
    fs.write((const char*)data.data(), data.size());
    fs.close();
    return true;
}

bool SaveFile::exportFaceToFile(const std::wstring &filename, int slot) {
    if (slot < 0 || slot >= slots_.size() || slots_[slot]->slotType != SaveSlot::Character) { return false; }
    std::ofstream ofs(std::filesystem::path(filename), std::ios::out | std::ios::binary);
    if (!ofs.is_open()) { return false; }
    const auto &data = slots_[slot]->data;
    struct SearchData {
        std::ofstream &ofs;
        const std::vector<uint8_t> &data;
    };
    SearchData sd = {ofs, data};
    kmpSearch(data.data(), data.size(), "\x46\x41\x43\x45\x04\x00\x00\x00\x20\x01\x00\x00", 12, [](int offset, void *data) {
        auto &sd = *reinterpret_cast<SearchData*>(data);
        sd.ofs.write((const char*)sd.data.data() + offset, 0x120);
    }, &sd);
    ofs.close();
    return true;
}

bool SaveFile::importFaceFromFile(const std::wstring &filename, int slot) {
    if (slot < 0 || slot >= slots_.size() || slots_[slot]->slotType != SaveSlot::Character) { return false; }
    std::ifstream ifs(std::filesystem::path(filename), std::ios::in | std::ios::binary);
    if (!ifs.is_open()) { return false; }
    ifs.seekg(0, std::ios::end);
    uint8_t face[0x120];
    if (ifs.tellg() != 0x120) { ifs.close(); return false; }
    ifs.seekg(0, std::ios::beg);
    ifs.read((char*)face, 0x120);
    ifs.close();
    if (memcmp(face, "\x46\x41\x43\x45\x04\x00\x00\x00\x20\x01\x00\x00", 12) != 0) { return false; }

    auto *slot2 = (CharSlot*)slots_[slot].get();
    auto &data = slot2->data;
    struct SearchData {
        uint8_t *face;
        std::vector<uint8_t> &data;
    };
    SearchData sd = {face, data};
    kmpSearch(data.data(), data.size(), "\x46\x41\x43\x45\x04\x00\x00\x00\x20\x01\x00\x00", 12, [](int offset, void *data) {
        auto &sd = *reinterpret_cast<SearchData*>(data);
        memcpy(sd.data.data() + offset, sd.face, 0x120);
    }, &sd);

    std::fstream fs(std::filesystem::path(filename_), std::ios::in | std::ios::out | std::ios::binary);

    for (auto &s: slots_) {
        if (s->slotType == SaveSlot::Summary) {
            memcpy(&s->data[0x195E + 0x24C * slot + 0x3A], face, 0x120);
            md5Hash(s->data.data(), s->data.size(), s->md5hash);
            fs.seekg(s->offset, std::ios::beg);
            if (saveType_ == Steam)
                fs.write((const char*)s->md5hash, 16);
            fs.write((const char*)s->data.data(), s->data.size());
            break;
        }
    }
    fs.seekg(slot2->offset, std::ios::beg);
    md5Hash(data.data(), data.size(), slot2->md5hash);
    if (saveType_ == Steam)
        fs.write((const char*)slot2->md5hash, 16);
    fs.write((const char*)data.data(), data.size());
    fs.close();
    return true;
}

void SaveFile::listSlots(int slot, const std::function<void(int, const SaveSlot&)> &func) {
#if defined(USE_CLI)
    fprintf(stdout, "SaveType: %s\n", saveType_ == Steam ? "Steam" : "PS4");
    if (saveType_ == Steam) {
        for (auto &s: slots_) {
            if (s->slotType == SaveSlot::Summary) {
                fprintf(stdout, "User ID:  %llu\n", ((SummarySlot *)s.get())->userid);
                break;
            }
        }
    }
    if (slot < 0) {
        for (int i = 0; i < slots_.size(); ++i) {
            listSlot(i);
        }
    } else if (slot < slots_.size()) {
        listSlot(slot);
    }
#else
    if (slot < 0) {
        for (int i = 0; i < slots_.size(); ++i) {
            func(i, *slots_[i]);
        }
    } else if (slot < slots_.size()) {
        func(slot, *slots_[slot]);
    }
#endif
}

void SaveFile::listSlot(int slot) {
    auto &s = slots_[slot];
    switch (s->slotType) {
    case SaveSlot::Character: {
        auto *slot2 = (CharSlot*)s.get();
        if (slot2->charname.empty()) break;
        fprintf(stdout, "%4d: %ls (Level %d)\n", slot, slot2->charname.c_str(), slot2->level);
        break;
    }
    case SaveSlot::Summary:
        fprintf(stdout, "%4d: Summary\n", slot);
        break;
    default:
        fprintf(stdout, "%4d: Other\n", slot);
        break;
    }
}

void SaveFile::fixHashes() {
    if (saveType_ != Steam) { return; }
    std::fstream fs(std::filesystem::path(filename_), std::ios::in | std::ios::out | std::ios::binary);
    uint8_t hash[16];
    for (auto &slot: slots_) {
        md5Hash(slot->data.data(), slot->data.size(), hash);
        if (memcmp(hash, slot->md5hash, 16) != 0) {
            memcpy(slot->md5hash, hash, 16);
            fs.seekg(slot->offset, std::ios::beg);
            fs.write((const char*)hash, 16);
        }
    }
    fs.close();
}

bool SaveFile::verifyHashes() {
    if (saveType_ != Steam) { return true; }
    uint8_t hash[16];
    for (auto &slot: slots_) {
        md5Hash(slot->data.data(), slot->data.size(), hash);
        if (memcmp(hash, slot->md5hash, 16) != 0) {
            return false;
        }
    }
    return true;
}
