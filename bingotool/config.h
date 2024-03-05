#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>

struct Config {
    void load();
    void postLoad();

    int cellSize[2] = {150, 150};
    int cellRoundCorner = 5;
    int cellSpacing = 2;
    int cellBorder = 0;
    SDL_Color cellSpacingColor = {255, 255, 255, 160};
    SDL_Color cellBorderColor = {0, 0, 0, 0};
    SDL_Color textColor = {255, 255, 255, 255};
    int textShadow = 0;
    SDL_Color textShadowColor = {0, 0, 0, 255};
    int textShadowOffset[2] = {0, 0};
    std::string fontFile = "data/font.ttf";
    int fontStyle = TTF_STYLE_NORMAL;
    int fontSize = 24;
    std::string scoreFontFile = "data/font.ttf";
    int scoreFontStyle = TTF_STYLE_NORMAL;
    int scoreFontSize = 24;
    int scoreTextShadow = 0;
    SDL_Color scoreTextShadowColor = {0, 0, 0, 255};
    int scoreTextShadowOffset[2] = {0, 0};
    std::string scoreNameFontFile = "data/font.ttf";
    int scoreNameFontStyle = TTF_STYLE_NORMAL;
    int scoreNameFontSize = 24;
    int scoreNameTextShadow = 0;
    SDL_Color scoreNameTextShadowColor = {0, 0, 0, 255};
    int scoreNameTextShadowOffset[2] = {0, 0};
    SDL_Color scoreBackgroundColor = {0, 0, 0, 0};
    int scorePadding = 8;
    int scoreRoundCorner = 0;
    std::string colorTextureFile[2];
    SDL_Texture *colorTexture[2] = {nullptr, nullptr};
    SDL_FColor colors[3] = {
        {0, 0, 0, 0},
        {1, 0, 0, 0},
        {0, 0, 1, 0},
    };
    SDL_Color colorsInt[3] = {
        {0, 0, 0, 0},
        {255, 0, 0, 0},
        {0, 0, 255, 0},
    };
    int bingoBrawlersMode = 0;
    int scores[5] = {2, 4, 6, 8, 10};
    int nFScores[5] = {1, 2, 3, 4, 5};
    int lineScore = 3;
    int maxPerRow = 3;
    int clearScore = 0;
    int clearQuestMultiplier = 2;
    std::wstring playerName[2] = {L"红方", L"蓝方"};

    std::string scoreWinText = "{1}";
    std::string scoreBingoText = "Bingo!";
    std::string scoreNameWinText = "{0}获胜";
    std::string scoreNameBingoText = "{0}达成Bingo!";

    TTF_Font *font = nullptr;
    TTF_Font *scoreFont = nullptr;
    TTF_Font *scoreNameFont = nullptr;
};

extern Config gConfig;
