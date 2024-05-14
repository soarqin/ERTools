/*
 * Copyright (c) 2022 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <istream>
#include <cstdint>

struct SaveSlot {
    enum SlotType {
        Character,
        Summary,
        Other,
    };
    SlotType slotType;
    uint32_t offset = 0;
    std::vector<uint8_t> data;
    uint8_t md5hash[16] = {};
    std::wstring filename;
};

struct CharSlot: SaveSlot {
    std::wstring charname;
    uint16_t level = 0;
    uint32_t stats[8] = {};
    int useridOffset = 0;
    uint64_t userid = 0;
    bool available = false;
};

struct SummarySlot: SaveSlot {
    uint64_t userid = 0;
};

class SaveFile {
public:
    enum SaveType {
        Steam,
        PS4,
    };
    explicit SaveFile(const std::wstring &filename);
    explicit SaveFile(const std::string &data, const std::wstring &saveAs);
    void load(std::istream &stream);
    void resign(uint64_t userid);
    void patchSlotTime(int slot, uint32_t millisec);
    [[nodiscard]] inline bool ok() const { return ok_; }
    bool exportToFile(const std::wstring &filename, int slot);
    bool importFromFile(const std::wstring &filename, int slot);
    bool exportFaceToFile(const std::wstring &filename, int slot);
    bool importFaceFromFile(const std::wstring &filename, int slot);
    void listSlots(int slot = -1, const std::function<void(int, const SaveSlot&)> &func = nullptr);
    void fixHashes();
    bool verifyHashes();
    [[nodiscard]] inline SaveType saveType() const { return saveType_; }
    [[nodiscard]] size_t count() const { return slots_.size(); }
    [[nodiscard]] const SaveSlot &slot(size_t index) const { return *slots_[index]; }

private:
    void listSlot(int slot);

private:
    bool ok_ = false;
    std::wstring filename_;
    SaveType saveType_ = Steam;
    std::vector<uint8_t> header_;
    std::vector<std::unique_ptr<SaveSlot>> slots_;
};
