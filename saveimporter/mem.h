/*
 * Copyright (c) 2022 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#pragma once

#include <cstdint>

void kmpSearch(const void *full, size_t fullLen, const void *sub, size_t subLen, bool(*func)(int, void *), void *userp);
void md5Hash(const void *buf, size_t len, uint8_t output[16]);
