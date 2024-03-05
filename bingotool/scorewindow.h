#pragma once

#include <SDL3/SDL.h>
#include <string>

struct ScoreWindow {
    static SDL_HitTestResult ScoreWindowHitTestCallback(SDL_Window *window, const SDL_Point *pt, void *data);

    void reset();

    void create(int idx, const std::string &name);

    void destroy();

    void updateTexture();

    void render();

    void setExtraScore(int idx);

    void unsetExtraScore();

    SDL_Window *window[2] = {nullptr, nullptr};
    SDL_Renderer *renderer[2] = {nullptr, nullptr};
    SDL_Texture *texture[2] = {nullptr, nullptr};
    SDL_Texture *colorMask[2] = {nullptr, nullptr};
    int w[2] = {0, 0}, h[2] = {0, 0}, tw[2] = {0, 0}, th[2] = {0, 0};
    std::string playerName;
    int index = -1;
    int score = -1;
    bool hasExtraScore = false;
    int extraScore = 0;
};

extern void updateScores();

extern ScoreWindow gScoreWindows[2];
