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
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#else
#include <sys/types.h>
#endif

void kmp_search(const void *full, size_t fullLen, const void *sub, size_t subLen, bool (*func)(int, void *), void *userp) {
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
            if (func(i, userp)) return;
            i += j - lpsTable[j - 1];
            j = lpsTable[j - 1];
        }
    }
}

void md5Hash(const void *buf, size_t len, uint8_t output[16]) {
#if defined(__AVX2__)
    md5_simd::MD5_SIMD md5;
    const auto *n = (const char *)buf;
    uint64_t l = len;
    md5.calculate<1>(&n, &l);
    md5.binarydigest(output, 0);
#else
    md5(buf, len, output);
#endif
}

#define ALPHABET_LEN 256
#if !defined(max)
#define max(a, b) (((a) < (b)) ? (b) : (a))
#endif

// BAD-CHARACTER RULE.
// delta1 table: delta1[c] contains the distance between the last
// character of pat and the rightmost occurrence of c in pat.
//
// If c does not occur in pat, then delta1[c] = patlen.
// If c is at string[i] and c != pat[patlen-1], we can safely shift i
//   over by delta1[c], which is the minimum distance needed to shift
//   pat forward to get string[i] lined up with some character in pat.
// c == pat[patlen-1] returning zero is only a concern for BMH, which
//   does not have delta2. BMH makes the value patlen in such a case.
//   We follow this choice instead of the original 0 because it skips
//   more. (correctness?)
//
// This algorithm runs in alphabet_len+patlen time.
void make_delta1(ptrdiff_t *delta1, const uint8_t *pat, size_t patlen) {
    for (int i = 0; i < ALPHABET_LEN; i++) {
        delta1[i] = patlen;
    }
    for (int i = 0; i < patlen; i++) {
        delta1[pat[i]] = patlen - 1 - i;
    }
}

// true if the suffix of word starting from word[pos] is a prefix
// of word
bool is_prefix(const uint8_t *word, size_t wordlen, ptrdiff_t pos) {
    int suffixlen = wordlen - pos;
    // could also use the strncmp() library function here
    // return ! strncmp(word, &word[pos], suffixlen);
    for (int i = 0; i < suffixlen; i++) {
        if (word[i] != word[pos + i]) {
            return false;
        }
    }
    return true;
}

// length of the longest suffix of word ending on word[pos].
// suffix_length("dddbcabc", 8, 4) = 2
size_t suffix_length(const uint8_t *word, size_t wordlen, ptrdiff_t pos) {
    size_t i;
    // increment suffix length i to the first mismatch or beginning
    // of the word
    for (i = 0; (word[pos - i] == word[wordlen - 1 - i]) && (i <= pos); i++);
    return i;
}

// GOOD-SUFFIX RULE.
// delta2 table: given a mismatch at pat[pos], we want to align
// with the next possible full match could be based on what we
// know about pat[pos+1] to pat[patlen-1].
//
// In case 1:
// pat[pos+1] to pat[patlen-1] does not occur elsewhere in pat,
// the next plausible match starts at or after the mismatch.
// If, within the substring pat[pos+1 .. patlen-1], lies a prefix
// of pat, the next plausible match is here (if there are multiple
// prefixes in the substring, pick the longest). Otherwise, the
// next plausible match starts past the character aligned with
// pat[patlen-1].
//
// In case 2:
// pat[pos+1] to pat[patlen-1] does occur elsewhere in pat. The
// mismatch tells us that we are not looking at the end of a match.
// We may, however, be looking at the middle of a match.
//
// The first loop, which takes care of case 1, is analogous to
// the KMP table, adapted for a 'backwards' scan order with the
// additional restriction that the substrings it considers as
// potential prefixes are all suffixes. In the worst case scenario
// pat consists of the same letter repeated, so every suffix is
// a prefix. This loop alone is not sufficient, however:
// Suppose that pat is "ABYXCDBYX", and text is ".....ABYXCDEYX".
// We will match X, Y, and find B != E. There is no prefix of pat
// in the suffix "YX", so the first loop tells us to skip forward
// by 9 characters.
// Although superficially similar to the KMP table, the KMP table
// relies on information about the beginning of the partial match
// that the BM algorithm does not have.
//
// The second loop addresses case 2. Since suffix_length may not be
// unique, we want to take the minimum value, which will tell us
// how far away the closest potential match is.
void make_delta2(ptrdiff_t *delta2, const uint8_t *pat, size_t patlen) {
    ssize_t p;
    size_t last_prefix_index = 1;

    // first loop
    for (p = patlen - 1; p >= 0; p--) {
        if (is_prefix(pat, patlen, p + 1)) {
            last_prefix_index = p + 1;
        }
        delta2[p] = last_prefix_index + (patlen - 1 - p);
    }

    // second loop
    for (p = 0; p < patlen - 1; p++) {
        size_t slen = suffix_length(pat, patlen, p);
        if (pat[p - slen] != pat[patlen - 1 - slen]) {
            delta2[patlen - 1 - slen] = patlen - 1 - p + slen;
        }
    }
}

// Returns pointer to first match.
// See also glibc memmem() (non-BM) and std::boyer_moore_searcher (first-match).
size_t boyer_moore(const uint8_t *string, size_t stringlen, const uint8_t *pat, size_t patlen) {
    ptrdiff_t delta1[ALPHABET_LEN];
#if defined(_MSC_VER)
    ptrdiff_t *delta2 = (ptrdiff_t *)alloca(patlen * sizeof(ptrdiff_t));
#else
    ptrdiff_t delta2[patlen]; // C99 VLA
#endif
    make_delta1(delta1, pat, patlen);
    make_delta2(delta2, pat, patlen);

    // The empty pattern must be considered specially
    if (patlen == 0) {
        return 0;
    }

    size_t i = patlen - 1;        // str-idx
    while (i < stringlen) {
        ptrdiff_t j = patlen - 1; // pat-idx
        while (j >= 0 && (string[i] == pat[j])) {
            --i;
            --j;
        }
        if (j < 0) {
            return i + 1;
        }

        ptrdiff_t shift = max(delta1[string[i]], delta2[j]);
        i += shift;
    }
    return (size_t)-1;
}
