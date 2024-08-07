#include "eldenring.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <psapi.h>
#include <tlhelp32.h>

#include <vector>
#include <cstdio>

namespace flagchecker::game {

EldenRing gEldenRing;

DWORD getProcessByName(const wchar_t *process_name) {
    PROCESSENTRY32W proc_entry = {.dwSize = sizeof(PROCESSENTRY32W)};

    HANDLE proc_list = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (proc_list == INVALID_HANDLE_VALUE)
        return -1;

    if (!Process32FirstW(proc_list, &proc_entry))
        return -1;

    do {
        if (lstrcmpiW(proc_entry.szExeFile, process_name) == 0)
            return proc_entry.th32ProcessID;
    } while (Process32NextW(proc_list, &proc_entry));

    return -1;
}

bool EldenRing::searchForGame() {
    handle_ = nullptr;
    auto window = FindWindowW(L"ELDEN RING™", nullptr);
    DWORD processId = 0;
    if (GetWindowThreadProcessId(window, &processId) == 0) {
        processId = getProcessByName(L"eldenring.exe");
    }
    if (processId == 0)
        return false;
    handle_ = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (handle_ == nullptr)
        return false;
    HMODULE module;
    DWORD cb;
    if (!EnumProcessModules(handle_, &module, sizeof(HMODULE), &cb))
        return false;
    baseAddress_ = reinterpret_cast<uintptr_t>(module);
    MODULEINFO moduleInfo;
    if (!GetModuleInformation(handle_, module, &moduleInfo, sizeof(MODULEINFO)))
        return false;
    size_ = moduleInfo.SizeOfImage;
    eventFlagMan_ = game::gEldenRing.scanWithJump("48 8B 3D ?? ?? ?? ?? 48 85 FF ?? ?? 32 C0 E9", 3, 7);
    return true;
}

bool EldenRing::running() const {
    if (handle_ == nullptr)
        return false;
    DWORD exitCode;
    if (!GetExitCodeProcess(handle_, &exitCode))
        return false;
    return exitCode == STILL_ACTIVE;
}

uintptr_t EldenRing::scan(const char *binary) const {
    alignas(64) constexpr const int8_t hexLookup[128] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1,
        -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    };
    uint8_t curr = 0;
    std::vector<uint8_t> bytes;
    std::vector<uint8_t> masks;
    int counter = 0;
    bool lastIsMask = false;
    for (const auto *c = binary; *c; c++) {
        auto ch = *c;
        if (ch < 0) continue;
        auto n = hexLookup[ch];
        if (n < 0) {
            switch (ch) {
                case ' ':
                    if (counter > 0) {
                        if (lastIsMask) {
                            bytes.emplace_back(0);
                            masks.emplace_back(0);
                        } else {
                            bytes.emplace_back(curr);
                            masks.emplace_back(0xFF);
                        }
                        lastIsMask = false;
                        curr = 0;
                        counter = 0;
                    }
                    break;
                case '?':
                case 'x':
                case 'X':
                    if (counter > 0 && !lastIsMask) {
                        bytes.emplace_back(curr);
                        masks.emplace_back(0xFF);
                        curr = 0;
                        counter = 0;
                    }
                    lastIsMask = true;
                    if (++counter == 2) {
                        bytes.emplace_back(0);
                        masks.emplace_back(0);
                        lastIsMask = false;
                        curr = 0;
                        counter = 0;
                    }
                    break;
            }
            continue;
        }
        if (counter > 0 && lastIsMask) {
            bytes.emplace_back(0);
            masks.emplace_back(0);
            curr = 0;
            counter = 0;
            lastIsMask = false;
        }
        curr = (curr << 4) | n;
        if (++counter == 2) {
            bytes.emplace_back(curr);
            masks.emplace_back(0xFF);
            lastIsMask = false;
            curr = 0;
            counter = 0;
        }
    }

    constexpr size_t blockSize = 0x10000;
    uint8_t block[blockSize + 256];
    size_t bytesSize = bytes.size();
    size_t readSize = blockSize + bytesSize - 1;
    for (uintptr_t curStart = 0; curStart < size_; curStart += blockSize) {
        size_t compareSize = readRelative(curStart, block, readSize);
        if (compareSize < bytesSize) break;
        compareSize = compareSize - bytesSize + 1;
        for (size_t i = 0; i <  compareSize; i++) {
            bool found = true;
            size_t offset = i;
            for (size_t j = 0; j < bytesSize; j++) {
                if ((block[offset++] & masks[j]) != bytes[j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return curStart + i;
            }
        }
    }
    return 0;
}

size_t EldenRing::read(uintptr_t address, void *buffer, size_t size) const {
    SIZE_T sizeRead = 0;
    if (!ReadProcessMemory(handle_, reinterpret_cast<LPCVOID>(address), buffer, (SIZE_T)size, &sizeRead))
        return 0;
    return sizeRead;
}

bool EldenRing::readFlag(uint32_t flagId) const {
    if (eventFlagMan_ == 0)
        return false;
    auto addr = read<uint64_t>(baseAddress_ + eventFlagMan_);
    if (addr == 0)
        return false;
    auto divisor = read<uint32_t>(addr + 0x1C);
    auto category = flagId / divisor;
    auto leastSignificantDigits = flagId - category * divisor;
    auto currentElement = read<uint64_t>(addr + 0x38);
    auto currentSubElement = read<uint64_t>(currentElement + 0x08);
    while (read<uint8_t>(currentSubElement + 0x19) == 0) {
        if (read<uint32_t>(currentSubElement + 0x20) < category) {
            currentSubElement = read<uint64_t>(currentSubElement + 0x10);
        } else {
            currentElement = currentSubElement;
            currentSubElement = read<uint64_t>(currentSubElement);
        }
    }
    if (currentElement == currentSubElement) {
        return false;
    }
    auto mysteryValue = read<uint32_t>(currentElement + 0x28) - 1;
    uint64_t calculatedPointer;
    if (mysteryValue != 0) {
        if (mysteryValue != 1) {
            calculatedPointer = read<uint64_t>(currentElement + 0x30);
        } else {
            return false;
        }
    } else {
        calculatedPointer = read<uint32_t>(addr + 0x20) * read<uint32_t>(currentElement + 0x30);
        calculatedPointer += read<uint64_t>(addr + 0x28);
    }
    if (calculatedPointer == 0) {
        return false;
    }
    auto thing = 7 - (leastSignificantDigits & 7);
    auto anotherThing = 1 << thing;
    auto shifted = leastSignificantDigits >> 3;
    return (read<uint8_t>(calculatedPointer + shifted) & anotherThing) != 0;
}

}
