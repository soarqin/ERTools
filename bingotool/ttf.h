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

#pragma once

#include <SDL3/SDL.h>

#include <string>
#include <string_view>
#include <unordered_map>
#include <tuple>
#include <vector>
#include <memory>
#include <cstdint>

namespace hojy::scene {

class RectPacker;

class TTF final {
protected:
    struct FontData {
        int fontidx;
        int16_t rpx, rpy;
        size_t rpidx;

        int16_t ix0, iy0;
        uint16_t w, h;
        uint16_t advW;
    };
    struct FontInfo {
        void *font = nullptr;
        std::vector<uint8_t> ttf_buffer;
        int lineHeight;
        int lineGap;
    };
public:
    explicit TTF(SDL_Renderer *renderer);
    ~TTF();

    void init(int size, uint16_t width = 0);
    void deinit();
    bool add(const std::string& filename, int index = 0);
    int charDimension(uint32_t ch, uint16_t &width, int16_t &t, int16_t &b, int fontSize = -1);
    std::tuple<int, int> measureDimension(const std::wstring &str, int fontSize = -1);
    std::tuple<int, int> measureDimension(const std::wstring &str, int fontSize, int maxWidth, int lineSpacing = 0);

    inline int fontSize() const { return fontSize_; }
    void setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void setAltColor(int index, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void setShadowColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

    void render(std::wstring_view str, int x, int y, bool shadow, int fontSize = -1);

private:
    const FontData *makeCache(uint32_t ch, int fontSize = - 1);

protected:
    int fontSize_ = 16;
    std::vector<FontInfo> fonts_;
    std::unordered_map<uint64_t, FontData> fontCache_;
    uint16_t monoWidth_ = 0;

private:
    SDL_Renderer *renderer_;

    uint8_t altR_[16] = {255}, altG_[16] = {255}, altB_[16] = {255}, altA_[16] = {255};
    uint8_t shadowR_ = 0, shadowG_ = 0, shadowB_ = 0, shadowA_ = 255;
    std::vector<SDL_Texture*> textures_;

    std::unique_ptr<RectPacker> rectpacker_;
};

}
