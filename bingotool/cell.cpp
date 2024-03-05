#include "cell.h"

#include "common.h"
#include "config.h"

#include <SDL3_gfxPrimitives.h>

extern SDL_Window *gWindow;
Cells gCells;

void Cell::updateTexture() {
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
    if (text.empty()) {
        w = h = 0;
        return;
    }
    auto *font = gConfig.font;
    if (false) {
        auto fontSize = gConfig.fontSize;
        auto maxHeight = gConfig.cellSize[1] * 9 / 10;
        while (true) {
            if (calculateMinimalHeight(font) <= maxHeight) {
                auto *surface =
                    TTF_RenderUTF8_BlackOutline_Wrapped(font,
                                                        text.c_str(),
                                                        &gConfig.textColor,
                                                        gConfig.cellSize[0] * 9 / 10,
                                                        &gConfig.textShadowColor,
                                                        gConfig.textShadow,
                                                        gConfig.textShadowOffset);
                texture = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_DestroySurface(surface);
                break;
            }
            fontSize--;
            font = TTF_OpenFont(gConfig.fontFile.c_str(), fontSize);
            TTF_SetFontStyle(font, gConfig.fontStyle);
            TTF_SetFontWrappedAlign(font, TTF_WRAPPED_ALIGN_CENTER);
        }
        if (font != gConfig.font) {
            TTF_CloseFont(font);
        }
    } else {
        auto *surface =
            TTF_RenderUTF8_BlackOutline_Wrapped(font, text.c_str(), &gConfig.textColor, gConfig.cellSize[0] * 9 / 10, &gConfig.textShadowColor, gConfig.textShadow, gConfig.textShadowOffset);
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
}

void Cell::render(int x, int y, int cx, int cy) const {
    bool dbl = status > 2;
    auto fx = (float)x, fy = (float)y, fcx = (float)cx, fcy = (float)cy;
    auto st = dbl ? (status - 2) : status;
    if (st > 0 && gConfig.colorTexture[st - 1]) {
        SDL_FRect dstrect = {fx, fy, fcx, fcy};
        SDL_RenderTexture(renderer, gConfig.colorTexture[st - 1], nullptr, &dstrect);
    } else {
        auto &col = gConfig.colorsInt[st];
        if (gConfig.cellRoundCorner > 0) {
            roundedBoxRGBA(renderer, x, y, x + cx - 1, y + cy - 1, gConfig.cellRoundCorner, col.r, col.g, col.b, col.a);
        } else {
            SDL_FRect dstrect = {fx, fy, fcx, fcy};
            SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);
            SDL_RenderFillRect(renderer, &dstrect);
        }
    }
    if (dbl) {
        auto &color = gConfig.colors[5 - status];
        const SDL_Vertex verts[] = {
            {SDL_FPoint{fx, fy + fcy}, color, SDL_FPoint{0},},
            {SDL_FPoint{fx, fy + fcy * .7f}, color, SDL_FPoint{0},},
            {SDL_FPoint{fx + fcx * .3f, fy + fcy}, color, SDL_FPoint{0},},
        };
        SDL_RenderGeometry(renderer, nullptr, verts, 3, nullptr, 0);
    }
    if (texture) {
        auto fw = (float)w, fh = (float)h;
        auto l = (float)(cx - w) * .5f, t = (float)(cy - h) * .5f;
        SDL_FRect rect = {fx + l, fy + t, fw, fh};
        SDL_RenderTexture(renderer, texture, nullptr, &rect);
    }
}

int Cell::calculateMinimalWidth(TTF_Font *font) {
    auto maxHeight = gConfig.cellSize[1] * 9 / 10;
    auto width = gConfig.cellSize[0] & ~1;
    size_t len = text.length();
    int tw, th;
    TTF_SizeText(font, text.c_str(), &tw, &th);
    th = std::max(th, TTF_FontHeight(font));
    while (true) {
        auto maxWidth = width * 9 / 10;
        size_t index = 0;
        int lines = 0;
        while (index < len) {
            int count;
            if (TTF_MeasureUTF8(font, text.c_str() + index, maxWidth, nullptr, &count) < 0) break;
            if (count == 0) break;
            lines++;
            while (count > 0 && index < len) {
                auto c = uint8_t(text[index]);
                if (c < 0x80) index++;
                else if (c < 0xE0) index += 2;
                else if (c < 0xF0) index += 3;
                else if (c < 0xF8) index += 4;
                else if (c < 0xFC) index += 5;
                else index += 6;
                count--;
            }
        }
        auto currHeight = lines * th;
        if (currHeight <= maxHeight) break;
        width += 2;
    }
    return width;
}

int Cell::calculateMinimalHeight(TTF_Font *font) {
    auto maxWidth = gConfig.cellSize[0] * 9 / 10;
    size_t index = 0;
    size_t len = text.length();
    int lines = 0;
    while (index < len) {
        int count;
        if (TTF_MeasureUTF8(font, text.c_str() + index, maxWidth, nullptr, &count) < 0) break;
        if (count == 0) break;
        lines++;
        while (count > 0 && index < len) {
            auto c = uint8_t(text[index]);
            if (c < 0x80) index++;
            else if (c < 0xE0) index += 2;
            else if (c < 0xF0) index += 3;
            else if (c < 0xF8) index += 4;
            else if (c < 0xFC) index += 5;
            else index += 6;
            count--;
        }
    }
    int tw, th;
    TTF_SizeText(font, text.c_str(), &tw, &th);
    return lines * std::max(th, TTF_FontHeight(font));
}

void Cells::init(SDL_Renderer *renderer) {
    fitCellsForText();
    for (auto &r : cells_) {
        for (auto &c : r) {
            c.renderer = renderer;
            c.updateTexture();
        }
    }
}

void Cells::fitCellsForText() {
    int minWidth = gConfig.cellSize[0];
    foreach([&minWidth](Cell &cell, int, int) {
        int w = cell.calculateMinimalWidth(gConfig.font);
        if (w > minWidth) minWidth = w;
    });
    if (minWidth > gConfig.cellSize[0]) {
        gConfig.cellSize[0] = minWidth;
        SDL_SetWindowSize(gWindow,
                          gConfig.cellSize[0] * 5 + gConfig.cellSpacing * 4 + gConfig.cellBorder * 2,
                          gConfig.cellSize[1] * 5 + gConfig.cellSpacing * 4 + gConfig.cellBorder * 2);
        SDL_SetWindowPosition(gWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
}

void Cells::foreach(const std::function<void(Cell &, int, int)> &callback) {
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            callback(cells_[y][x], x, y);
        }
    }
}
