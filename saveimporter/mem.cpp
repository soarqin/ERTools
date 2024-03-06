/*
 * Copyright (c) 2022 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "mem.h"

#if defined(__AVX2__)
#include "md5_simd.h"
#else
#include "md5.h"
#endif
#if defined(_MSC_VER)
#include <malloc.h>
#endif

void kmpSearch(const void *full, size_t fullLen, const void *sub, size_t subLen, void (*func)(int, void*), void *userp) {
    int i, j, len;
    const auto *haystack = (const uint8_t *)full;
    const auto *needle = (const uint8_t *)sub;

#if defined(_MSC_VER)
    int *lpsTable = static_cast<int*>(_alloca(subLen * sizeof(int)));
#else
    int lpsTable[subLen];
#endif
    lpsTable[0] = 0;
    len = 0;
    i = 1;
    while (i < subLen) {
        if (needle[i] == needle[len]) {
            lpsTable[i++] = ++len;
        } else {
            if (len == 0) {
                lpsTable[i++] = 0;
            } else {
                len = lpsTable[len - 1];
            }
        }
    }

    i = 0;
    j = 0;
    while (i <= fullLen - subLen) {
        while (j < subLen) {
            if (haystack[i + j] != needle[j]) {
                break;
            }
            j++;
        }
        if (j == 0) {
            i++;
        } else if (j < subLen) {
            i += j - lpsTable[j - 1];
            j = lpsTable[j - 1];
        } else {
            func(i, userp);
            i += j - lpsTable[j - 1];
            j = lpsTable[j - 1];
        }
    }
}

void md5Hash(const void *buf, size_t len, uint8_t output[16]) {
#if defined(__AVX2__)
    md5_simd::MD5_SIMD md5;
    const auto *n = (const char*)buf;
    uint64_t l = len;
    md5.calculate<1>(&n, &l);
    md5.binarydigest(output, 0);
#else
    md5(buf, len, output);
#endif
}
