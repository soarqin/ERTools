#include "cell.h"

#include "common.h"
#include "config.h"

#include <SDL3_gfxPrimitives.h>

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
    auto fontSize = gConfig.fontSize;
    while (true) {
        auto wrapLength = gConfig.cellSize[0] * 9 / 10;
        auto *surface =
            TTF_RenderUTF8_BlackOutline_Wrapped(font, text.c_str(), &gConfig.textColor, wrapLength, &gConfig.textShadowColor, gConfig.textShadow, gConfig.textShadowOffset);
        if (surface->h <= gConfig.cellSize[1] * 9 / 10) {
            texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_DestroySurface(surface);
            break;
        }
        SDL_DestroySurface(surface);
        fontSize--;
        font = TTF_OpenFont(gConfig.fontFile.c_str(), fontSize);
        TTF_SetFontStyle(font, gConfig.fontStyle);
        TTF_SetFontWrappedAlign(font, TTF_WRAPPED_ALIGN_CENTER);
    }
    if (font != gConfig.font) {
        TTF_CloseFont(font);
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
}

void Cell::render(int x, int y, int cx, int cy) {
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
void Cells::init(SDL_Renderer *renderer) {
    for (auto &r : cells_) {
        for (auto &c : r) {
            c.renderer = renderer;
            c.updateTexture();
        }
    }
}

void Cells::foreach(const std::function<void(Cell &, int, int)> &callback) {
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            callback(cells_[y][x], x, y);
        }
    }
}
