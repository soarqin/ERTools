#include "randomtable.h"
#include "sync.h"
#include "common.h"

#include <fmt/format.h>
#define WIN32_LEAN_AND_MEAN
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_gfxPrimitives.h>
#include <windows.h>
#include <shellapi.h>
#if defined(_MSC_VER)
#undef max
#undef min
#endif
#undef WIN32_LEAN_AND_MEAN

#include <cstdio>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

static int gCellSize = 150;
static int gCellRoundCorner = 5;
static int gCellSpacing = 2;
static int gCellBorder = 0;
static SDL_Color gCellSpacingColor = {255, 255, 255, 160};
static SDL_Color gCellBorderColor = {0, 0, 0, 0};
static SDL_Color gTextColor = {255, 255, 255, 255};
static int gTextShadow = 0;
static SDL_Color gTextShadowColor = {0, 0, 0, 255};
static std::string gFontFile = "data/font.ttf";
static int gFontSize = 24;
static std::string gScoreFontFile = "data/font.ttf";
static int gScoreFontSize = 24;
static std::string gScoreNameFontFile = "data/font.ttf";
static int gScoreNameFontSize = 24;
static SDL_Color gScoreBackgroundColor = {0, 0, 0, 0};
static int gScorePadding = 8;
static int gScoreRoundCorner = 0;
static std::string gColorTextureFile[2];
static SDL_Texture *gColorTexture[2] = {nullptr, nullptr};
static SDL_FColor gColors[3] = {
    {0, 0, 0, 0},
    {1, 0, 0, 0},
    {0, 0, 1, 0},
};
static SDL_Color gColorsInt[3] = {
    {0, 0, 0, 0},
    {255, 0, 0, 0},
    {0, 0, 255, 0},
};
static int gBingoBrawlersMode = 0;
static int gScores[5] = {2, 4, 6, 8, 10};
static int gNFScores[5] = {1, 2, 3, 4, 5};
static int gLineScore = 3;
static int gMaxPerRow = 3;
static int gClearScore = 0;
static int gClearQuestMultiplier = 2;
static std::wstring gPlayerName[2] = {L"红方", L"蓝方"};

static SDL_Window *gWindow = nullptr;
static SDL_Renderer *gRenderer = nullptr;
static TTF_Font *gFont = nullptr;
static TTF_Font *gScoreFont = nullptr;
static TTF_Font *gScoreNameFont = nullptr;

static int gScoreFormat = 1;
static std::string gScoreWinText = "{1}";
static std::string gScoreBingoText = "Bingo!";
static std::string gScoreNameWinText = "{0}获胜";
static std::string gScoreNameBingoText = "{0}达成Bingo!";

static const SDL_Color black = {0x00, 0x00, 0x00, 0x00};
static const SDL_Color white = {0xff, 0xff, 0xff, 0x00};

SDL_Surface *TTF_RenderUTF8_BlackOutline_Wrapped(TTF_Font *font, const char *t, const SDL_Color *c, int wrapLength, const SDL_Color *shadowColor, int outline) {
    if (outline <= 0)
        return TTF_RenderUTF8_Blended_Wrapped(font, t, *c, wrapLength);

    SDL_Surface *black_letters;
    SDL_Surface *white_letters;
    SDL_Surface *bg;
    SDL_Rect dstrect;

    if (!font) {
        return nullptr;
    }

    if (!t || !c) {
        return nullptr;
    }

    if (t[0] == '\0') {
        return nullptr;
    }

    black_letters = TTF_RenderUTF8_Blended_Wrapped(font, t, *shadowColor, wrapLength);

    if (!black_letters) {
        return nullptr;
    }

    auto ol = 1 + outline * 2;
    bg = SDL_CreateSurface(black_letters->w + ol, black_letters->h + ol, SDL_PIXELFORMAT_RGBA8888);

    /* Now draw black outline/shadow 2 pixels on each side: */
    dstrect.w = black_letters->w;
    dstrect.h = black_letters->h;

    /* NOTE: can make the "shadow" more or less pronounced by */
    /* changing the parameters of these loops.                */
    for (dstrect.x = 0; dstrect.x < ol; dstrect.x++)
        for (dstrect.y = 0; dstrect.y < ol; dstrect.y++)
            SDL_BlitSurface(black_letters, nullptr, bg, &dstrect);

    SDL_DestroySurface(black_letters);

    /* --- Put the color version of the text on top! --- */
    white_letters = TTF_RenderUTF8_Blended_Wrapped(font, t, *c, wrapLength);

    if (!white_letters) {
        return nullptr;
    }

    dstrect.x = outline;
    dstrect.y = outline;
    SDL_BlitSurface(white_letters, nullptr, bg, &dstrect);
    SDL_DestroySurface(white_letters);

    return bg;
}

struct Cell {
    std::string text;
    SDL_Texture *texture = nullptr;
    uint32_t status = 0;
    int w = 0, h = 0;
    void updateTexture() {
        if (texture) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
        if (text.empty()) {
            w = h = 0;
            return;
        }
        auto *font = gFont;
        auto fontSize = gFontSize;
        while (true) {
            auto wrapLength = gCellSize * 9 / 10;
            auto *surface =
                TTF_RenderUTF8_BlackOutline_Wrapped(font, text.c_str(), &gTextColor, wrapLength, &gTextShadowColor, gTextShadow);
            if (surface->h <= gCellSize * 9 / 10) {
                texture = SDL_CreateTextureFromSurface(gRenderer, surface);
                SDL_DestroySurface(surface);
                break;
            }
            SDL_DestroySurface(surface);
            fontSize--;
            font = TTF_OpenFont(gFontFile.c_str(), fontSize);
            TTF_SetFontWrappedAlign(font, TTF_WRAPPED_ALIGN_CENTER);
        }
        if (font != gFont) {
            TTF_CloseFont(font);
        }
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
    }
};

static Cell gCells[5][5];

struct ScoreWindow {
    static SDL_HitTestResult ScoreWindowHitTestCallback(SDL_Window *window, const SDL_Point *pt, void *data) {
        (void)data;
        (void)window;
        (void)pt;
        return SDL_HITTEST_DRAGGABLE;
    }

    void reset() {
        score = 0;
        extraScore = 0;
        hasExtraScore = false;
        updateTexture();
    }

    void create(int idx) {
        index = idx;
        playerName = UnicodeToUtf8(gPlayerName[idx]);
        const char *title = idx == 0 ? "Player A" : "Player B";
        SDL_SetHint("SDL_BORDERLESS_RESIZABLE_STYLE", "1");
        SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
        SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
        window = SDL_CreateWindow(title, 200, 200, SDL_WINDOW_BORDERLESS | SDL_WINDOW_TRANSPARENT | SDL_WINDOW_ALWAYS_ON_TOP);
        renderer = SDL_CreateRenderer(window, "opengl", SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        SDL_SetWindowHitTest(window, ScoreWindowHitTestCallback, nullptr);
        int x, y;
        SDL_GetWindowPosition(gWindow, &x, &y);
        SDL_GetWindowSize(gWindow, &w, &h);
        SDL_SetWindowPosition(window, x, y + h + 8 + idx * (gScoreFontSize + gScoreNameFontSize + gScorePadding * 2 + 8));
        if (!gColorTextureFile[index].empty()) {
            colorMask = loadTexture(renderer, gColorTextureFile[index].c_str());
            SDL_SetTextureBlendMode(colorMask, SDL_BLENDMODE_MUL);
        }
    }

    void destroy() {
        if (colorMask) {
            SDL_DestroyTexture(colorMask);
            colorMask = nullptr;
        }
        if (texture) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
        if (renderer) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }
        if (window) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
    }

    void updateTexture() {
        auto scoreText = fmt::format(gBingoBrawlersMode ? score >= 100 ? gScoreBingoText : score >= 13 ? gScoreWinText : "{1}" : "{1}", playerName, score);
        auto nameText = fmt::format(gBingoBrawlersMode ? score >= 100 ? gScoreNameBingoText : score >= 13 ? gScoreNameWinText : "{0}" : "{0}", playerName, score);
        SDL_Surface *surface1, *surface2;
        if (colorMask) {
            SDL_Color clr = {255, 255, 255, 255};
            surface1 = TTF_RenderUTF8_BlackOutline_Wrapped(gScoreFont, scoreText.c_str(), &clr, 0, &gTextShadowColor, gTextShadow);
            surface2 = TTF_RenderUTF8_BlackOutline_Wrapped(gScoreNameFont, nameText.c_str(), &clr, 0, &gTextShadowColor, gTextShadow);
        } else {
            surface1 = TTF_RenderUTF8_BlackOutline_Wrapped(gScoreFont, scoreText.c_str(), &gColorsInt[index + 1], 0, &gTextShadowColor, gTextShadow);
            surface2 = TTF_RenderUTF8_BlackOutline_Wrapped(gScoreNameFont, nameText.c_str(), &gColorsInt[index + 1], 0, &gTextShadowColor, gTextShadow);
        }
        auto *texture1 = SDL_CreateTextureFromSurface(renderer, surface1);
        auto *texture2 = SDL_CreateTextureFromSurface(renderer, surface2);
        auto mw = std::max(surface1->w, surface2->w), mh = surface1->h + surface2->h;
        if (texture)
            SDL_DestroyTexture(texture);
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, mw, mh);
        SDL_SetRenderTarget(renderer, texture);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        if (gScoreFormat > 0) {
            SDL_FRect rc1 = {(float)(mw - surface1->w) * .5f, 0, (float)surface1->w, (float)surface1->h};
            SDL_RenderTexture(renderer, texture1, nullptr, &rc1);
            SDL_FRect rc2 = {(float)(mw - surface2->w) * .5f, (float)surface1->h, (float)surface2->w, (float)surface2->h};
            SDL_RenderTexture(renderer, texture2, nullptr, &rc2);
        } else {
            SDL_FRect rc2 = {(float)(mw - surface2->w) * .5f, 0, (float)surface2->w, (float)surface2->h};
            SDL_RenderTexture(renderer, texture2, nullptr, &rc2);
            SDL_FRect rc1 = {(float)(mw - surface1->w) * .5f, (float)surface2->h, (float)surface1->w, (float)surface1->h};
            SDL_RenderTexture(renderer, texture1, nullptr, &rc1);
        }
        if (colorMask) {
            SDL_RenderTexture(renderer, colorMask, nullptr, nullptr);
        }
        SDL_DestroyTexture(texture1);
        SDL_DestroyTexture(texture2);
        SDL_DestroySurface(surface1);
        SDL_DestroySurface(surface2);
        SDL_SetRenderTarget(renderer, nullptr);
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        SDL_QueryTexture(texture, nullptr, nullptr, &tw, &th);
        w = tw + gScorePadding * 2;
        h = th + gScorePadding * 2 - 2;
        SDL_SetWindowSize(window, w, h);
    }

    inline void setExtraScore(int idx) {
        int count[2] = {0, 0};
        for (auto &r: gCells) {
            for (auto &c: r) {
                switch (c.status) {
                    case 1:
                        count[0]++;
                        break;
                    case 2:
                        count[1]++;
                        break;
                }
            }
        }
        extraScore = gClearScore + gClearQuestMultiplier * (count[idx] - count[1 - idx]);
        hasExtraScore = true;
    }

    inline void unsetExtraScore() {
        extraScore = 0;
        hasExtraScore = false;
    }

    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Texture *texture = nullptr;
    SDL_Texture *colorMask = nullptr;
    int w = 0, h = 0, tw = 0, th = 0;
    std::string playerName;
    int index = -1;
    int score = -1;
    bool hasExtraScore = false;
    int extraScore = 0;
};

static ScoreWindow gScoreWindows[2];

static void updateScores() {
    if (gBingoBrawlersMode) {
        int score[2] = {0, 0};
        for (auto & row : gCells) {
            for (auto & cell : row) {
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
            }
        }
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
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 5; ++j) {
            auto &cell = gCells[i][j];
            switch (cell.status) {
                case 1:
                    score[0] += gScores[i];
                    if (i == 0) line[j] = 0;
                    else if (line[j] != 0) line[j] = -1;
                    break;
                case 2:
                    score[1] += gScores[i];
                    if (i == 0) line[j] = 1;
                    else if (line[j] != 1) line[j] = -1;
                    break;
                case 3:
                    score[0] += gScores[i];
                    score[1] += gNFScores[i];
                    if (i == 0) line[j] = 0;
                    else if (line[j] != 0) line[j] = -1;
                    break;
                case 4:
                    score[0] += gNFScores[i];
                    score[1] += gScores[i];
                    if (i == 0) line[j] = 1;
                    else if (line[j] != 1) line[j] = -1;
                    break;
                default:
                    line[j] = -1;
                    break;
            }
        }
    }
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
        score[i] += gLineScore;
    }
    for (int i: cross) {
        if (i < 0) continue;
        score[i] += gLineScore;
    }
    if (gScoreWindows[0].hasExtraScore) {
        score[0] += gScoreWindows[0].extraScore;
    }
    if (gScoreWindows[1].hasExtraScore) {
        score[1] += gScoreWindows[1].extraScore;
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

void sendJudgeSyncState() {
    uint64_t val[2] = {0, 0};
    size_t idx = 0;
    for (auto &v: gCells) {
        for (auto &c: v) {
            val[idx / 16] |= (uint64_t)(uint32_t)c.status << (4 * (idx % 16));
            idx++;
        }
    }
    if (!gBingoBrawlersMode) {
        val[1] |= (uint64_t)gScoreWindows[0].score << 48;
        val[1] |= (uint64_t)gScoreWindows[1].score << 56;
    }
    syncSendData('S', std::to_string(val[0]) + ',' + std::to_string(val[1]));
}

void sendJudgeSyncData() {
    std::vector<std::string> n;
    for (auto &v: gCells) {
        for (auto &c: v) {
            n.emplace_back(c.text);
        }
    }
    n.emplace_back(UnicodeToUtf8(gPlayerName[0]));
    n.emplace_back(UnicodeToUtf8(gPlayerName[1]));
    syncSendData('T', n);
    sendJudgeSyncState();
}

static void randomCells() {
    if (syncGetMode() == 0) return;
    RandomTable rt;
    rt.load("randomtable.txt");
    std::vector<std::string> result;
    rt.generate(result);
    std::ofstream ofs("data/squares.txt");
    for (auto &s: result) {
        ofs << s << std::endl;
    }
    ofs.close();
    size_t idx = 0;
    for (auto &r: gCells) {
        for (auto &c: r) {
            c.status = 0;
            c.text = result[idx++];
            c.updateTexture();
        }
    }
    gScoreWindows[0].reset();
    gScoreWindows[1].reset();
    sendJudgeSyncData();
}

static std::string mergeString(const std::vector<std::string> &strs, char sep) {
    std::string result;
    for (auto &s: strs) {
        if (!result.empty()) result += sep;
        result += s;
    }
    return result;
}

static void unescape(std::string &str) {
    for (auto ite = str.begin(); ite != str.end(); ite++) {
        if (*ite == '\\') {
            ite = str.erase(ite);
            if (ite == str.end()) break;
            switch (*ite) {
                case 'n':
                    *ite = '\n';
                    break;
                case 'r':
                    *ite = '\r';
                    break;
                case 't':
                    *ite = '\t';
                    break;
                case '0':
                    *ite = '\0';
                    break;
                case '\\':
                    break;
                default:
                    ite--;
                    break;
            }
        }
    }
}

static void load() {
    std::ifstream ifs("data/config.txt");
    std::string line;
    while (!ifs.eof()) {
        std::getline(ifs, line);
        if (line.empty() || line[0] == ';') continue;
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        auto key = line.substr(0, pos);
        auto value = line.substr(pos + 1);
        if (key == "CellSize") {
            gCellSize = std::stoi(value);
        } else if (key == "CellRoundCorner") {
            gCellRoundCorner = std::stoi(value);
        } else if (key == "CellSpacing") {
            gCellSpacing = std::stoi(value);
        } else if (key == "CellBorder") {
            gCellBorder = std::stoi(value);
        } else if (key == "FontFile") {
            gFontFile = value;
        } else if (key == "CellColor") {
            auto sl = splitString(value, ',');
            if (sl.size() != 4) continue;
            gColorsInt[0].r = std::stoi(sl[0]);
            gColorsInt[0].g = std::stoi(sl[1]);
            gColorsInt[0].b = std::stoi(sl[2]);
            gColorsInt[0].a = std::stoi(sl[3]);
        } else if (key == "CellSpacingColor") {
            auto sl = splitString(value, ',');
            if (sl.size() != 4) continue;
            gCellSpacingColor.r = std::stoi(sl[0]);
            gCellSpacingColor.g = std::stoi(sl[1]);
            gCellSpacingColor.b = std::stoi(sl[2]);
            gCellSpacingColor.a = std::stoi(sl[3]);
        } else if (key == "CellBorderColor") {
            auto sl = splitString(value, ',');
            if (sl.size() != 4) continue;
            gCellBorderColor.r = std::stoi(sl[0]);
            gCellBorderColor.g = std::stoi(sl[1]);
            gCellBorderColor.b = std::stoi(sl[2]);
            gCellBorderColor.a = std::stoi(sl[3]);
        } else if (key == "FontSize") {
            gFontSize = std::stoi(value);
        } else if (key == "TextColor") {
            auto sl = splitString(value, ',');
            if (sl.size() != 4) continue;
            gTextColor.r = std::stoi(sl[0]);
            gTextColor.g = std::stoi(sl[1]);
            gTextColor.b = std::stoi(sl[2]);
            gTextColor.a = std::stoi(sl[3]);
        } else if (key == "TextShadow") {
            gTextShadow = std::stoi(value);
        } else if (key == "TextShadowColor") {
            auto sl = splitString(value, ',');
            if (sl.size() != 4) continue;
            gTextShadowColor.r = std::stoi(sl[0]);
            gTextShadowColor.g = std::stoi(sl[1]);
            gTextShadowColor.b = std::stoi(sl[2]);
            gTextShadowColor.a = std::stoi(sl[3]);
        } else if (key == "Color1") {
            auto sl = splitString(value, ',');
            if (sl.size() == 3) {
                gColorsInt[1].r = std::stoi(sl[0]);
                gColorsInt[1].g = std::stoi(sl[1]);
                gColorsInt[1].b = std::stoi(sl[2]);
                gColorTextureFile[0].clear();
                gColorTexture[0] = nullptr;
            } else if (sl.size() == 1) {
                gColorTextureFile[0] = sl[0].c_str();
            }
        } else if (key == "Color2") {
            auto sl = splitString(value, ',');
            if (sl.size() == 3) {
                gColorsInt[2].r = std::stoi(sl[0]);
                gColorsInt[2].g = std::stoi(sl[1]);
                gColorsInt[2].b = std::stoi(sl[2]);
                gColorTextureFile[1].clear();
                gColorTexture[1] = nullptr;
            } else if (sl.size() == 1) {
                gColorTextureFile[1] = sl[0].c_str();
            }
        } else if (key == "BingoBrawlersMode") {
            gBingoBrawlersMode = std::stoi(value) != 0 ? 1 : 0;
        } else if (key == "ScoreBackgroundColor") {
            auto sl = splitString(value, ',');
            if (sl.size() != 4) continue;
            gScoreBackgroundColor.r = std::stoi(sl[0]);
            gScoreBackgroundColor.g = std::stoi(sl[1]);
            gScoreBackgroundColor.b = std::stoi(sl[2]);
            gScoreBackgroundColor.a = std::stoi(sl[3]);
        } else if (key == "ScorePadding") {
            gScorePadding = std::stoi(value);
        } else if (key == "ScoreRoundCorner") {
            gScoreRoundCorner = std::stoi(value);
        } else if (key == "ScoreFontFile") {
            gScoreFontFile = value;
        } else if (key == "ScoreFontSize") {
            gScoreFontSize = std::stoi(value);
        } else if (key == "ScoreNameFontFile") {
            gScoreNameFontFile = value;
        } else if (key == "ScoreNameFontSize") {
            gScoreNameFontSize = std::stoi(value);
        } else if (key == "ScoreFormat") {
            gScoreFormat = std::stoi(value);
        } else if (key == "ScoreWinText") {
            gScoreWinText = value;
            unescape(gScoreWinText);
        } else if (key == "ScoreBingoText") {
            gScoreBingoText = value;
            unescape(gScoreBingoText);
        } else if (key == "ScoreNameWinText") {
            gScoreNameWinText = value;
            unescape(gScoreNameWinText);
        } else if (key == "ScoreNameBingoText") {
            gScoreNameBingoText = value;
            unescape(gScoreNameBingoText);
        } else if (key == "FirstScores") {
            auto sl = splitString(value, ',');
            int sz = (int)sl.size();
            if (sz > 5) sz = 5;
            int i;
            for (i = 0; i < sz; i++) {
                gScores[i] = std::stoi(sl[i]);
            }
            if (i == 0) {
                gScores[0] = 2;
                i = 1;
            }
            for (; i < 5; i++) {
                gScores[i] = gScores[i - 1];
            }
        } else if (key == "NoneFirstScores") {
            auto sl = splitString(value, ',');
            int sz = (int)sl.size();
            if (sz > 5) sz = 5;
            int i;
            for (i = 0; i < sz; i++) {
                gNFScores[i] = std::stoi(sl[i]);
            }
            if (i == 0) {
                gNFScores[0] = 1;
                i = 1;
            }
            for (; i < 5; i++) {
                gNFScores[i] = gNFScores[i - 1];
            }
        } else if (key == "LineScore") {
            gLineScore = std::stoi(value);
        } else if (key == "MaxPerRow") {
            gMaxPerRow = std::stoi(value);
        } else if (key == "ClearScore") {
            gClearScore = std::stoi(value);
        } else if (key == "ClearQuestMultiplier") {
            gClearQuestMultiplier = std::stoi(value);
        } else if (key == "Player1") {
            gPlayerName[0] = Utf8ToUnicode(value);
        } else if (key == "Player2") {
            gPlayerName[1] = Utf8ToUnicode(value);
        }
    }
    for (int i = 0; i < 3; i++) {
        auto &c = gColors[i];
        c.r = (float)gColorsInt[i].r / 255.f;
        c.g = (float)gColorsInt[i].g / 255.f;
        c.b = (float)gColorsInt[i].b / 255.f;
        if (i > 0)
            gColorsInt[i].a = gColorsInt[0].a;
        c.a = (float)gColorsInt[i].a / 255.f;
    }
    if (gBingoBrawlersMode) {
        gMaxPerRow = 5;
        gClearQuestMultiplier = 0;
        for (int i = 0; i < 5; i++) {
            gScores[i] = 1;
            gNFScores[i] = 0;
        }
    }
}

static void postLoad() {
    gFont = TTF_OpenFont(gFontFile.c_str(), gFontSize);
    TTF_SetFontWrappedAlign(gFont, TTF_WRAPPED_ALIGN_CENTER);
    gScoreFont = TTF_OpenFont(gScoreFontFile.c_str(), gScoreFontSize);
    TTF_SetFontWrappedAlign(gScoreFont, TTF_WRAPPED_ALIGN_CENTER);
    gScoreNameFont = TTF_OpenFont(gScoreNameFontFile.c_str(), gScoreNameFontSize);
    TTF_SetFontWrappedAlign(gScoreNameFont, TTF_WRAPPED_ALIGN_CENTER);
    std::ifstream ifs("data/squares.txt");
    std::string line;
    for (auto & row : gCells) {
        for (auto & cell : row) {
            std::getline(ifs, line);
            cell.text = line;
            cell.updateTexture();
        }
    }
}

static void saveState() {
    std::ofstream ofs("state.txt");
    std::vector<std::string> n;
    for (auto &w: gScoreWindows) {
        n.emplace_back(std::to_string(w.hasExtraScore ? 1 : 0));
        n.emplace_back(std::to_string(w.extraScore));
    }
    for (auto &r: gCells) {
        for (auto &c: r) {
            n.emplace_back(std::to_string(c.status));
        }
    }
    int x, y;
    SDL_GetWindowPosition(gWindow, &x, &y);
    n.emplace_back(std::to_string(x));
    n.emplace_back(std::to_string(y));
    for (auto &win: gScoreWindows) {
        SDL_GetWindowPosition(win.window, &x, &y);
        n.emplace_back(std::to_string(x));
        n.emplace_back(std::to_string(y));
    }
    ofs << mergeString(n, ',') << std::endl;
}

static void loadState() {
    std::ifstream ifs("state.txt");
    if (!ifs.is_open()) return;
    std::string line;
    std::getline(ifs, line);
    auto sl = splitString(line, ',');
    if (sl.size() < 35) return;
    int i = 0;
    for (auto &w: gScoreWindows) {
        w.hasExtraScore = sl[i++] == "1";
        w.extraScore = std::stoi(sl[i++]);
    }
    for (auto &r: gCells) {
        for (auto &c: r) {
            c.status = std::stoi(sl[i++]);
        }
    }
    int x = std::stoi(sl[i++]);
    int y = std::stoi(sl[i++]);
    SDL_SetWindowPosition(gWindow, x, y);
    for (auto &win: gScoreWindows) {
        x = std::stoi(sl[i++]);
        y = std::stoi(sl[i++]);
        SDL_SetWindowPosition(win.window, x, y);
    }
}

SDL_HitTestResult HitTestCallback(SDL_Window *window, const SDL_Point *pt, void *data) {
    (void)data;
    (void)window;
    if (pt->y < 30)
        return SDL_HITTEST_DRAGGABLE;
    return SDL_HITTEST_NORMAL;
}

int wmain(int argc, wchar_t *argv[]) {
    // Unused argc, argv
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) < 0) {
        printf("SDL could not be initialized!\n"
               "SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    TTF_Init();
    load();
    syncInit();

    SDL_SetHint("SDL_BORDERLESS_RESIZABLE_STYLE", "1");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
    gWindow = SDL_CreateWindow("BingoTool",
                               gCellSize * 5 + gCellSpacing * 4 + gCellBorder * 2,
                               gCellSize * 5 + gCellSpacing * 4 + gCellBorder * 2,
                               SDL_WINDOW_BORDERLESS | SDL_WINDOW_TRANSPARENT | SDL_WINDOW_ALWAYS_ON_TOP);
    SDL_SetWindowPosition(gWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    gRenderer = SDL_CreateRenderer(gWindow, "opengl", SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetWindowHitTest(gWindow, HitTestCallback, nullptr);
    SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_NONE);
    for (int i = 0; i < 2; i++) {
        if (gColorTextureFile[i].empty()) continue;
        gColorTexture[i] = loadTexture(gRenderer, gColorTextureFile[i].c_str());
    }

    postLoad();
    gScoreWindows[0].create(0);
    gScoreWindows[1].create(1);
    loadState();
    updateScores();

    syncOpen([]{
        syncSetChannel([](char t, const std::string &s) {
            switch (t) {
                case 'T': {
                    if (syncGetMode() != 0) break;
                    auto sl = splitString(s, '\n');
                    size_t idx = 0;
                    bool dirty = false;
                    for (auto &r: gCells) {
                        for (auto &c: r) {
                            if (idx >= sl.size()) break;
                            const auto &text = sl[idx++];
                            if (c.text == text) continue;
                            c.text = text;
                            c.updateTexture();
                            dirty = true;
                        }
                    }
                    if (idx < sl.size()) {
                        auto pname = sl[idx++];;
                        gPlayerName[0] = Utf8ToUnicode(pname);
                        if (gScoreWindows[0].playerName != pname) {
                            gScoreWindows[0].playerName = pname;
                            gScoreWindows[0].updateTexture();
                        }
                    }
                    if (idx < sl.size()) {
                        auto pname = sl[idx++];;
                        gPlayerName[1] = Utf8ToUnicode(pname);
                        if (gScoreWindows[1].playerName != pname) {
                            gScoreWindows[1].playerName = pname;
                            gScoreWindows[1].updateTexture();
                        }
                    }
                    if (dirty) {
                        std::ofstream ofs("data/squares.txt");
                        ofs << s << std::endl;
                        ofs.close();
                    }
                    break;
                }
                case 'S': {
                    if (syncGetMode() != 0) break;
                    auto sl = splitString(s, ',');
                    uint64_t val[2];
                    size_t idx = 0;
                    for (const auto &ss: sl) {
                        val[idx++] = std::stoull(ss);
                    }
                    idx = 0;
                    bool dirty = false;
                    for (auto &r: gCells) {
                        for (auto &c: r) {
                            auto state = (int)((val[idx / 16] >> (4 * (idx % 16))) & 0x0FULL);
                            idx++;
                            if (c.status == state) continue;
                            c.status = state;
                            c.updateTexture();
                            dirty = true;
                        }
                    }
                    if (!gBingoBrawlersMode) {
                        int es[2] = {
                            (int)(uint32_t)((val[1] >> 48) & 0xFFULL),
                            (int)(uint32_t)((val[1] >> 56) & 0xFFULL),
                        };
                        for (int i = 0; i < 2; i++) {
                            if (es[i] != gScoreWindows[i].extraScore) {
                                gScoreWindows[i].extraScore = es[i];
                                gScoreWindows[i].hasExtraScore = es[i] != 0;
                                dirty = true;
                            }
                        }
                    }
                    if (dirty) {
                        updateScores();
                        saveState();
                    }
                    break;
                }
            }
        });
        if (syncGetMode() != 0) {
            sendJudgeSyncData();
        } else {
            syncSendData('T', "");
            syncSendData('S', "");
        }
    });

    while (true) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_EVENT_QUIT:
                    goto QUIT;
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
                    if (e.window.windowID == SDL_GetWindowID(gWindow)) goto QUIT;
                    break;
                }
                case SDL_EVENT_MOUSE_BUTTON_UP: {
                    if (e.button.windowID == SDL_GetWindowID(gWindow) && e.button.button != SDL_BUTTON_RIGHT) break;
                    if (syncGetMode() == 0) {
                        auto menu = CreatePopupMenu();
                        AppendMenuW(menu, MF_STRING, 7, L"退出");
                        POINT pt;
                        GetCursorPos(&pt);
                        auto hwnd = (HWND)SDL_GetProperty(SDL_GetWindowProperties(gWindow), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
                        auto cmd = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, nullptr);
                        switch (cmd) {
                            case 7:
                                goto QUIT;
                            default:
                                break;
                        }
                        break;
                    }
                    float fx, fy;
                    SDL_GetMouseState(&fx, &fy);
                    int x = (int)fx, y = (int)fy;
                    int dx = (x - gCellBorder) / (gCellSize + gCellSpacing);
                    int dy = (y - gCellBorder) / (gCellSize + gCellSpacing);
                    if (dx >= gCellSize || dy >= gCellSize) break;
                    auto &cell = gCells[dy][dx];
                    auto menu = CreatePopupMenu();
                    int count[2] = {0, 0};
                    for (int j = 0; j < 5; ++j) {
                        switch (gCells[dy][j].status) {
                            case 1:
                                count[0]++;
                                break;
                            case 2:
                                count[1]++;
                                break;
                            case 3:
                            case 4:
                                count[0]++;
                                count[1]++;
                                break;
                        }
                    }
                    if (gBingoBrawlersMode) {
                        AppendMenuW(menu,
                                    (cell.status != 0 ? MF_DISABLED : 0)
                                        | MF_STRING,
                                    1, (gPlayerName[0] + L"完成").c_str());
                        AppendMenuW(menu,
                                    (cell.status != 0 ? MF_DISABLED : 0)
                                        | MF_STRING,
                                    2, (gPlayerName[1] + L"完成").c_str());
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, (cell.status == 0 ? MF_DISABLED : 0) | MF_STRING, 3, L"重置");
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING, 5, L"重新开始");
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING, 6, L"重新随机表格");
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING, 7, L"退出");
                        POINT pt;
                        GetCursorPos(&pt);
                        auto hwnd = (HWND)SDL_GetProperty(SDL_GetWindowProperties(gWindow), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
                        auto cmd = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, nullptr);
                        switch (cmd) {
                            case 1: {
                                cell.status = 1;
                                updateScores();
                                sendJudgeSyncState();
                                break;
                            }
                            case 2: {
                                cell.status = 2;
                                updateScores();
                                sendJudgeSyncState();
                                break;
                            }
                            case 3: {
                                cell.status = 0;
                                updateScores();
                                sendJudgeSyncState();
                                break;
                            }
                            case 5: {
                                for (auto &r: gCells) {
                                    for (auto &c: r) {
                                        c.status = 0;
                                    }
                                }
                                gScoreWindows[0].reset();
                                gScoreWindows[1].reset();
                                sendJudgeSyncState();
                                break;
                            }
                            case 6:
                                randomCells();
                                break;
                            case 7:
                                goto QUIT;
                            default:
                                break;
                        }
                    } else {
                        AppendMenuW(menu,
                                    (cell.status == 1 || cell.status == 3 || cell.status == 4 || count[0] >= gMaxPerRow ? MF_DISABLED : 0)
                                        | MF_STRING,
                                    1, (gPlayerName[0] + L"完成").c_str());
                        AppendMenuW(menu,
                                    (cell.status == 2 || cell.status == 3 || cell.status == 4 || count[1] >= gMaxPerRow ? MF_DISABLED : 0)
                                        | MF_STRING,
                                    2, (gPlayerName[1] + L"完成").c_str());
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, (cell.status == 0 ? MF_DISABLED : 0) | MF_STRING, 3, L"重置");
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING | (gScoreWindows[1].hasExtraScore ? MF_DISABLED : 0), 4,
                                    (gPlayerName[0] + (gScoreWindows[0].hasExtraScore ? L"重置为未通关状态" : L"通关")).c_str());
                        AppendMenuW(menu, MF_STRING | (gScoreWindows[0].hasExtraScore ? MF_DISABLED : 0), 5,
                                    (gPlayerName[1] + (gScoreWindows[1].hasExtraScore ? L"重置为未通关状态" : L"通关")).c_str());
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING, 6, L"重新开始");
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING, 7, L"退出");
                        POINT pt;
                        GetCursorPos(&pt);
                        auto hwnd = (HWND)SDL_GetProperty(SDL_GetWindowProperties(gWindow), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
                        auto cmd = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, nullptr);
                        switch (cmd) {
                            case 1: {
                                if (cell.status == 0) {
                                    cell.status = 1;
                                } else {
                                    cell.status = 4;
                                }
                                updateScores();
                                sendJudgeSyncState();
                                break;
                            }
                            case 2: {
                                if (cell.status == 0) {
                                    cell.status = 2;
                                } else {
                                    cell.status = 3;
                                }
                                updateScores();
                                sendJudgeSyncState();
                                break;
                            }
                            case 3: {
                                cell.status = 0;
                                updateScores();
                                sendJudgeSyncState();
                                break;
                            }
                            case 4: {
                                if (gScoreWindows[0].hasExtraScore) {
                                    gScoreWindows[0].unsetExtraScore();
                                } else {
                                    gScoreWindows[0].setExtraScore(0);
                                }
                                updateScores();
                                sendJudgeSyncState();
                                break;
                            }
                            case 5: {
                                if (gScoreWindows[1].hasExtraScore) {
                                    gScoreWindows[1].unsetExtraScore();
                                } else {
                                    gScoreWindows[1].setExtraScore(1);
                                }
                                updateScores();
                                sendJudgeSyncState();
                                break;
                            }
                            case 6: {
                                for (auto &r: gCells) {
                                    for (auto &c: r) {
                                        c.status = 0;
                                    }
                                }
                                gScoreWindows[0].reset();
                                gScoreWindows[1].reset();
                                sendJudgeSyncState();
                                break;
                            }
                            case 7:
                                goto QUIT;
                            default:
                                break;
                        }
                    }
                    break;
                }
            }
        }
        SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 0);
        SDL_RenderClear(gRenderer);
        SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(gRenderer, gCellBorderColor.r, gCellBorderColor.g, gCellBorderColor.b, gCellBorderColor.a);
        SDL_RenderFillRect(gRenderer, nullptr);
        SDL_SetRenderDrawColor(gRenderer, gCellSpacingColor.r, gCellSpacingColor.g, gCellSpacingColor.b, gCellSpacingColor.a);
        SDL_FRect rcInner = {(float)gCellBorder, (float)gCellBorder, (float)(gCellSize * 5 + gCellSpacing * 4), (float)(gCellSize * 5 + gCellSpacing * 4)};
        SDL_RenderFillRect(gRenderer, &rcInner);
        SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_NONE);
        auto cs = gCellSize;
        for (int i = 0; i < 5; i++) {
            auto y = i * (gCellSize + gCellSpacing) + gCellBorder;
            for (int j = 0; j < 5; j++) {
                auto x = j * (gCellSize + gCellSpacing) + gCellBorder;
                auto &cell = gCells[i][j];
                bool dbl = cell.status > 2;
                auto fx = (float)x, fy = (float)y, fcs = (float)cs;
                auto status = dbl ? (cell.status - 2) : cell.status;
                if (status > 0 && gColorTexture[status - 1]) {
                    SDL_FRect dstrect = {fx, fy, fcs, fcs};
                    SDL_RenderTexture(gRenderer, gColorTexture[status - 1], nullptr, &dstrect);
                } else {
                    auto &col = gColorsInt[status];
                    if (gCellRoundCorner > 0) {
                        roundedBoxRGBA(gRenderer, x, y, x + cs - 1, y + cs - 1, gCellRoundCorner, col.r, col.g, col.b, col.a);
                    } else {
                        SDL_FRect dstrect = {fx, fy, fcs, fcs};
                        SDL_SetRenderDrawColor(gRenderer, col.r, col.g, col.b, col.a);
                        SDL_RenderFillRect(gRenderer, &dstrect);
                    }
                }
                if (dbl) {
                    auto &color = gColors[5 - cell.status];
                    const SDL_Vertex verts[] = {
                        {SDL_FPoint{fx, fy + fcs}, color, SDL_FPoint{0},},
                        {SDL_FPoint{fx, fy + fcs * .7f}, color, SDL_FPoint{0},},
                        {SDL_FPoint{fx + fcs * .3f, fy + fcs}, color, SDL_FPoint{0},},
                    };
                    SDL_RenderGeometry(gRenderer, nullptr, verts, 3, nullptr, 0);
                }
                if (cell.texture) {
                    auto fw = (float)cell.w, fh = (float)cell.h;
                    auto l = (float)(gCellSize - cell.w) * .5f, t = (float)(gCellSize - cell.h) * .5f;
                    SDL_FRect rect = {fx + l, fy + t, fw, fh};
                    SDL_RenderTexture(gRenderer, cell.texture, nullptr, &rect);
                }
            }
        }
        for (auto &sw : gScoreWindows) {
            SDL_SetRenderDrawColor(sw.renderer, 0, 0, 0, 0);
            SDL_RenderClear(sw.renderer);
            if (gScoreRoundCorner > 0)
                roundedBoxRGBA(sw.renderer, 0, 0, sw.w - 1, sw.h - 1, gScoreRoundCorner, gScoreBackgroundColor.r, gScoreBackgroundColor.g, gScoreBackgroundColor.b, gScoreBackgroundColor.a);
            else {
                SDL_SetRenderDrawColor(sw.renderer, gScoreBackgroundColor.r, gScoreBackgroundColor.g, gScoreBackgroundColor.b, gScoreBackgroundColor.a);
                SDL_FRect dstrect = {0, 0, (float)sw.w, (float)sw.h};
                SDL_RenderFillRect(sw.renderer, &dstrect);
            }
            SDL_FRect dstRect = {(float)(sw.w - sw.tw) * .5f, (float)(sw.h - sw.th) * .5f + 1.f, (float)sw.tw, (float)sw.th};
            SDL_RenderTexture(sw.renderer, sw.texture, nullptr, &dstRect);
            SDL_RenderPresent(sw.renderer);
        }
        SDL_RenderPresent(gRenderer);
        syncProcess();
    }
QUIT:
    saveState();
    for (int i = 0; i < 2; i++) {
        SDL_DestroyTexture(gColorTexture[i]);
    }
    TTF_CloseFont(gScoreFont);
    TTF_CloseFont(gFont);
    for (auto &w: gScoreWindows) {
        w.destroy();
    }
    for (auto &c: gCells) {
        for (auto &cc: c) {
            SDL_DestroyTexture(cc.texture);
            cc.texture = nullptr;
        }
    }
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    TTF_Quit();
    SDL_Quit();
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    int argc;
    wchar_t **argv = CommandLineToArgvW(lpCmdLine, &argc);
    return wmain(argc, argv);
}
