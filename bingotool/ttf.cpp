/*
 * Heroes of Jin Yong.
 * A reimplementation of the DOS game `The legend of Jin Yong Heroes`.
 * Copyright (C) 2021, Soar Qin<soarchin@gmail.com>

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ttf.h"

#include "rectpacker.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"

#include <fstream>

namespace hojy::scene {

TTF::TTF(SDL_Renderer *renderer): renderer_(renderer), rectpacker_(new RectPacker(RectPackWidthDefault, RectPackWidthDefault)) {
}

TTF::~TTF() {
    deinit();
}

void TTF::init(int size, uint16_t width) {
    fontSize_ = size;
    monoWidth_ = width;
}

void TTF::deinit() {
    for (auto *tex: textures_) {
        SDL_DestroyTexture(tex);
    }
    textures_.clear();
    fontCache_.clear();
    for (auto &p: fonts_) {
        delete static_cast<stbtt_fontinfo *>(p.font);
        p.ttf_buffer.clear();
    }
    fonts_.clear();
}

bool TTF::add(const std::string &filename, int index) {
    FontInfo fi;
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) {
        return false;
    }
    ifs.seekg(0, std::ios::end);
    fi.ttf_buffer.resize(ifs.tellg());
    ifs.seekg(0, std::ios::beg);
    ifs.read(reinterpret_cast<char*>(&fi.ttf_buffer[0]), fi.ttf_buffer.size());
    auto *info = new stbtt_fontinfo;
    stbtt_InitFont(info, &fi.ttf_buffer[0], stbtt_GetFontOffsetForIndex(&fi.ttf_buffer[0], index));
    fi.font = info;
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(info, &ascent, &descent, &lineGap);
    fi.lineHeight = ascent + descent;
    fi.lineGap = lineGap;
    fonts_.emplace_back(std::move(fi));
    return true;
}

int TTF::charDimension(uint32_t ch, uint16_t &width, int16_t &t, int16_t &b, int fontSize) {
    if (fontSize < 0) fontSize = fontSize_;
    const FontData *fd;
    uint64_t key = (uint64_t(fontSize) << 32) | uint64_t(ch);
    auto ite = fontCache_.find(key);
    if (ite == fontCache_.end()) {
        fd = makeCache(ch, fontSize);
        if (!fd) {
            width = t = b = 0;
            return -1;
        }
    } else {
        fd = &ite->second;
        if (fd->advW == 0) {
            width = t = b = 0;
            return -1;
        }
    }
    if (monoWidth_)
        width = std::max(fd->advW, monoWidth_);
    else
        width = fd->advW;
    t = fd->iy0;
    b = fd->iy0 + fd->h;
    return fd->fontidx;
}

std::tuple<int, int> TTF::measureDimension(const std::wstring &str, int fontSize) {
    uint16_t w;
    int16_t mt = 0, mb = 0;
    int16_t t, b;
    int res = 0;
    for (auto &ch: str) {
        if (ch < 32) { continue; }
        charDimension(ch, w, t, b, fontSize);
        if (t < mt) mt = t;
        if (b > mb) mb = b;
        res += int(uint32_t(w));
    }
    return std::make_tuple(res, mb - mt);
}

std::tuple<int, int> TTF::measureDimension(const std::wstring &str, int fontSize, int maxWidth, int lineSpacing) {
    uint16_t w;
    int16_t t, b;
    int maxw = 0, linew = 0, maxlineh = 0, totalh = 0;
    if (fontSize < 0) fontSize = fontSize_;
    for (auto &ch: str) {
        if (ch < 32) continue;
        auto fidx = charDimension(ch, w, t, b, fontSize);
        if (fidx < 0) continue;

        auto &info = fonts_[fidx];
        float fontScale = stbtt_ScaleForMappingEmToPixels(static_cast<stbtt_fontinfo*>(info.font), float(fontSize));
        auto lineh = std::lround(fontScale * (float)(info.lineHeight + info.lineGap)) + lineSpacing;
        if (lineh > maxlineh) maxlineh = lineh;
        auto ww = int(uint32_t(w));
        if (linew + ww > maxWidth) {
            if (linew > maxw) maxw = linew;
            totalh += maxlineh;
            linew = 0;
            maxlineh = lineh;
        } else {
            linew += ww;
        }
    }
    if (linew > maxw) maxw = linew;
    return std::make_tuple(maxw, totalh + maxlineh);
}

void TTF::setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    altR_[0] = r; altG_[0] = g; altB_[0] = b; altA_[0] = a;
}

void TTF::setAltColor(int index, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (index > 0 && index <= 16) {
        --index;
        altR_[index] = r;
        altG_[index] = g;
        altB_[index] = b;
        altA_[index] = a;
    }
}

void TTF::setShadowColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    shadowR_ = r; shadowG_ = g; shadowB_ = b; shadowA_ = a;
}

void TTF::render(std::wstring_view str, int x, int y, bool shadow, int fontSize) {
    if (fontSize < 0) fontSize = fontSize_;
    int colorIndex = 0;
    for (auto ch: str) {
        if (ch > 0 && ch < 17) { colorIndex = ch - 1; continue; }
        const FontData *fd;
        uint64_t key = (uint64_t(fontSize) << 32) | uint64_t(ch);
        auto ite = fontCache_.find(key);
        if (ite == fontCache_.end()) {
            fd = makeCache(ch, fontSize);
            if (!fd) {
                continue;
            }
        } else {
            fd = &ite->second;
            if (fd->advW == 0) continue;
        }
        auto *tex = textures_[fd->rpidx];
        SDL_FRect rcDest;
        SDL_FRect rcSrc = {(float)(fd->rpx), (float)(fd->rpy), (float)(fd->w), (float)(fd->h)};
        if (shadow) {
            SDL_SetTextureColorMod(tex, shadowR_, shadowG_, shadowB_);
            SDL_SetTextureAlphaMod(tex, shadowA_);
            rcDest = {(float)(x + fd->ix0 + 2), (float)(y + fd->iy0 + 2), (float)(fd->w), (float)(fd->h)};
            SDL_RenderTexture(renderer_, tex, &rcSrc, &rcDest);
        }
        SDL_SetTextureColorMod(tex, altR_[colorIndex], altG_[colorIndex], altB_[colorIndex]);
        SDL_SetTextureAlphaMod(tex, altA_[colorIndex]);
        rcDest = {(float)(x + fd->ix0), (float)(y + fd->iy0), (float)(fd->w), (float)(fd->h)};
        SDL_RenderTexture(renderer_, tex, &rcSrc, &rcDest);
        x += fd->advW;
    }
}

const TTF::FontData *TTF::makeCache(uint32_t ch, int fontSize) {
    if (fontSize < 0) fontSize = fontSize_;
    FontInfo *fi = nullptr;
    stbtt_fontinfo *info;
    int index = 0;
    size_t sz = fonts_.size();
    size_t fidx = 0;
    for (fidx = 0; fidx < sz; fidx++) {
        auto &f = fonts_[fidx];
        info = static_cast<stbtt_fontinfo*>(f.font);
        index = stbtt_FindGlyphIndex(info, ch);
        if (index != 0) { fi = &f; break; }
    }
    uint64_t key = (uint64_t(fontSize) << 32) | uint64_t(ch);
    FontData *fd = &fontCache_[key];
    if (fi == nullptr) {
        memset(fd, 0, sizeof(FontData));
        return nullptr;
    }

    fd->fontidx = int(fidx);
    /* Read font data to cache */
    int advW, leftB;
    float fontScale = stbtt_ScaleForMappingEmToPixels(info, static_cast<float>(fontSize));
    stbtt_GetGlyphHMetrics(info, index, &advW, &leftB);
    int ascent, descent;
    stbtt_GetFontVMetrics(info, &ascent, &descent, nullptr);
    fd->advW = uint16_t(std::lround(fontScale * float(advW)));
    int w, h, x, y;
    stbtt_GetGlyphBitmap(info, fontScale, fontScale, index, &w, &h, &x, &y);
    fd->ix0 = int16_t(x);
    fd->iy0 = int16_t(std::lround(fontScale * (float)(ascent + descent)) + y);
    fd->w = w;
    fd->h = h;

    int dstPitch = int((fd->w + 1u) & ~1u);
    /* Get last rect pack bitmap */
    auto rpidx = rectpacker_->pack(dstPitch, fd->h, fd->rpx, fd->rpy);
    if (rpidx < 0) {
        memset(fd, 0, sizeof(FontData));
        return nullptr;
    }
    // stbrp_rect rc = {0, uint16_t((fd->w + 3u) & ~3u), fd->h};
    fd->rpidx = rpidx;

    uint8_t dst[64 * 64];

    stbtt_MakeGlyphBitmapSubpixel(info, dst, fd->w, fd->h, dstPitch, fontScale, fontScale, 0, 0, index);

    if (rpidx >= textures_.size()) {
        textures_.resize(rpidx + 1, nullptr);
    }
    auto *tex = textures_[rpidx];
    if (tex == nullptr) {
        tex = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, RectPackWidthDefault, RectPackWidthDefault);
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        textures_[rpidx] = tex;
    }
    int pitch;
    SDL_Rect rc = {fd->rpx, fd->rpy, dstPitch, fd->h};
    uint32_t *pixels = nullptr;
    if (SDL_LockTexture(tex, &rc, (void**)&pixels, &pitch) == 0) {
        auto *pdst = dst;
        int offset = pitch - dstPitch;
        int hh = fd->h;
        while (hh--) {
            int ww = dstPitch;
            while (ww--) {
                *pixels++ = 0xFFFFFFu | (uint32_t(*pdst++) << 24);
            }
            pixels += offset;
        }
        SDL_UnlockTexture(tex);
    }
    return fd;
}

}
