#include "randomtable.h"

#include <fmt/format.h>
#define WIN32_LEAN_AND_MEAN
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_gfxPrimitives.h>
#include <windows.h>
#include <shellapi.h>
#undef WIN32_LEAN_AND_MEAN

#include <cstdio>
#include <string>
#include <vector>
#include <fstream>
#include <utility>
#include <random>
#include <algorithm>

static int gCellSize = 150;
static int gCellRoundCorner = 5;
static int gCellSpacing = 2;
static SDL_Color gCellSpacingColor = {255, 255, 255, 160};
static SDL_Color gTextColor = {255, 255, 255, 255};
static std::string gFontFile = "data/font.ttf";
static int gFontSize = 24;
static std::string gScoreFontFile = "data/font.ttf";
static int gScoreFontSize = 24;
static SDL_Color gScoreBackgroundColor = {0, 0, 0, 0};
static int gScorePadding = 8;
static int gScoreRoundCorner = 10;
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

static std::string gScoreFormat = "{0}  {1}", gBingoFormat = "{0}达成Bingo!", gScoreWinFormat = "{0}  {1}  积分获胜!";

static const SDL_Color black = {0x00, 0x00, 0x00, 0x00};
static const SDL_Color white = {0xff, 0xff, 0xff, 0x00};

SDL_Surface *BlackOutline(TTF_Font *font, const char *t, const SDL_Color *c, int wrapLength) {
    SDL_Surface *out;
    SDL_Surface *black_letters;
    SDL_Surface *white_letters;
    SDL_Surface *bg;
    SDL_Rect dstrect;
    Uint32 color_key;

    if (!font) {
        fprintf(stderr, "BlackOutline(): could not load needed font - returning.\n");
        return nullptr;
    }

    if (!t || !c) {
        fprintf(stderr, "BlackOutline(): invalid ptr parameter, returning.\n");
        return nullptr;
    }

    if (t[0] == '\0') {
        fprintf(stderr, "BlackOutline(): empty string, returning\n");
        return nullptr;
    }

    black_letters = TTF_RenderUTF8_Blended_Wrapped(font, t, black, wrapLength);

    if (!black_letters) {
        fprintf(stderr, "Warning - BlackOutline() could not create image for %s\n", t);
        return nullptr;
    }

    bg = SDL_CreateSurface(black_letters->w + 5, black_letters->h + 5, SDL_PIXELFORMAT_RGBA8888);
    /* Use color key for eventual transparency: */
    color_key = SDL_MapRGB(bg->format, 01, 01, 01);
    SDL_FillSurfaceRect(bg, nullptr, color_key);

    /* Now draw black outline/shadow 2 pixels on each side: */
    dstrect.w = black_letters->w;
    dstrect.h = black_letters->h;

    /* NOTE: can make the "shadow" more or less pronounced by */
    /* changing the parameters of these loops.                */
    for (dstrect.x = 1; dstrect.x < 4; dstrect.x++)
        for (dstrect.y = 1; dstrect.y < 3; dstrect.y++)
            SDL_BlitSurface(black_letters, nullptr, bg, &dstrect);

    SDL_DestroySurface(black_letters);

    /* --- Put the color version of the text on top! --- */
    white_letters = TTF_RenderUTF8_Blended_Wrapped(font, t, *c, wrapLength);

    if (!white_letters) {
        fprintf(stderr, "Warning - BlackOutline() could not create image for %s\n", t);
        return nullptr;
    }

    dstrect.x = 1;
    dstrect.y = 1;
    SDL_BlitSurface(white_letters, nullptr, bg, &dstrect);
    SDL_DestroySurface(white_letters);

    /* --- Convert to the screen format for quicker blits --- */
    SDL_SetSurfaceRLE(bg, SDL_TRUE);
    SDL_SetSurfaceColorKey(bg, SDL_TRUE, color_key);
    out = SDL_ConvertSurfaceFormat(bg, SDL_PIXELFORMAT_RGBA8888);
    SDL_DestroySurface(bg);

    return out;
}

struct Cell {
    std::string text;
    SDL_Texture *texture = nullptr;
    uint32_t status = 0;
    int w = 0, h = 0;
    void updateTexture() {
        if (text.empty()) return;
        auto *font = gFont;
        auto fontSize = gFontSize;
        while (true) {
            auto wrapLength = gCellSize * 9 / 10;
            auto *surface =
                TTF_RenderUTF8_Blended_Wrapped(font, text.c_str(), gTextColor, wrapLength);
            if (surface->h <= gCellSize * 9 / 10) {
                auto *shadow = BlackOutline(font, text.c_str(), &black, wrapLength);
                SDL_BlitSurface(surface, nullptr, shadow, nullptr);
                texture = SDL_CreateTextureFromSurface(gRenderer, shadow);
                SDL_DestroySurface(shadow);
                SDL_DestroySurface(surface);
                break;
            }
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
        char name[256];
        WideCharToMultiByte(CP_UTF8, 0, gPlayerName[idx].c_str(), -1, name, 256, nullptr, nullptr);
        playerName = name;
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
        SDL_SetWindowPosition(window, x, y + h + 8 + idx * 50);
    }

    void destroy() {
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
        if (texture) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
        auto clrf = gColors[index + 1];
        SDL_Color clr = { (uint8_t)roundf(clrf.r * 255), (uint8_t)roundf(clrf.g * 255), (uint8_t)roundf(clrf.b * 255), 255 };
        auto scoreText = fmt::format(gBingoBrawlersMode ? score >= 100 ? gBingoFormat : score >= 13 ? gScoreWinFormat : gScoreFormat : gScoreFormat, playerName, score);
        auto *surface = TTF_RenderUTF8_Blended_Wrapped(gScoreFont, scoreText.c_str(), clr, 0);
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
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

static std::vector<std::string> splitString(const std::string &str, char sep) {
    std::vector<std::string> result;
    std::string::size_type pos1 = 0, pos2 = str.find(sep);
    while (pos2 != std::string::npos) {
        result.push_back(str.substr(pos1, pos2 - pos1));
        pos1 = pos2 + 1;
        pos2 = str.find(sep, pos1);
    }
    if (pos1 < str.length()) {
        result.push_back(str.substr(pos1));
    }
    return result;
}

static void randomCells() {
    RandomTable rt;
    rt.load("randomtable.txt");
    std::vector<std::string> result;
    rt.generate("squares.txt", result);
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
        } else if (key == "FontSize") {
            gFontSize = std::stoi(value);
        } else if (key == "TextColor") {
            auto sl = splitString(value, ',');
            if (sl.size() != 4) continue;
            gTextColor.r = std::stoi(sl[0]);
            gTextColor.g = std::stoi(sl[1]);
            gTextColor.b = std::stoi(sl[2]);
            gTextColor.a = std::stoi(sl[3]);
        } else if (key == "Color1") {
            auto sl = splitString(value, ',');
            if (sl.size() != 3) continue;
            gColorsInt[1].r = std::stoi(sl[0]);
            gColorsInt[1].g = std::stoi(sl[1]);
            gColorsInt[1].b = std::stoi(sl[2]);
        } else if (key == "Color2") {
            auto sl = splitString(value, ',');
            if (sl.size() != 3) continue;
            gColorsInt[2].r = std::stoi(sl[0]);
            gColorsInt[2].g = std::stoi(sl[1]);
            gColorsInt[2].b = std::stoi(sl[2]);
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
        } else if (key == "ScoreFormat") {
            gScoreFormat = value;
            unescape(gScoreFormat);
        } else if (key == "BingoFormat") {
            gBingoFormat = value;
            unescape(gBingoFormat);
        } else if (key == "ScoreWinFormat") {
            gScoreWinFormat = value;
            unescape(gScoreWinFormat);
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
            wchar_t name[256];
            MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, name, 256);
            gPlayerName[0] = name;
        } else if (key == "Player2") {
            wchar_t name[256];
            MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, name, 256);
            gPlayerName[1] = name;
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

    SDL_SetHint("SDL_BORDERLESS_RESIZABLE_STYLE", "1");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
    gWindow = SDL_CreateWindow("BingoTool",
                               gCellSize * 5 + gCellSpacing * 6,
                               gCellSize * 5 + gCellSpacing * 6,
                               SDL_WINDOW_BORDERLESS | SDL_WINDOW_TRANSPARENT | SDL_WINDOW_ALWAYS_ON_TOP);
    SDL_SetWindowPosition(gWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    gRenderer = SDL_CreateRenderer(gWindow, "opengl", SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetWindowHitTest(gWindow, HitTestCallback, nullptr);
    SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_NONE);

    postLoad();
    gScoreWindows[0].create(0);
    gScoreWindows[1].create(1);
    loadState();
    updateScores();

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
                    float fx, fy;
                    SDL_GetMouseState(&fx, &fy);
                    int x = (int)fx, y = (int)fy;
                    int dx = (x - gCellSpacing) / (gCellSize + gCellSpacing);
                    int dy = (y - gCellSpacing) / (gCellSize + gCellSpacing);
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
                                break;
                            }
                            case 2: {
                                cell.status = 2;
                                updateScores();
                                break;
                            }
                            case 3: {
                                cell.status = 0;
                                updateScores();
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
                                break;
                            }
                            case 2: {
                                if (cell.status == 0) {
                                    cell.status = 2;
                                } else {
                                    cell.status = 3;
                                }
                                updateScores();
                                break;
                            }
                            case 3: {
                                cell.status = 0;
                                updateScores();
                                break;
                            }
                            case 4: {
                                if (gScoreWindows[0].hasExtraScore) {
                                    gScoreWindows[0].unsetExtraScore();
                                } else {
                                    gScoreWindows[0].setExtraScore(0);
                                }
                                updateScores();
                                break;
                            }
                            case 5: {
                                if (gScoreWindows[1].hasExtraScore) {
                                    gScoreWindows[1].unsetExtraScore();
                                } else {
                                    gScoreWindows[1].setExtraScore(1);
                                }
                                updateScores();
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
        SDL_SetRenderDrawColor(gRenderer, gCellSpacingColor.r, gCellSpacingColor.g, gCellSpacingColor.b, gCellSpacingColor.a);
        SDL_RenderFillRect(gRenderer, nullptr);
        SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_NONE);
        auto cs = gCellSize;
        for (int i = 0; i < 5; i++) {
            auto y = i * (gCellSize + gCellSpacing) + gCellSpacing;
            for (int j = 0; j < 5; j++) {
                auto x = j * (gCellSize + gCellSpacing) + gCellSpacing;
                auto &cell = gCells[i][j];
                bool dbl = cell.status > 2;
                auto fx = (float)x, fy = (float)y, fcs = (float)cs;
                auto &col = gColorsInt[dbl ? (cell.status - 2) : cell.status];
                if (gCellRoundCorner > 0) {
                    roundedBoxRGBA(gRenderer, x, y, x + cs, y + cs, gCellRoundCorner, col.r, col.g, col.b, col.a);
                } else {
                    SDL_FRect dstrect = {fx, fy, fcs, fcs};
                    SDL_SetRenderDrawColor(gRenderer, col.r, col.g, col.b, col.a);
                    SDL_RenderFillRect(gRenderer, &dstrect);
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
                roundedBoxRGBA(sw.renderer, 0, 0, sw.w, sw.h, gScoreRoundCorner, gScoreBackgroundColor.r, gScoreBackgroundColor.g, gScoreBackgroundColor.b, gScoreBackgroundColor.a);
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
    }
QUIT:
    saveState();
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
