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
#include <windows.h>

inline void readWString(std::wstring &output, const uint16_t *input) {
    while (*input) {
        output.push_back(*input++);
    }
}

inline void findUserIDOffset(CharSlot *slot, const std::function<void(int offset)> &func) {
    size_t pointer = 0x1E0000;
    while (true) {
        auto offset = boyer_moore(slot->data.data() + pointer, slot->data.size() - pointer, (const uint8_t *)"\x4D\x4F\x45\x47\x00\x26\x04\x21", 8);
        if (offset == (size_t)-1) return;
        offset += 0x1E0000;
        offset += *(int *)&slot->data[offset - 4];
        offset += 4;
        if (memcmp(&slot->data[offset], "\x46\x4F\x45\x47\x00\x26\x04\x21", 8) != 0) {
            pointer = offset + 1;
            continue;
        }
        offset += *(int *)&slot->data[offset - 4];
        offset += *(int *)&slot->data[offset] + 4 + 0x20078;
        func((int)offset);
        return;
    }
}

inline size_t findStatOffset(const std::vector<uint8_t> &data, int level, const wchar_t *name) {
    const auto *ptr = data.data();
    size_t sz = data.size() - 0x130;
    for (size_t i = 0; i < sz; ++i) {
        auto nl = *(int*)(ptr + i + 0x2C);
        if (nl == level && *(int*)(ptr + i) + *(int*)(ptr + i + 4) + *(int*)(ptr + i + 8) + *(int*)(ptr + i + 12) +
            *(int*)(ptr + i + 16) + *(int*)(ptr + i + 20) + *(int*)(ptr + i + 24) + *(int*)(ptr + i + 28)
            == 79 + nl && lstrcmpW(name, (const wchar_t*)(ptr + i + 0x60)) == 0) {
            return i;
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
        ofs.write((const char*)data.data(), (int)data.size());
        ofs.close();
    }
    if (ok_) filename_ = saveAs;
}

void SaveFile::load(std::istream &stream) {
    ok_ = false;
    header_.clear();
    slots_.clear();
    summarySlot_ = -1;
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
                summarySlot_ = i;
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
                summarySlot_ = i;
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
    for (size_t idx = 0; idx < slots_.size(); idx++) {
        auto &slot = slots_[idx];
        switch (slot->slotType) {
            case SaveSlot::Character: {
                auto *slot2 = (CharSlot *)slot.get();
                slot2->useridOffset = 0;
                slot2->userid = 0;
                findUserIDOffset(slot2, [slot2](int offset) {
                    slot2->useridOffset = offset;
                    slot2->userid = *(uint64_t*)&slot2->data[offset];
                });
                auto &summaryData = slots_[summarySlot_]->data;
                auto rlevel = *(int*)&summaryData[0x195E + 0x24C * idx + 0x22];
                const auto *rname = (const wchar_t *)&summaryData[0x195E + 0x24C * idx];
                auto offset = findStatOffset(slot2->data, rlevel, rname);
                if (offset > 0) {
                    slot2->statOffset = (int)offset;
                    slot2->level = *(uint16_t*)(slot2->data.data() + offset + 0x2C);
                    memcpy(slot2->stats, slot2->data.data() + offset, 8 * sizeof(uint32_t));
                } else {
                    slot2->statOffset = 0;
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
        fs.write((const char*)data.data(), (int)data.size());
    }
    fs.close();
}

bool SaveFile::resign(int slot, uint64_t userid) {
    if (slot < 0 || slot >= slots_.size() || slots_[slot]->slotType != SaveSlot::Character) { return false; }
    std::fstream fs(std::filesystem::path(filename_), std::ios::in | std::ios::out | std::ios::binary);
    if (!fs.is_open()) {
        return false;
    }
    {
        auto *slot2 = (CharSlot*)slots_[slot].get();
        auto &data = slot2->data;
        slot2->userid = userid;
        *(uint64_t*)&data[slot2->useridOffset] = userid;
        fs.seekg(slot2->offset, std::ios::beg);
        md5Hash(data.data(), data.size(), slot2->md5hash);
        if (saveType_ == Steam)
            fs.write((const char*)slot2->md5hash, 16);
        fs.write((const char*)data.data(), (int)data.size());
    }
    {
        auto *slot2 = (SummarySlot*)slots_[summarySlot_].get();
        auto &data = slot2->data;
        slot2->userid = userid;
        *(uint64_t*)&data[4] = userid;
        fs.seekg(slot2->offset, std::ios::beg);
        md5Hash(data.data(), data.size(), slot2->md5hash);
        if (saveType_ == Steam)
            fs.write((const char*)slot2->md5hash, 16);
        fs.write((const char*)data.data(), (int)data.size());
    }
    fs.close();
    return true;
}

void SaveFile::patchSlotTime(int slot, uint32_t millisec) {
    {
        auto &s = slots_[slot];
        if (s->slotType != SaveSlot::Character) { return; }
        *(uint32_t *)(s->data.data() + 8) = millisec;
    }
    {
        auto &s = slots_[summarySlot_];
        *(uint16_t *)&s->data[0x195E + 0x24C * slot + 0x22 + 0x04] = millisec / 1000;
    }
}

bool SaveFile::exportTo(std::vector<uint8_t> &buf, int slot) {
    if (slot < 0 || slot >= slots_.size() || slots_[slot]->slotType != SaveSlot::Character) { return false; }
    const auto &data = slots_[slot]->data;
    buf.resize(data.size() + 0x24C);
    memcpy(buf.data(), data.data(), data.size());
    const auto &summary = slots_[summarySlot_]->data;
    memcpy(buf.data() + data.size(), summary.data() + 0x195E + 0x24C * slot, 0x24C);
    return true;
}

bool SaveFile::exportToFile(const std::wstring &filename, int slot) {
    std::vector<uint8_t> buf;
    if (!exportTo(buf, slot)) return false;
    std::ofstream ofs(std::filesystem::path(filename), std::ios::out | std::ios::binary);
    if (!ofs.is_open()) { return false; }
    ofs.write((const char*)buf.data(), (int)buf.size());
    ofs.close();
    return true;
}

bool SaveFile::importFrom(const std::vector<uint8_t> &buf, int slot, const std::function<void()>& beforeResign, bool keepFace) {
    if (slot < 0 || slot >= slots_.size() || slots_[slot]->slotType != SaveSlot::Character) { return false; }
    auto *slot2 = (CharSlot*)slots_[slot].get();
    auto &data = slot2->data;
    if (buf.size() != data.size() + 0x24C) { return false; }

    std::vector<uint8_t> face;
    if (keepFace) {
        if (!exportFace(face, slot)) face.clear();
    }
    memcpy(data.data(), buf.data(), data.size());

    slot2->useridOffset = 0;
    slot2->userid = 0;
    findUserIDOffset(slot2, [slot2](int offset) {
        slot2->useridOffset = offset;
        slot2->userid = *(uint64_t*)&slot2->data[offset];
    });

    auto *ndata = buf.data() + data.size();
    auto level = *(int*)(ndata + 0x22);
    const auto *name = (const wchar_t *)ndata;
    auto offset = findStatOffset(data, level, name);
    if (offset > 0) {
        slot2->statOffset = (int)offset;
        slot2->level = *(uint16_t*)(slot2->data.data() + offset + 0x2C);
        memcpy(slot2->stats, slot2->data.data() + offset, 8 * sizeof(uint32_t));
    } else {
        slot2->statOffset = 0;
        slot2->level = 0;
        memset(slot2->stats, 0, sizeof(slot2->stats));
    }

    auto *summary = (SummarySlot*)slots_[summarySlot_].get();
    summary->data[0x1954 + slot] = 1;
    summary->data[0x1334] = (int8_t)slot;
    memcpy(&summary->data[0x195E + 0x24C * slot], ndata, 0x24C);
    if (keepFace && !face.empty()) {
        importFace(face, slot, false);
    }
    if (beforeResign) {
        beforeResign();
    }
    resign(slot, summary->userid);
    return true;
}

bool SaveFile::importFromFile(const std::wstring &filename, int slot, const std::function<void()>& beforeResign, bool keepFace) {
    if (slot < 0 || slot >= slots_.size() || slots_[slot]->slotType != SaveSlot::Character) { return false; }
    std::ifstream ifs(std::filesystem::path(filename), std::ios::in | std::ios::binary);
    if (!ifs.is_open()) { return false; }
    ifs.seekg(0, std::ios::end);
    auto &data = slots_[slot]->data;
    if (ifs.tellg() != data.size() + 0x24C) { ifs.close(); return false; }
    ifs.seekg(0, std::ios::beg);
    std::vector<uint8_t> buf(data.size() + 0x24C);
    ifs.read((char*)buf.data(), (int)data.size() + 0x24C);
    ifs.close();
    return importFrom(buf, slot, beforeResign, keepFace);
}

bool SaveFile::exportFace(std::vector<uint8_t> &buf, int slot) {
    if (slot < 0 || slot >= slots_.size() || slots_[slot]->slotType != SaveSlot::Character) { return false; }
    const auto &data = slots_[slot]->data;
    const auto *m = data.data();
    auto offset = boyer_moore(m, data.size(), (const uint8_t *)"\x46\x41\x43\x45\x04\x00\x00\x00\x20\x01\x00\x00", 12);
    buf.assign(m + offset, m + offset + 0x120);
    const auto *s = (const CharSlot*)slots_[slot].get();
    if (s->statOffset > 0)
        buf.push_back(data[s->statOffset + 0x82]);
    else
        buf.push_back(255);
    return true;
}

bool SaveFile::exportFaceToFile(const std::wstring &filename, int slot) {
    std::vector<uint8_t> buf;
    if (!exportFace(buf, slot)) { return false; }
    std::ofstream ofs(std::filesystem::path(filename), std::ios::out | std::ios::binary);
    if (!ofs.is_open()) { return false; }
    ofs.write((const char*)buf.data(), (int)buf.size());
    ofs.close();
    return true;
}

bool SaveFile::importFace(const std::vector<uint8_t> &buf, int slot, bool resign) {
    if (slot < 0 || slot >= slots_.size() || slots_[slot]->slotType != SaveSlot::Character) { return false; }
    if (memcmp(buf.data(), "\x46\x41\x43\x45\x04\x00\x00\x00\x20\x01\x00\x00", 12) != 0) { return false; }
    auto sex = buf[0x120];
    if (sex == 0xFF) { return true; }
    auto *slot2 = (CharSlot*)slots_[slot].get();
    if (slot2->statOffset == 0) { return false; }
    auto &data = slot2->data;
    auto *m = data.data();
    auto offset = boyer_moore(m, data.size(), (const uint8_t *)"\x46\x41\x43\x45\x04\x00\x00\x00\x20\x01\x00\x00", 12);
    memcpy(m + offset, buf.data(), 0x120);
    slot2->data[slot2->statOffset + 0x82] = sex;
    auto &s = slots_[summarySlot_];
    memcpy(&s->data[0x195E + 0x24C * slot + 0x22 + 0x18], buf.data(), 0x120);
    s->data[0x195E + 0x24C * slot + 0x242] = sex;
    if (resign) {
        std::fstream fs(std::filesystem::path(filename_), std::ios::in | std::ios::out | std::ios::binary);
        if (!fs.is_open()) return false;
        md5Hash(s->data.data(), s->data.size(), s->md5hash);
        fs.seekg(s->offset, std::ios::beg);
        if (saveType_ == Steam)
            fs.write((const char *)s->md5hash, 16);
        fs.write((const char *)s->data.data(), (int)s->data.size());
        fs.seekg(slot2->offset, std::ios::beg);
        md5Hash(data.data(), data.size(), slot2->md5hash);
        if (saveType_ == Steam)
            fs.write((const char *)slot2->md5hash, 16);
        fs.write((const char *)data.data(), (int)data.size());
        fs.close();
    }
    return true;
}

bool SaveFile::importFaceFromFile(const std::wstring &filename, int slot, bool resign) {
    std::ifstream ifs(std::filesystem::path(filename), std::ios::in | std::ios::binary);
    if (!ifs.is_open()) { return false; }
    ifs.seekg(0, std::ios::end);
    if (ifs.tellg() != 0x120) { ifs.close(); return false; }
    std::vector<uint8_t> face(0x120);
    ifs.seekg(0, std::ios::beg);
    ifs.read((char*)face.data(), 0x120);
    ifs.close();

    return importFace(face, slot, resign);
}

void SaveFile::listSlots(int slot, const std::function<void(int, const SaveSlot&)> &func) {
#if defined(USE_CLI)
    fprintf(stdout, "SaveType: %s\n", saveType_ == Steam ? "Steam" : "PS4");
    if (saveType_ == Steam) {
        auto &s = slots_[summarySlot_];
        fprintf(stdout, "User ID:  %llu\n", ((SummarySlot *)s.get())->userid);
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
