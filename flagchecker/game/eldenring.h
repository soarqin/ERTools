#pragma once

#include <cstdint>

namespace flagchecker::game {

class EldenRing {
public:
    bool searchForGame();
    [[nodiscard]] bool running() const;
    [[nodiscard]] bool resolved() const {
        return eventFlagMan_ != 0;
    }

    uintptr_t scan(const char *binary) const;
    uintptr_t scanWithJump(const char *binary, int jmpInstrOffset, int jmpInstrLength) const {
        uintptr_t addr = scan(binary);
        if (addr == 0)
            return 0;
        return addr + read<int32_t>(baseAddress_ + addr + jmpInstrOffset) + jmpInstrLength;
    }
    size_t read(uintptr_t address, void *buffer, size_t size) const;
    size_t readRelative(uintptr_t address, void *buffer, size_t size) const {
        return read(baseAddress_ + address, buffer, size);
    }
    template<typename T>
    [[nodiscard]] T read(uintptr_t address) const {
        T buffer;
        if (read(address, &buffer, sizeof(T)) < sizeof(T))
            return T();
        return buffer;
    }
    [[nodiscard]] bool readFlag(uint32_t flagId) const;

private:
    void *handle_ = nullptr;
    uintptr_t baseAddress_ = 0;
    size_t size_ = 0;
    uintptr_t eventFlagMan_ = 0;
};

extern EldenRing gEldenRing;

}