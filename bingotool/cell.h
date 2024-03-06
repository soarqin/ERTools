#pragma once

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
    void init(SDL_Renderer *renderer);
    Cell *operator[](int index) { return cells_[index]; }
    void fitCellsForText();
    void foreach(const std::function<void(Cell &cell, int x, int y)> &callback);
    void updateTextures();
    void showSettingsWindow();

private:
    Cell cells_[5][5];
};

extern Cells gCells;
