/*
 * Copyright (C) 2024, Soar Qin<soarchin@gmail.com>

 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#pragma once

#include "memory.h"
#include <Pattern16.h>

namespace er::util {

class Signature {
public:
    /**
     * \brief Constructs the signature with an IDA pattern
     * \param pattern The IDA pattern string
     */
    explicit Signature(const char *pattern) : pattern_(pattern) {
    }

    /**
     * \brief Scans for the pattern in a memory region
     * \param region The region to search in, default is the main module
     * \return MemoryHandle
     */
    MemoryHandle scan(MemoryRegion region = Module(nullptr)) {
        return Pattern16::scan(region.base().as<void *>(), region.size(), pattern_);
    }
private:
    const char *pattern_;
};

}