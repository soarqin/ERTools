#include "scorewindow.h"

#include "cell.h"
#include "common.h"
#include "config.h"

#include <SDL3_gfxPrimitives.h>
#include <fmt/format.h>
#if defined(max)
#undef max
#endif
#if defined(min)
#undef min
#endif

ScoreWindow gScoreWindows[2];

SDL_HitTestResult ScoreWindowHitTestCallback(SDL_Window *window, const SDL_Point *pt, void *data) {
    (void)data;
    (void)window;
    (void)pt;
    return SDL_HITTEST_DRAGGABLE;
}

void ScoreWindow::reset() {
    score = 0;
    cleared = false;
    clearCount = 0;
    updateTexture();
}

void ScoreWindow::create(int idx, const std::string &name) {
    destroy();
    index = idx;
    playerName = name;
    if (!gConfig.simpleMode)
        createWindow();
}

void ScoreWindow::createWindow() {
    SDL_SetHint("SDL_BORDERLESS_RESIZABLE_STYLE", "1");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
    for (int i = 0; i < 2; i++) {
        std::string title = i == 0 ? "Score A" : "Player A";
        title.back() += index;
        window[i] = SDL_CreateWindow(title.c_str(), tw[i], th[i], SDL_WINDOW_BORDERLESS | SDL_WINDOW_TRANSPARENT | SDL_WINDOW_ALWAYS_ON_TOP);
        renderer[i] = SDL_CreateRenderer(window[i], "opengl", SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        SDL_SetWindowHitTest(window[i], ScoreWindowHitTestCallback, nullptr);
        int x, y;
        auto *cellwin = gCells.window();
        SDL_GetWindowPosition(cellwin, &x, &y);
        int cw, ch;
        SDL_GetWindowSize(cellwin, &cw, &ch);
        SDL_SetWindowPosition(window[i],
                              x + i * 200,
                              y + ch + 8 + index * (std::max(gConfig.scoreFontSize, gConfig.scoreNameFontSize) + gConfig.scorePadding * 2 + 8));
        if (!gConfig.colorTextureFile[index].empty()) {
            colorMask[i] = loadTexture(renderer[i], gConfig.colorTextureFile[index].c_str());
            SDL_SetTextureBlendMode(colorMask[i], SDL_BLENDMODE_MUL);
        }
        textSource[i] = new TextSource();
    }
}

void ScoreWindow::switchMode() {
    destroyWindow();
    if (!gConfig.simpleMode) {
        createWindow();
        for (int i = 0; i < 2; i++) {
            SDL_SetWindowPosition(window[i], tx[i], ty[i]);
        }
    }
}

void ScoreWindow::destroyWindow() {
    for (int i = 0; i < 2; i++) {
        if (textSource[i]) {
            delete textSource[i];
            textSource[i] = nullptr;
        }
        if (colorMask[i]) {
            SDL_DestroyTexture(colorMask[i]);
            colorMask[i] = nullptr;
        }
        if (texture[i]) {
            SDL_DestroyTexture(texture[i]);
            texture[i] = nullptr;
        }
        if (renderer[i]) {
            SDL_DestroyRenderer(renderer[i]);
            renderer[i] = nullptr;
        }
        if (window[i]) {
            SDL_DestroyWindow(window[i]);
            window[i] = nullptr;
        }
    }
}

void ScoreWindow::destroy() {
    destroyWindow();
    index = -1;
    score = -1;
    clearCount = 0;
    cleared = false;
}

void ScoreWindow::updateTexture(bool reloadMask) {
    if (gConfig.simpleMode) {
        gCells.updateScoreTextures(index);
        return;
    }
    if (reloadMask) {
        for (int i = 0; i < 2; i++) {
            if (colorMask[i]) {
                SDL_DestroyTexture(colorMask[i]);
            }
            colorMask[i] = loadTexture(renderer[i], gConfig.colorTextureFile[index].c_str());
            SDL_SetTextureBlendMode(colorMask[i], SDL_BLENDMODE_MUL);
        }
    }
    std::string utext[2] = {
        fmt::format(gConfig.bingoBrawlersMode ? score >= 100 ? gConfig.scoreBingoText : score >= 13 ? gConfig.scoreWinText : "{1}" : "{1}",
                    playerName,
                    score),
        fmt::format(gConfig.bingoBrawlersMode ? score >= 100 ? gConfig.scoreNameBingoText : score >= 13 ? gConfig.scoreNameWinText : "{0}" : "{0}",
                    playerName,
                    score)
    };
    for (int i = 0; i < 2; i++) {
        if (!textSource[i]->font) {
            textSource[i]->face = L"方正大黑简体";
            textSource[i]->face_size = gConfig.scoreFontSize;
            textSource[i]->bold = gConfig.scoreFontStyle & 1;
            textSource[i]->italic = gConfig.scoreFontStyle & 2;
            textSource[i]->underline = gConfig.scoreFontStyle & 4;
            textSource[i]->strikeout = gConfig.scoreFontStyle & 8;
            auto &c = gConfig.colorsInt[index + 1];
            textSource[i]->color = textSource[i]->color2 = c.b | (c.g << 8) | (c.r << 16) | (c.a << 24);
        }
        textSource[i]->text = Utf8ToUnicode(utext[i]);
        textSource[i]->Update();
        if (texture[i])
            SDL_DestroyTexture(texture[i]);
        texture[i] = SDL_CreateTextureFromSurface(renderer[i], textSource[i]->surface);
        SDL_SetTextureBlendMode(texture[i], SDL_BLENDMODE_BLEND);
        SDL_QueryTexture(texture[i], nullptr, nullptr, &tw[i], &th[i]);
        w[i] = tw[i] + gConfig.scorePadding * 2;
        h[i] = th[i] + gConfig.scorePadding * 2;
        SDL_SetWindowSize(window[i], w[i], h[i]);
    }
    return;
    TTF_Font *ufont[2] = {gConfig.scoreFont, gConfig.scoreNameFont};
    int ushadow[2] = {gConfig.scoreTextShadow, gConfig.scoreNameTextShadow};
    SDL_Color ushadowColor[2] = {gConfig.scoreTextShadowColor, gConfig.scoreNameTextShadowColor};
    int *ushadowOffset[2] = {gConfig.scoreTextShadowOffset, gConfig.scoreNameTextShadowOffset};
    for (int i = 0; i < 2; i++) {
        SDL_Surface *usurface;
        if (gConfig.useColorTexture[index]) {
            SDL_Color clr = {255, 255, 255, 255};
            usurface = TTF_RenderUTF8_BlackOutline_Wrapped(ufont[i], utext[i].c_str(), &clr, 0, &ushadowColor[i], ushadow[i], ushadowOffset[i]);
        } else {
            usurface = TTF_RenderUTF8_BlackOutline_Wrapped(ufont[i],
                                                           utext[i].c_str(),
                                                           &gConfig.colorsInt[index + 1],
                                                           0,
                                                           &ushadowColor[i],
                                                           ushadow[i],
                                                           ushadowOffset[i]);
        }
        SDL_Texture *utexture;
        if (usurface) {
            utexture = SDL_CreateTextureFromSurface(renderer[i], usurface);
        } else {
            utexture = SDL_CreateTexture(renderer[i], SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 8, 8);
        }
        int mw, mh;
        SDL_QueryTexture(utexture, nullptr, nullptr, &mw, &mh);
        if (texture[i])
            SDL_DestroyTexture(texture[i]);
        texture[i] = SDL_CreateTexture(renderer[i], SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, mw, mh);
        SDL_SetRenderTarget(renderer[i], texture[i]);
        SDL_SetRenderDrawColor(renderer[i], 0, 0, 0, 0);
        SDL_RenderClear(renderer[i]);
        SDL_FRect rc = {0, 0, (float)mw, (float)mh};
        SDL_RenderTexture(renderer[i], utexture, nullptr, &rc);
        if (gConfig.useColorTexture[index]) {
            SDL_RenderTexture(renderer[i], colorMask[i], nullptr, nullptr);
        }
        SDL_DestroyTexture(utexture);
        SDL_DestroySurface(usurface);
        SDL_SetRenderTarget(renderer[i], nullptr);
        SDL_SetTextureBlendMode(texture[i], SDL_BLENDMODE_BLEND);
        SDL_QueryTexture(texture[i], nullptr, nullptr, &tw[i], &th[i]);
        w[i] = tw[i] + gConfig.scorePadding * 2;
        h[i] = th[i] + gConfig.scorePadding * 2;
        SDL_SetWindowSize(window[i], w[i], h[i]);
    }
}

void ScoreWindow::updateXYValue() {
    if (gConfig.simpleMode) return;
    for (int i = 0; i < 2; i++) {
        SDL_GetWindowPosition(window[i], &tx[i], &ty[i]);
    }
}

void ScoreWindow::render() {
    if (gConfig.simpleMode) return;
    for (int i = 0; i < 2; i++) {
        SDL_SetRenderDrawColor(renderer[i], 0, 0, 0, 0);
        SDL_RenderClear(renderer[i]);
        if (gConfig.scoreRoundCorner > 0)
            roundedBoxRGBA(renderer[i],
                           0,
                           0,
                           w[i] - 1,
                           h[i] - 1,
                           gConfig.scoreRoundCorner,
                           gConfig.scoreBackgroundColor.r,
                           gConfig.scoreBackgroundColor.g,
                           gConfig.scoreBackgroundColor.b,
                           gConfig.scoreBackgroundColor.a);
        else {
            SDL_SetRenderDrawColor(renderer[i],
                                   gConfig.scoreBackgroundColor.r,
                                   gConfig.scoreBackgroundColor.g,
                                   gConfig.scoreBackgroundColor.b,
                                   gConfig.scoreBackgroundColor.a);
            SDL_FRect dstrect = {0, 0, (float)w[i], (float)h[i]};
            SDL_RenderFillRect(renderer[i], &dstrect);
        }
        auto descend = TTF_FontDescent(i == 0 ? gConfig.scoreFont : gConfig.scoreNameFont);
        SDL_FRect dstRect = {(float)(w[i] - tw[i]) * .5f, (float)(h[i] - th[i] - (descend >> 1)) * .5f, (float)tw[i], (float)th[i]};
        SDL_RenderTexture(renderer[i], texture[i], nullptr, &dstRect);
        SDL_RenderPresent(renderer[i]);
    }
}

void ScoreWindow::setClear(bool clear) {
    if (cleared == clear) return;
    cleared = clear;
    if (!clear) {
        return;
    }
    int count[2] = {0, 0};
    gCells.foreach([&count](Cell &cell, int, int) {
        switch (cell.status) {
            case 1:
                count[0]++;
                break;
            case 2:
                count[1]++;
                break;
        }
    });
    gScoreWindows[0].clearCount = count[0];
    gScoreWindows[1].clearCount = count[1];
}

void updateScores() {
    if (gConfig.bingoBrawlersMode) {
        int score[2] = {0, 0};
        gCells.foreach([&score](Cell &cell, int x, int y) {
            switch (cell.status) {
                case 1:
                    score[0]++;
                    break;
                case 2:
                    score[1]++;
                    break;
                default:
                    break;
            }
        });
        auto n = gCells[0][0].status;
        if (n) {
            auto match = true;
            for (int i = 1; i < 5; i++) {
                if (gCells[i][i].status != n) {
                    match = false;
                    break;
                }
            }
            if (match) {
                score[n - 1] = 100;
            }
        }
        n = gCells[4][0].status;
        if (n) {
            auto match = true;
            for (int i = 1; i < 5; i++) {
                if (gCells[4 - i][i].status != n) {
                    match = false;
                    break;
                }
            }
            if (match) {
                score[n - 1] = 100;
            }
        }
        for (int j = 0; j < 5; j++) {
            n = gCells[j][0].status;
            if (n) {
                auto match = true;
                for (int i = 1; i < 5; i++) {
                    if (gCells[j][i].status != n) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    score[n - 1] = 100;
                }
            }
            n = gCells[0][j].status;
            if (n) {
                auto match = true;
                for (int i = 1; i < 5; i++) {
                    if (gCells[i][j].status != n) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    score[n - 1] = 100;
                }
            }
        }
        if (score[0] != gScoreWindows[0].score) {
            gScoreWindows[0].score = score[0];
            gScoreWindows[0].updateTexture();
        }
        if (score[1] != gScoreWindows[1].score) {
            gScoreWindows[1].score = score[1];
            gScoreWindows[1].updateTexture();
        }
        return;
    }
    int score[2] = {0, 0};
    int line[5] = {0};
    gCells.foreach([&score, &line](Cell &cell, int x, int y) {
        switch (cell.status) {
            case 1:
                score[0] += gConfig.scores[y];
                if (y == 0) line[x] = 0;
                else if (line[x] != 0) line[x] = -1;
                break;
            case 2:
                score[1] += gConfig.scores[y];
                if (y == 0) line[x] = 1;
                else if (line[x] != 1) line[x] = -1;
                break;
            case 3:
                score[0] += gConfig.scores[y];
                score[1] += gConfig.nFScores[y];
                if (y == 0) line[x] = 0;
                else if (line[x] != 0) line[x] = -1;
                break;
            case 4:
                score[0] += gConfig.nFScores[y];
                score[1] += gConfig.scores[y];
                if (y == 0) line[x] = 1;
                else if (line[x] != 1) line[x] = -1;
                break;
            default:
                line[x] = -1;
                break;
        }
    });
    int cross[2] = {0, 0};
    for (int i = 0; i < 5; ++i) {
        switch (gCells[i][i].status) {
            case 1:
            case 3:
                if (i == 0) cross[0] = 0;
                else if (cross[0] != 0) cross[0] = -1;
                break;
            case 2:
            case 4:
                if (i == 0) cross[0] = 1;
                else if (cross[0] != 1) cross[0] = -1;
                break;
            default:
                cross[0] = -1;
                break;
        }
        switch (gCells[i][4 - i].status) {
            case 1:
            case 3:
                if (i == 0) cross[1] = 0;
                else if (cross[1] != 0) cross[1] = -1;
                break;
            case 2:
            case 4:
                if (i == 0) cross[1] = 1;
                else if (cross[1] != 1) cross[1] = -1;
                break;
            default:
                cross[1] = -1;
                break;
        }
    }
    for (int i: line) {
        if (i < 0) continue;
        score[i] += gConfig.lineScore;
    }
    for (int i: cross) {
        if (i < 0) continue;
        score[i] += gConfig.lineScore;
    }
    bool player1Cleared = gScoreWindows[0].cleared;
    if (player1Cleared || gScoreWindows[1].cleared) {
        score[0] += (player1Cleared ? gConfig.clearScore : 0) + (gScoreWindows[0].clearCount - gScoreWindows[1].clearCount) * gConfig.clearQuestMultiplier;
        score[1] += (player1Cleared ? 0 : gConfig.clearScore) + (gScoreWindows[1].clearCount - gScoreWindows[0].clearCount) * gConfig.clearQuestMultiplier;
    }
    if (score[0] != gScoreWindows[0].score) {
        gScoreWindows[0].score = score[0];
        gScoreWindows[0].updateTexture();
    }
    if (score[1] != gScoreWindows[1].score) {
        gScoreWindows[1].score = score[1];
        gScoreWindows[1].updateTexture();
    }
}
