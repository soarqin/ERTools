/*
 * Copyright (C) 2024, Soar Qin<soarchin@gmail.com>

 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "gamehook.h"

#include "util/signature.h"
#include <MinHook.h>
#include <fmt/xchar.h>

namespace er::modloader {

template<typename T>
struct StringImpl {
    void *unk;
    T *string;
    void *unk2;
    uint64_t length;
    uint64_t capacity;

    T *str() {
        if (sizeof(T) * capacity >= 15) {
            return string;
        }

        return (T *)&string;
    }
};

void* __cdecl mapArchivePath(StringImpl<wchar_t>* path, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5, uint64_t p6);

decltype(&mapArchivePath) oldMapArchivePath = nullptr;

void* __cdecl mapArchivePath(StringImpl<wchar_t>* path, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5, uint64_t p6) {
    auto res = oldMapArchivePath(path, p2, p3, p4, p5, p6);
    if (path != nullptr) {
        fmt::print(L"mapArchivePath: {}\n", path->str());
    }
    return res;
}

bool GameHook::install() {
    MH_Initialize();
    auto addr = util::Signature("e8 ?? ?? ?? ?? 48 83 7b 20 08 48 8d 4b 08 72 03 48 8b 09 4c 8b 4b 18 41 b8 05 00 00 00 4d 3b c8").scan();
    if (!addr) {
        return false;
    }
    auto addr2 = addr.add(addr.add(1).as<uint32_t &>() + 5);
    if (addr2.as<uint8_t &>() == 0xE9) {
        addr2 = addr2.add(addr2.add(1).as<uint32_t &>() + 5);
    }
    MH_CreateHook(addr2.as<void*>(), (void*)&mapArchivePath, (void**)&oldMapArchivePath);
    MH_EnableHook(MH_ALL_HOOKS);
    return true;
}

void GameHook::uninstall() {
    MH_DisableHook(MH_ALL_HOOKS);
}

}
