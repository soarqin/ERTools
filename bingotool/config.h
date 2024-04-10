#pragma once

#include <SDL3/SDL.h>
#include <string>

struct Config {
    void load();
    void save();
    void oldLoad();

    bool simpleMode = false;
    int originCellSizeX = 150;
    int cellSize[2] = {originCellSizeX, originCellSizeX};
    int cellSpacing = 2;
    int cellBorder = 0;
    SDL_Color cellSpacingColor = {255, 255, 255, 160};
    SDL_Color cellBorderColor = {0, 0, 0, 0};
    int cellAutoFit = 0;
    SDL_Color textColor = {255, 255, 255, 255};
    int textShadow = 0;
    SDL_Color textShadowColor = {0, 0, 0, 255};
    int textShadowOffset[2] = {0, 0};
    std::wstring fontFace = L"微软雅黑";
    int fontStyle = 0;
    int originalFontSize = 24;
    int fontSize = originalFontSize;
    std::wstring scoreFontFace = L"微软雅黑";
    int scoreFontStyle = 0;
    int scoreFontSize = 24;
    int scoreTextShadow = 0;
    SDL_Color scoreTextShadowColor = {0, 0, 0, 255};
    int scoreTextShadowOffset[2] = {0, 0};
    std::wstring scoreNameFontFace = L"微软雅黑";
    int scoreNameFontStyle = 0;
    int scoreNameFontSize = 24;
    int scoreNameTextShadow = 0;
    SDL_Color scoreNameTextShadowColor = {0, 0, 0, 255};
    int scoreNameTextShadowOffset[2] = {0, 0};
    SDL_Color scoreBackgroundColor = {0, 0, 0, 0};
    int scorePadding = 8;
    int scoreRoundCorner = 0;
    bool useColorTexture[2] = {false, false};
    std::string colorTextureFile[2];
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

    /* 0 - Periodic
     * 1 - Bingo Brawlers Season 1/2
     * 2 - Bingo Brawlers Season 3 */
    int bingoBrawlersMode = 1;

    /* Periodic rules */
    int scores[5] = {2, 2, 4, 4, 6};
    int nFScores[5] = {1, 1, 2, 2, 3};
    int lineScore = 3;
    int maxPerRow = 3;
    int clearScore = 0;
    int clearQuestMultiplier = 2;

    /* Bingo Brawlers Season 3 rules */
    int bingoScore = 2;
    int winScore = 13;

    std::wstring playerName[2] = {L"红方", L"蓝方"};

    std::string scoreWinText = "{1}";
    std::string scoreBingoText = "Bingo!";
    std::string scoreNameWinText = "{0}获胜";
    std::string scoreNameBingoText = "{0}达成Bingo!";
};

extern Config gConfig;
