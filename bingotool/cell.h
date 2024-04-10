#pragma once

#include "textsource.h"

#include <SDL3/SDL.h>
#include <string>
#include <functional>

struct Cell {
    void deinit();
    void updateTexture();
    void render(int x, int y, int cx, int cy) const;
    int calculateMinimalWidth() const;
    bool setText(const std::string &text);

    SDL_Renderer *renderer = nullptr;
    SDL_Texture *texture = nullptr;
    TextSettings *textSettings = nullptr;
    TextSource *textSource = nullptr;
    uint32_t status = 0;
    int w = 0, h = 0;
    bool needDeallocTextSettings = false;
};

class Cells {
public:
    void preinit();
    void init();
    void deinit();
    void render();
    Cell *operator[](int index) { return cells_[index]; }
    void fitCellsForText();
    void foreach(const std::function<void(Cell &cell, int x, int y)> &callback);
    void resizeWindow();
    static void updateCellTextSettings(TextSettings *s);
    void updateScoreTextSettings();
    void resetCellFonts();
    void updateTextures(bool fit = true);
    void updateScoreTextures(int index);
    void reloadColorTexture();
    void reloadColorTexture(int index);
    void showSettingsWindow();
    inline TextSettings *cellTextSettings() { return cellTextSettings_; }
    inline TextSettings *scoreTextSettings() { return scoreTextSettings_; }
    SDL_Window *window() { return window_; }
    SDL_Texture *colorTexture(int index) { return colorTexture_[index]; }

private:
    SDL_Window *window_ = nullptr;
    SDL_Renderer *renderer_ = nullptr;
    TextSettings *cellTextSettings_ = nullptr;
    Cell cells_[5][5];
    SDL_Texture *colorTexture_[2] = {nullptr, nullptr};
    SDL_Texture *scoreTexture_[2] = {nullptr, nullptr};
    TextSettings *scoreTextSettings_ = nullptr;
    TextSource *scoreTextSource_ = nullptr;
};

extern Cells gCells;
