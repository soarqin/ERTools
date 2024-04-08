#pragma once

#include "textsource.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <functional>

struct Cell {
    void updateTexture();
    void render(int x, int y, int cx, int cy) const;
    int calculateMinimalWidth(TTF_Font *font);
    int calculateMinimalHeight(TTF_Font *font);

    SDL_Renderer *renderer = nullptr;
    SDL_Texture *texture = nullptr;
    std::string text;
    uint32_t status = 0;
    int w = 0, h = 0;
};

class Cells {
public:
    void init();
    void deinit();
    void render();
    Cell *operator[](int index) { return cells_[index]; }
    void fitCellsForText();
    void foreach(const std::function<void(Cell &cell, int x, int y)> &callback);
    void resizeWindow();
    void updateTextures(bool fit = true);
    void updateScoreTextures(int index);
    void reloadColorTexture();
    void reloadColorTexture(int index);
    void showSettingsWindow();
    inline TextSource *textSource() { return textSource_; }
    SDL_Window *window() { return window_; }
    SDL_Texture *colorTexture(int index) { return colorTexture_[index]; }

private:
    SDL_Window *window_ = nullptr;
    SDL_Renderer *renderer_ = nullptr;
    Cell cells_[5][5];
    SDL_Texture *colorTexture_[2] = {nullptr, nullptr};
    SDL_Texture *scoreTexture_[2] = {nullptr, nullptr};
    TextSource *textSource_ = nullptr;
};

extern Cells gCells;
