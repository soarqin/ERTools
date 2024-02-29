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

static int gCellSize[2] = {150, 150};
static int gCellRoundCorner = 5;
static int gCellSpacing = 2;
static int gCellBorder = 0;
static SDL_Color gCellSpacingColor = {255, 255, 255, 160};
static SDL_Color gCellBorderColor = {0, 0, 0, 0};
static SDL_Color gTextColor = {255, 255, 255, 255};
static int gTextShadow = 0;
static SDL_Color gTextShadowColor = {0, 0, 0, 255};
static int gTextShadowOffset[2] = {0, 0};
static std::string gFontFile = "data/font.ttf";
static int gFontStyle = TTF_STYLE_NORMAL;
static int gFontSize = 24;
static std::string gScoreFontFile = "data/font.ttf";
static int gScoreFontStyle = TTF_STYLE_NORMAL;
static int gScoreFontSize = 24;
static int gScoreTextShadow = 0;
static SDL_Color gScoreTextShadowColor = {0, 0, 0, 255};
static int gScoreTextShadowOffset[2] = {0, 0};
static std::string gScoreNameFontFile = "data/font.ttf";
static int gScoreNameFontStyle = TTF_STYLE_NORMAL;
static int gScoreNameFontSize = 24;
static int gScoreNameTextShadow = 0;
static SDL_Color gScoreNameTextShadowColor = {0, 0, 0, 255};
static int gScoreNameTextShadowOffset[2] = {0, 0};
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

static std::string gScoreWinText = "{1}";
static std::string gScoreBingoText = "Bingo!";
static std::string gScoreNameWinText = "{0}获胜";
static std::string gScoreNameBingoText = "{0}达成Bingo!";

static const SDL_Color black = {0x00, 0x00, 0x00, 0x00};
static const SDL_Color white = {0xff, 0xff, 0xff, 0x00};

SDL_Surface *TTF_RenderUTF8_BlackOutline_Wrapped(TTF_Font *font, const char *t, const SDL_Color *c, int wrapLength, const SDL_Color *shadowColor, int outline, int offset[2]) {
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

    bool fullOutline = offset[0] == 0 && offset[1] == 0;
    auto olx = fullOutline ? (1 + outline * 2) : std::abs(offset[0]);
    auto oly = fullOutline ? (1 + outline * 2) : std::abs(offset[1]);
    auto sw = fullOutline ? (1 + outline * 2) : outline;
    auto sh = fullOutline ? (1 + outline * 2) : outline;
    bg = SDL_CreateSurface(black_letters->w + olx, black_letters->h + oly, SDL_PIXELFORMAT_RGBA8888);

    dstrect.y = offset[1] > 0 ? offset[1] : 0;
    dstrect.w = black_letters->w;
    dstrect.h = black_letters->h;

    for (int j = 0; j < sh; j++, dstrect.y++) {
        dstrect.x = offset[0] > 0 ? offset[0] : 0;
        for (int i = 0; i < sw; i++, dstrect.x++)
            SDL_BlitSurface(black_letters, nullptr, bg, &dstrect);
    }

    SDL_DestroySurface(black_letters);

    /* --- Put the color version of the text on top! --- */
    white_letters = TTF_RenderUTF8_Blended_Wrapped(font, t, *c, wrapLength);

    if (!white_letters) {
        return nullptr;
    }

    if (fullOutline) {
        dstrect.x = outline;
    } else if (offset[0] >= 0) {
        dstrect.x = 0;
    } else if (offset[0] < 0) {
        dstrect.x = -offset[0];
    }

    if (fullOutline) {
        dstrect.y = outline;
    } else if (offset[1] >= 0) {
        dstrect.y = 0;
    } else if (offset[1] < 0) {
        dstrect.y = -offset[1];
    }

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
            auto wrapLength = gCellSize[0] * 9 / 10;
            auto *surface =
                TTF_RenderUTF8_BlackOutline_Wrapped(font, text.c_str(), &gTextColor, wrapLength, &gTextShadowColor, gTextShadow, gTextShadowOffset);
            if (surface->h <= gCellSize[1] * 9 / 10) {
                texture = SDL_CreateTextureFromSurface(gRenderer, surface);
                SDL_DestroySurface(surface);
                break;
            }
            SDL_DestroySurface(surface);
            fontSize--;
            font = TTF_OpenFont(gFontFile.c_str(), fontSize);
            TTF_SetFontStyle(font, gFontStyle);
            TTF_SetFontWrappedAlign(font, TTF_WRAPPED_ALIGN_CENTER);
        }
        if (font != gFont) {
            TTF_CloseFont(font);
        }
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
    }

    inline void render(int x, int y, int cx, int cy) {
        bool dbl = status > 2;
        auto fx = (float)x, fy = (float)y, fcx = (float)cx, fcy = (float)cy;
        auto st = dbl ? (status - 2) : status;
        if (st > 0 && gColorTexture[st - 1]) {
            SDL_FRect dstrect = {fx, fy, fcx, fcy};
            SDL_RenderTexture(gRenderer, gColorTexture[st - 1], nullptr, &dstrect);
        } else {
            auto &col = gColorsInt[st];
            if (gCellRoundCorner > 0) {
                roundedBoxRGBA(gRenderer, x, y, x + cx - 1, y + cy - 1, gCellRoundCorner, col.r, col.g, col.b, col.a);
            } else {
                SDL_FRect dstrect = {fx, fy, fcx, fcy};
                SDL_SetRenderDrawColor(gRenderer, col.r, col.g, col.b, col.a);
                SDL_RenderFillRect(gRenderer, &dstrect);
            }
        }
        if (dbl) {
            auto &color = gColors[5 - status];
            const SDL_Vertex verts[] = {
                {SDL_FPoint{fx, fy + fcy}, color, SDL_FPoint{0},},
                {SDL_FPoint{fx, fy + fcy * .7f}, color, SDL_FPoint{0},},
                {SDL_FPoint{fx + fcx * .3f, fy + fcy}, color, SDL_FPoint{0},},
            };
            SDL_RenderGeometry(gRenderer, nullptr, verts, 3, nullptr, 0);
        }
        if (texture) {
            auto fw = (float)w, fh = (float)h;
            auto l = (float)(cx - w) * .5f, t = (float)(cy - h) * .5f;
            SDL_FRect rect = {fx + l, fy + t, fw, fh};
            SDL_RenderTexture(gRenderer, texture, nullptr, &rect);
        }
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
        destroy();
        index = idx;
        playerName = UnicodeToUtf8(gPlayerName[idx]);
        SDL_SetHint("SDL_BORDERLESS_RESIZABLE_STYLE", "1");
        SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
        SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
        for (int i = 0; i < 2; i++) {
            std::string title = i == 0 ? "Score A" : "Player A";
            title.back() += idx;
            window[i] = SDL_CreateWindow(title.c_str(), 200, 200, SDL_WINDOW_BORDERLESS | SDL_WINDOW_TRANSPARENT | SDL_WINDOW_ALWAYS_ON_TOP);
            renderer[i] = SDL_CreateRenderer(window[i], "opengl", SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            SDL_SetWindowHitTest(window[i], ScoreWindowHitTestCallback, nullptr);
            int x, y;
            SDL_GetWindowPosition(gWindow, &x, &y);
            int cw, ch;
            SDL_GetWindowSize(gWindow, &cw, &ch);
            SDL_SetWindowPosition(window[i], x + i * 200, y + ch + 8 + idx * (std::max(gScoreFontSize, gScoreNameFontSize) + gScorePadding * 2 + 8));
            if (!gColorTextureFile[index].empty()) {
                colorMask[i] = loadTexture(renderer[i], gColorTextureFile[index].c_str());
                SDL_SetTextureBlendMode(colorMask[i], SDL_BLENDMODE_MUL);
            }
        }
    }

    void destroy() {
        for (int i = 0; i < 2; i++) {
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
            w[i] = 0;
            h[i] = 0;
            tw[i] = 0;
            th[i] = 0;
        }
        index = -1;
        score = -1;
        hasExtraScore = false;
        extraScore = 0;
    }

    void updateTexture() {
        std::string utext[2] = {
            fmt::format(gBingoBrawlersMode ? score >= 100 ? gScoreBingoText : score >= 13 ? gScoreWinText : "{1}" : "{1}", playerName, score),
            fmt::format(gBingoBrawlersMode ? score >= 100 ? gScoreNameBingoText : score >= 13 ? gScoreNameWinText : "{0}" : "{0}", playerName, score)
        };
        TTF_Font *ufont[2] = {gScoreFont, gScoreNameFont};
        int ushadow[2] = {gScoreTextShadow, gScoreNameTextShadow};
        SDL_Color ushadowColor[2] = {gScoreTextShadowColor, gScoreNameTextShadowColor};
        int *ushadowOffset[2] = {gScoreTextShadowOffset, gScoreNameTextShadowOffset};
        for (int i = 0; i < 2; i++) {
            SDL_Surface *usurface;
            if (colorMask[i]) {
                SDL_Color clr = {255, 255, 255, 255};
                usurface = TTF_RenderUTF8_BlackOutline_Wrapped(ufont[i], utext[i].c_str(), &clr, 0, &ushadowColor[i], ushadow[i], ushadowOffset[i]);
            } else {
                usurface = TTF_RenderUTF8_BlackOutline_Wrapped(ufont[i], utext[i].c_str(), &gColorsInt[index + 1], 0, &ushadowColor[i], ushadow[i], ushadowOffset[i]);
            }
            auto *utexture = SDL_CreateTextureFromSurface(renderer[i], usurface);
            auto mw = usurface->w, mh = usurface->h;
            if (texture[i])
                SDL_DestroyTexture(texture[i]);
            texture[i] = SDL_CreateTexture(renderer[i], SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, mw, mh);
            SDL_SetRenderTarget(renderer[i], texture[i]);
            SDL_SetRenderDrawColor(renderer[i], 0, 0, 0, 0);
            SDL_RenderClear(renderer[i]);
            SDL_FRect rc = {0, 0, (float)mw, (float)mh};
            SDL_RenderTexture(renderer[i], utexture, nullptr, &rc);
            if (colorMask[i]) {
                SDL_RenderTexture(renderer[i], colorMask[i], nullptr, nullptr);
            }
            SDL_DestroyTexture(utexture);
            SDL_DestroySurface(usurface);
            SDL_SetRenderTarget(renderer[i], nullptr);
            SDL_SetTextureBlendMode(texture[i], SDL_BLENDMODE_BLEND);
            SDL_QueryTexture(texture[i], nullptr, nullptr, &tw[i], &th[i]);
            w[i] = tw[i] + gScorePadding * 2;
            h[i] = th[i] + gScorePadding * 2;
            SDL_SetWindowSize(window[i], w[i], h[i]);
        }
    }

    inline void render() {
        for (int i = 0; i < 2; i++) {
            SDL_SetRenderDrawColor(renderer[i], 0, 0, 0, 0);
            SDL_RenderClear(renderer[i]);
            if (gScoreRoundCorner > 0)
                roundedBoxRGBA(renderer[i], 0, 0, w[i] - 1, h[i] - 1, gScoreRoundCorner,
                               gScoreBackgroundColor.r, gScoreBackgroundColor.g, gScoreBackgroundColor.b, gScoreBackgroundColor.a);
            else {
                SDL_SetRenderDrawColor(renderer[i],
                                       gScoreBackgroundColor.r, gScoreBackgroundColor.g, gScoreBackgroundColor.b, gScoreBackgroundColor.a);
                SDL_FRect dstrect = {0, 0, (float)w[i], (float)h[i]};
                SDL_RenderFillRect(renderer[i], &dstrect);
            }
            auto descend = TTF_FontDescent(i == 0 ? gScoreFont : gScoreNameFont);
            SDL_FRect dstRect = {(float)(w[i] - tw[i]) * .5f, (float)(h[i] - th[i] - (descend >> 1)) * .5f, (float)tw[i], (float)th[i]};
            SDL_RenderTexture(renderer[i], texture[i], nullptr, &dstRect);
            SDL_RenderPresent(renderer[i]);
        }
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

inline int setFontStyle(const std::string &style) {
    int res = TTF_STYLE_NORMAL;
    for (auto &c: style) {
        switch (c) {
            case 'B':
                res |= TTF_STYLE_BOLD;
                break;
            case 'I':
                res |= TTF_STYLE_ITALIC;
                break;
            default:
                break;
        }
    }
    return res;
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
            auto sl = splitString(value, ',');
            switch (sl.size()) {
            case 1:
                gCellSize[0] = gCellSize[1] = std::stoi(sl[0]);
                break;
            case 2:
                gCellSize[0] = std::stoi(sl[0]);
                gCellSize[1] = std::stoi(sl[1]);
                break;
            default:
                break;
            }
        } else if (key == "CellRoundCorner") {
            gCellRoundCorner = std::stoi(value);
        } else if (key == "CellSpacing") {
            gCellSpacing = std::stoi(value);
        } else if (key == "CellBorder") {
            gCellBorder = std::stoi(value);
        } else if (key == "FontFile") {
            auto sl = splitString(value, '|');
            if (sl.size() == 2) {
                gFontFile = sl[0];
                gFontStyle = setFontStyle(sl[1]);
            } else {
                gFontFile = value;
            }
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
        } else if (key == "TextShadowOffset") {
            auto sl = splitString(value, ',');
            if (sl.size() != 2) continue;
            gTextShadowOffset[0] = std::stoi(sl[0]);
            gTextShadowOffset[1] = std::stoi(sl[1]);
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
            auto sl = splitString(value, '|');
            if (sl.size() == 2) {
                gScoreFontFile = sl[0];
                gScoreFontStyle = setFontStyle(sl[1]);
            } else {
                gScoreFontFile = value;
            }
        } else if (key == "ScoreFontSize") {
            gScoreFontSize = std::stoi(value);
        } else if (key == "ScoreTextShadow") {
            gScoreTextShadow = std::stoi(value);
        } else if (key == "ScoreTextShadowColor") {
            auto sl = splitString(value, ',');
            if (sl.size() != 4) continue;
            gScoreTextShadowColor.r = std::stoi(sl[0]);
            gScoreTextShadowColor.g = std::stoi(sl[1]);
            gScoreTextShadowColor.b = std::stoi(sl[2]);
            gScoreTextShadowColor.a = std::stoi(sl[3]);
        } else if (key == "ScoreTextShadowOffset") {
            auto sl = splitString(value, ',');
            if (sl.size() != 2) continue;
            gScoreTextShadowOffset[0] = std::stoi(sl[0]);
            gScoreTextShadowOffset[1] = std::stoi(sl[1]);
        } else if (key == "ScoreNameFontFile") {
            auto sl = splitString(value, '|');
            if (sl.size() == 2) {
                gScoreNameFontFile = sl[0];
                gScoreNameFontStyle = setFontStyle(sl[1]);
            } else {
                gScoreNameFontFile = value;
            }
        } else if (key == "ScoreNameFontSize") {
            gScoreNameFontSize = std::stoi(value);
        } else if (key == "ScoreNameTextShadow") {
            gScoreNameTextShadow = std::stoi(value);
        } else if (key == "ScoreNameTextShadowColor") {
            auto sl = splitString(value, ',');
            if (sl.size() != 4) continue;
            gScoreNameTextShadowColor.r = std::stoi(sl[0]);
            gScoreNameTextShadowColor.g = std::stoi(sl[1]);
            gScoreNameTextShadowColor.b = std::stoi(sl[2]);
            gScoreNameTextShadowColor.a = std::stoi(sl[3]);
        } else if (key == "ScoreNameTextShadowOffset") {
            auto sl = splitString(value, ',');
            if (sl.size() != 2) continue;
            gScoreNameTextShadowOffset[0] = std::stoi(sl[0]);
            gScoreNameTextShadowOffset[1] = std::stoi(sl[1]);
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
    if (gFont) {
        TTF_CloseFont(gFont);
        gFont = nullptr;
    }
    gFont = TTF_OpenFont(gFontFile.c_str(), gFontSize);
    TTF_SetFontStyle(gFont, gFontStyle);
    TTF_SetFontWrappedAlign(gFont, TTF_WRAPPED_ALIGN_CENTER);
    if (gScoreFont) {
        TTF_CloseFont(gScoreFont);
        gScoreFont = nullptr;
    }
    gScoreFont = TTF_OpenFont(gScoreFontFile.c_str(), gScoreFontSize);
    TTF_SetFontStyle(gScoreFont, gScoreFontStyle);
    TTF_SetFontWrappedAlign(gScoreFont, TTF_WRAPPED_ALIGN_CENTER);
    if (gScoreNameFont) {
        TTF_CloseFont(gScoreNameFont);
        gScoreNameFont = nullptr;
    }
    gScoreNameFont = TTF_OpenFont(gScoreNameFontFile.c_str(), gScoreNameFontSize);
    TTF_SetFontStyle(gScoreNameFont, gScoreNameFontStyle);
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
    for (int i = 0; i < 2; i++) {
        for (auto &win: gScoreWindows) {
            SDL_GetWindowPosition(win.window[i], &x, &y);
            n.emplace_back(std::to_string(x));
            n.emplace_back(std::to_string(y));
        }
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
    for (int i = 0; i < 2; i++) {
        for (auto &win: gScoreWindows) {
            x = std::stoi(sl[i++]);
            y = std::stoi(sl[i++]);
            SDL_SetWindowPosition(win.window[i], x, y);
        }
    }
}

SDL_HitTestResult HitTestCallback(SDL_Window *window, const SDL_Point *pt, void *data) {
    (void)data;
    (void)window;
    if (pt->y < 30)
        return SDL_HITTEST_DRAGGABLE;
    return SDL_HITTEST_NORMAL;
}

void reloadAll() {
    load();
    syncClose();
    syncInit();

    if (gWindow != nullptr) {
        SDL_DestroyWindow(gWindow);
        gWindow = nullptr;
    }
    if (gRenderer != nullptr) {
        SDL_DestroyRenderer(gRenderer);
        gRenderer = nullptr;
    }
    SDL_SetHint("SDL_BORDERLESS_RESIZABLE_STYLE", "1");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
    gWindow = SDL_CreateWindow("BingoTool",
                               gCellSize[0] * 5 + gCellSpacing * 4 + gCellBorder * 2,
                               gCellSize[1] * 5 + gCellSpacing * 4 + gCellBorder * 2,
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
    reloadAll();

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
                        AppendMenuW(menu, MF_STRING, 7, L"重新加载选项");
                        AppendMenuW(menu, MF_STRING, 8, L"退出");
                        POINT pt;
                        GetCursorPos(&pt);
                        auto hwnd = (HWND)SDL_GetProperty(SDL_GetWindowProperties(gWindow), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
                        auto cmd = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, nullptr);
                        switch (cmd) {
                            case 7:
                                reloadAll();
                                break;
                            case 8:
                                goto QUIT;
                            default:
                                break;
                        }
                        break;
                    }
                    float fx, fy;
                    SDL_GetMouseState(&fx, &fy);
                    int x = (int)fx, y = (int)fy;
                    int dx = (x - gCellBorder) / (gCellSize[0] + gCellSpacing);
                    int dy = (y - gCellBorder) / (gCellSize[1] + gCellSpacing);
                    if (dx >= gCellSize[0] || dy >= gCellSize[1]) break;
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
                        AppendMenuW(menu, MF_STRING, 7, L"重新加载选项");
                        AppendMenuW(menu, MF_STRING, 8, L"退出");
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
                                reloadAll();
                                break;
                            case 8:
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
        SDL_FRect rcInner = {(float)gCellBorder, (float)gCellBorder, (float)(gCellSize[0] * 5 + gCellSpacing * 4), (float)(gCellSize[1] * 5 + gCellSpacing * 4)};
        SDL_RenderFillRect(gRenderer, &rcInner);
        SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_NONE);
        auto cx = gCellSize[0], cy = gCellSize[1];
        for (int i = 0; i < 5; i++) {
            auto y = i * (cy + gCellSpacing) + gCellBorder;
            for (int j = 0; j < 5; j++) {
                auto x = j * (cx + gCellSpacing) + gCellBorder;
                auto &cell = gCells[i][j];
                cell.render(x, y, cx, cy);
            }
        }
        for (auto &sw : gScoreWindows) {
            sw.render();
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
