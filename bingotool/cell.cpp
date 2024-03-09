#include "cell.h"

#include "res/resource.h"

#include "common.h"
#include "config.h"
#include "scorewindow.h"

#if defined(_MSC_VER)
#define strcasecmp _stricmp
#endif

#include <SDL3_gfxPrimitives.h>
#include <fmt/format.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <shobjidl.h>
#include <shlguid.h>
#include <shellapi.h>
#undef WIN32_LEAN_AND_MEAN
#include <algorithm>
#if defined(max)
#undef max
#endif
#if defined(min)
#undef min
#endif

Cells gCells;

static HWND configDialog = nullptr;
static bool noEnChangeNotification = true;
static HBRUSH textColorBrush = nullptr;
static HBRUSH backgroundColorBrush = nullptr;
static HBRUSH textShadowColorBrush = nullptr;
static HBRUSH cellBorderColorBrush = nullptr;
static HBRUSH cellSpacingColorBrush = nullptr;
static HBRUSH scoreShadowColorBrush = nullptr;
static HBRUSH scoreBackgroundColorBrush = nullptr;
static HBRUSH nameShadowColorBrush = nullptr;
static HBRUSH player1ColorBrush = nullptr;
static HBRUSH player2ColorBrush = nullptr;
static HBITMAP player1Bitmap = nullptr;
static HBITMAP player2Bitmap = nullptr;

void Cell::updateTexture() {
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
    if (text.empty()) {
        w = h = 0;
        return;
    }
    auto *font = gConfig.font;
    switch (gConfig.cellAutoFit) {
        case 0: {
            auto fontSize = gConfig.fontSize;
            auto maxHeight = gConfig.cellSize[1] - 2;
            while (true) {
                bool fit = calculateMinimalHeight(font) <= maxHeight;
                if (fit) {
                    auto *surface =
                        TTF_RenderUTF8_BlackOutline_Wrapped(font,
                                                            text.c_str(),
                                                            &gConfig.textColor,
                                                            gConfig.cellSize[0] * 9 / 10,
                                                            &gConfig.textShadowColor,
                                                            gConfig.textShadow,
                                                            gConfig.textShadowOffset);
                    texture = SDL_CreateTextureFromSurface(renderer, surface);
                    SDL_DestroySurface(surface);
                }
                if (font != gConfig.font) {
                    TTF_CloseFont(font);
                }
                if (fit) break;
                fontSize--;
                font = TTF_OpenFont(gConfig.fontFile.c_str(), fontSize);
                TTF_SetFontStyle(font, gConfig.fontStyle);
                TTF_SetFontWrappedAlign(font, TTF_WRAPPED_ALIGN_CENTER);
            }
            break;
        }
        default: {
            auto *surface =
                TTF_RenderUTF8_BlackOutline_Wrapped(font, text.c_str(), &gConfig.textColor, gConfig.cellSize[0] * 9 / 10, &gConfig.textShadowColor, gConfig.textShadow, gConfig.textShadowOffset);
            texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_DestroySurface(surface);
            break;
        }
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
}

void Cell::render(int x, int y, int cx, int cy) const {
    bool dbl = status > 2;
    auto fx = (float)x, fy = (float)y, fcx = (float)cx, fcy = (float)cy;
    auto st = dbl ? (status - 2) : status;
    if (st > 0 && gConfig.useColorTexture[st - 1]) {
        SDL_FRect dstrect = {fx, fy, fcx, fcy};
        SDL_RenderTexture(renderer, gCells.colorTexture(st - 1), nullptr, &dstrect);
    } else {
        auto &col = gConfig.colorsInt[st];
        SDL_FRect dstrect = {fx, fy, fcx, fcy};
        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);
        SDL_RenderFillRect(renderer, &dstrect);
    }
    if (dbl) {
        auto &color = gConfig.colors[5 - status];
        const SDL_Vertex verts[] = {
            {SDL_FPoint{fx, fy + fcy}, color, SDL_FPoint{0},},
            {SDL_FPoint{fx, fy + fcy * .7f}, color, SDL_FPoint{0},},
            {SDL_FPoint{fx + fcx * .3f, fy + fcy}, color, SDL_FPoint{0},},
        };
        SDL_RenderGeometry(renderer, nullptr, verts, 3, nullptr, 0);
    }
    if (texture) {
        auto fw = (float)w, fh = (float)h;
        auto l = (float)(cx - w) * .5f, t = (float)(cy - h) * .5f;
        SDL_FRect rect = {fx + l, fy + t, fw, fh};
        SDL_RenderTexture(renderer, texture, nullptr, &rect);
    }
}

int Cell::calculateMinimalWidth(TTF_Font *font) {
    auto maxHeight = gConfig.cellSize[1] - 2;
    auto width = gConfig.cellSize[0];
    size_t len = text.length();
    int sw, sh;
    TTF_SizeUTF8(font, text.c_str(), &sw, &sh);
    int th = std::max(sh, TTF_FontLineSkip(font));
    while (true) {
        auto maxWidth = width * 9 / 10;
        size_t index = 0;
        int lines = 0;
        while (index < len) {
            int count;
            if (TTF_MeasureUTF8(font, text.c_str() + index, maxWidth, nullptr, &count) < 0) break;
            if (count == 0) break;
            lines++;
            while (count > 0 && index < len) {
                auto c = uint8_t(text[index]);
                if (c < 0x80) index++;
                else if (c < 0xE0) index += 2;
                else if (c < 0xF0) index += 3;
                else if (c < 0xF8) index += 4;
                else if (c < 0xFC) index += 5;
                else index += 6;
                count--;
            }
        }
        auto currHeight = (lines - 1) * th + sh;
        if (currHeight <= maxHeight) break;
        width += 2;
    }
    return width;
}

int Cell::calculateMinimalHeight(TTF_Font *font) {
    auto maxWidth = gConfig.cellSize[0] * 9 / 10;
    size_t index = 0;
    size_t len = text.length();
    int lines = 0;
    while (index < len) {
        int count;
        if (TTF_MeasureUTF8(font, text.c_str() + index, maxWidth, nullptr, &count) < 0) break;
        if (count == 0) break;
        lines++;
        while (count > 0 && index < len) {
            auto c = uint8_t(text[index]);
            if (c < 0x80) index++;
            else if (c < 0xE0) index += 2;
            else if (c < 0xF0) index += 3;
            else if (c < 0xF8) index += 4;
            else if (c < 0xFC) index += 5;
            else index += 6;
            count--;
        }
    }
    int tw, th;
    TTF_SizeUTF8(font, text.c_str(), &tw, &th);
    return (lines - 1) * std::max(th, TTF_FontLineSkip(font)) + th;
}

SDL_HitTestResult HitTestCallback(SDL_Window *window, const SDL_Point *pt, void *data) {
    (void)data;
    (void)window;
    if (pt->y < 30)
        return SDL_HITTEST_DRAGGABLE;
    return SDL_HITTEST_NORMAL;
}

void Cells::init() {
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    if (renderer_ != nullptr) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    SDL_SetHint("SDL_BORDERLESS_RESIZABLE_STYLE", "1");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
    window_ = SDL_CreateWindow("BingoTool",
                               gConfig.cellSize[0] * 5 + gConfig.cellSpacing * 4 + gConfig.cellBorder * 2,
                               gConfig.cellSize[1] * 5 + gConfig.cellSpacing * 4 + gConfig.cellBorder * 2
                                   + (gConfig.simpleMode ? (TTF_FontHeight(gConfig.scoreFont) + 5) : 0),
                               SDL_WINDOW_BORDERLESS | SDL_WINDOW_TRANSPARENT | SDL_WINDOW_ALWAYS_ON_TOP);
    SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    renderer_ = SDL_CreateRenderer(window_, "opengl", SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetWindowHitTest(window_, HitTestCallback, nullptr);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);

    fitCellsForText();
    for (auto &r : cells_) {
        for (auto &c : r) {
            c.renderer = renderer_;
            c.updateTexture();
        }
    }
    reloadColorTexture();
}

void Cells::deinit() {
    for (auto & tex : colorTexture_) {
        if (tex != nullptr) SDL_DestroyTexture(tex);
    }
    for (auto & tex : scoreTexture_) {
        if (tex != nullptr) SDL_DestroyTexture(tex);
    }
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);
}

void Cells::render() {
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 0);
    SDL_RenderClear(renderer_);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer_, gConfig.cellBorderColor.r, gConfig.cellBorderColor.g, gConfig.cellBorderColor.b, gConfig.cellBorderColor.a);
    SDL_FRect rcOuter = {0, 0, (float)(gConfig.cellSize[0] * 5 + gConfig.cellSpacing * 4 + gConfig.cellBorder * 2), (float)(gConfig.cellSize[1] * 5 + gConfig.cellSpacing * 4 + gConfig.cellBorder * 2)};
    SDL_RenderFillRect(renderer_, &rcOuter);
    SDL_SetRenderDrawColor(renderer_, gConfig.cellSpacingColor.r, gConfig.cellSpacingColor.g, gConfig.cellSpacingColor.b, gConfig.cellSpacingColor.a);
    SDL_FRect rcInner = {(float)gConfig.cellBorder, (float)gConfig.cellBorder, (float)(gConfig.cellSize[0] * 5 + gConfig.cellSpacing * 4), (float)(gConfig.cellSize[1] * 5 + gConfig.cellSpacing * 4)};
    SDL_RenderFillRect(renderer_, &rcInner);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
    auto cx = gConfig.cellSize[0], cy = gConfig.cellSize[1];
    for (int i = 0; i < 5; i++) {
        auto y = i * (cy + gConfig.cellSpacing) + gConfig.cellBorder;
        for (int j = 0; j < 5; j++) {
            auto x = j * (cx + gConfig.cellSpacing) + gConfig.cellBorder;
            auto &cell = gCells[i][j];
            cell.render(x, y, cx, cy);
        }
    }

    if (gConfig.simpleMode) {
        int ww, wh;
        SDL_GetWindowSize(window_, &ww, &wh);
        int fh = TTF_FontHeight(gConfig.scoreFont);
        int sw, sh;
        SDL_QueryTexture(scoreTexture_[0], nullptr, nullptr, &sw, &sh);
        SDL_FRect rect = {20.f, (float)(wh - fh + 5), (float)sw, (float)sh};
        SDL_RenderTexture(renderer_, scoreTexture_[0], nullptr, &rect);
        SDL_QueryTexture(scoreTexture_[1], nullptr, nullptr, &sw, &sh);
        rect = {(float)(ww - sw - 20), (float)(wh - fh + 5), (float)sw, (float)sh};
        SDL_RenderTexture(renderer_, scoreTexture_[1], nullptr, &rect);
    }

    SDL_RenderPresent(renderer_);
}

void Cells::switchMode() {
    resizeWindow();
}

void Cells::fitCellsForText() {
    gConfig.cellSize[0] = gConfig.originCellSizeX;
    if (gConfig.fontSize != gConfig.originalFontSize) {
        TTF_CloseFont(gConfig.font);
        gConfig.font = TTF_OpenFont(gConfig.fontFile.c_str(), gConfig.originalFontSize);
        TTF_SetFontStyle(gConfig.font, gConfig.fontStyle);
        TTF_SetFontWrappedAlign(gConfig.font, TTF_WRAPPED_ALIGN_CENTER);
        gConfig.fontSize = gConfig.originalFontSize;
    }
    switch (gConfig.cellAutoFit) {
        case 1: {
            auto *font = gConfig.font;
            auto fontSize = gConfig.fontSize;
            auto maxHeight = gConfig.cellSize[1] - 2;
            while (fontSize > 11) {
                bool fit = true;
                for (int i = 0; i < 25; i++) {
                    auto &cell = cells_[i / 5][i % 5];
                    int h = cell.calculateMinimalHeight(font);
                    if (h > maxHeight) {
                        fit = false;
                        break;
                    }
                }
                if (fit) {
                    if (font != gConfig.font) {
                        TTF_CloseFont(gConfig.font);
                        gConfig.font = font;
                        gConfig.fontSize = fontSize;
                    }
                    break;
                }
                if (font != gConfig.font) {
                    TTF_CloseFont(font);
                }
                fontSize--;
                font = TTF_OpenFont(gConfig.fontFile.c_str(), fontSize);
                TTF_SetFontStyle(font, gConfig.fontStyle);
                TTF_SetFontWrappedAlign(font, TTF_WRAPPED_ALIGN_CENTER);
            }
            break;
        }
        case 2: {
            int minWidth = gConfig.cellSize[0];
            foreach([&minWidth](Cell &cell, int, int) {
                int w = cell.calculateMinimalWidth(gConfig.font);
                if (w > minWidth) minWidth = w;
            });
            if (minWidth > gConfig.cellSize[0]) {
                gConfig.cellSize[0] = minWidth;
                gCells.resizeWindow();
            }
            break;
        }
        default: break;
    }
}

void Cells::foreach(const std::function<void(Cell &, int, int)> &callback) {
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            callback(cells_[y][x], x, y);
        }
    }
}

void Cells::resizeWindow() {
    SDL_SetWindowSize(window_,
                      gConfig.cellSize[0] * 5 + gConfig.cellSpacing * 4 + gConfig.cellBorder * 2,
                      gConfig.cellSize[1] * 5 + gConfig.cellSpacing * 4 + gConfig.cellBorder * 2
                      + (gConfig.simpleMode ? (TTF_FontHeight(gConfig.scoreFont) + 5) : 0));
}

void Cells::updateTextures(bool fit) {
    if (fit) fitCellsForText();
    foreach([](Cell &cell, int, int) {
        cell.updateTexture();
    });
}

void Cells::updateScoreTextures() {
    updateScoreTextures(0);
    updateScoreTextures(1);
}

void Cells::updateScoreTextures(int index) {
    auto score = gScoreWindows[index].score;
    const auto &playerName = gScoreWindows[index].playerName;
    std::string utext =
        fmt::format(gConfig.bingoBrawlersMode ? score >= 100 ? gConfig.scoreBingoText : score >= 13 ? gConfig.scoreWinText : "{1}" : "{1}",
                    playerName,
                    score);
    TTF_Font *ufont = gConfig.scoreFont;
    int ushadow = gConfig.scoreTextShadow;
    SDL_Color ushadowColor = gConfig.scoreTextShadowColor;
    int *ushadowOffset = gConfig.scoreTextShadowOffset;
    SDL_Surface *usurface;
    if (gConfig.useColorTexture[index]) {
        SDL_Color clr = {255, 255, 255, 255};
        usurface = TTF_RenderUTF8_BlackOutline_Wrapped(ufont, utext.c_str(), &clr, 0, &ushadowColor, ushadow, ushadowOffset);
    } else {
        usurface = TTF_RenderUTF8_BlackOutline_Wrapped(ufont,
                                                       utext.c_str(),
                                                       &gConfig.colorsInt[index + 1],
                                                       0,
                                                       &ushadowColor,
                                                       ushadow,
                                                       ushadowOffset);
    }
    SDL_Texture *utexture;
    if (usurface) {
        utexture = SDL_CreateTextureFromSurface(renderer_, usurface);
    } else {
        utexture = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 8, 8);
    }
    int mw, mh;
    SDL_QueryTexture(utexture, nullptr, nullptr, &mw, &mh);
    if (scoreTexture_[index])
        SDL_DestroyTexture(scoreTexture_[index]);
    scoreTexture_[index] = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, mw, mh);
    SDL_SetRenderTarget(renderer_, scoreTexture_[index]);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 0);
    SDL_RenderClear(renderer_);
    SDL_FRect rc = {0, 0, (float)mw, (float)mh};
    SDL_RenderTexture(renderer_, utexture, nullptr, &rc);
    if (gConfig.useColorTexture[index]) {
        SDL_SetTextureBlendMode(colorTexture_[index], SDL_BLENDMODE_MOD);
        SDL_RenderTexture(renderer_, colorTexture_[index], nullptr, nullptr);
        SDL_SetTextureBlendMode(colorTexture_[index], SDL_BLENDMODE_NONE);
    }
    SDL_DestroyTexture(utexture);
    SDL_DestroySurface(usurface);
    SDL_SetRenderTarget(renderer_, nullptr);
    SDL_SetTextureBlendMode(scoreTexture_[index], SDL_BLENDMODE_BLEND);
}

void Cells::reloadColorTexture() {
    reloadColorTexture(0);
    reloadColorTexture(1);
}

void Cells::reloadColorTexture(int index) {
    if (!gConfig.useColorTexture[index] || gConfig.colorTextureFile[index].empty()) return;
    colorTexture_[index] = loadTexture(renderer_, gConfig.colorTextureFile[index].c_str());
    if (!colorTexture_[index]) gConfig.useColorTexture[index] = false;
}

static void setEditUpDownIntAndRange(HWND hwnd, int id, int newVal, int min, int max) {
    noEnChangeNotification = true;
    SetWindowTextW(GetDlgItem(hwnd, id), std::to_wstring(newVal).c_str());
    noEnChangeNotification = false;
    SendDlgItemMessageW(hwnd, id + 1, UDM_SETRANGE32, min, max);
}

void initConfigDialog(HWND hwnd) {
    if (textColorBrush) {
        DeleteObject(textColorBrush);
    }
    if (backgroundColorBrush) {
        DeleteObject(backgroundColorBrush);
    }
    if (textShadowColorBrush) {
        DeleteObject(textShadowColorBrush);
    }
    if (cellBorderColorBrush) {
        DeleteObject(cellBorderColorBrush);
    }
    if (cellSpacingColorBrush) {
        DeleteObject(cellSpacingColorBrush);
    }
    if (scoreShadowColorBrush) {
        DeleteObject(scoreShadowColorBrush);
    }
    if (scoreBackgroundColorBrush) {
        DeleteObject(scoreBackgroundColorBrush);
    }
    if (nameShadowColorBrush) {
        DeleteObject(nameShadowColorBrush);
    }
    if (player1ColorBrush) {
        DeleteObject(player1ColorBrush);
    }
    if (player2ColorBrush) {
        DeleteObject(player2ColorBrush);
    }
    if (player1Bitmap) {
        DeleteObject(player1Bitmap);
    }
    if (player2Bitmap) {
        DeleteObject(player2Bitmap);
    }
    textColorBrush = CreateSolidBrush(RGB(gConfig.textColor.r, gConfig.textColor.g, gConfig.textColor.b));
    backgroundColorBrush = CreateSolidBrush(RGB(gConfig.colorsInt[0].r, gConfig.colorsInt[0].g, gConfig.colorsInt[0].b));
    textShadowColorBrush = CreateSolidBrush(RGB(gConfig.textShadowColor.r, gConfig.textShadowColor.g, gConfig.textShadowColor.b));
    cellBorderColorBrush = CreateSolidBrush(RGB(gConfig.cellBorderColor.r, gConfig.cellBorderColor.g, gConfig.cellBorderColor.b));
    cellSpacingColorBrush = CreateSolidBrush(RGB(gConfig.cellSpacingColor.r, gConfig.cellSpacingColor.g, gConfig.cellSpacingColor.b));
    scoreShadowColorBrush = CreateSolidBrush(RGB(gConfig.scoreTextShadowColor.r, gConfig.scoreTextShadowColor.g, gConfig.scoreTextShadowColor.b));
    scoreBackgroundColorBrush = CreateSolidBrush(RGB(gConfig.scoreBackgroundColor.r, gConfig.scoreBackgroundColor.g, gConfig.scoreBackgroundColor.b));
    nameShadowColorBrush = CreateSolidBrush(RGB(gConfig.scoreNameTextShadowColor.r, gConfig.scoreNameTextShadowColor.g, gConfig.scoreNameTextShadowColor.b));
    player1ColorBrush = CreateSolidBrush(RGB(gConfig.colorsInt[1].r, gConfig.colorsInt[1].g, gConfig.colorsInt[1].b));
    player2ColorBrush = CreateSolidBrush(RGB(gConfig.colorsInt[2].r, gConfig.colorsInt[2].g, gConfig.colorsInt[2].b));
    if (gConfig.useColorTexture[0]) player1Bitmap = loadBitmap(gConfig.colorTextureFile[0].c_str());
    if (gConfig.useColorTexture[1]) player2Bitmap = loadBitmap(gConfig.colorTextureFile[1].c_str());

    int x, y;
    SDL_GetWindowPosition(gCells.window(), &x, &y);
    noEnChangeNotification = true;
    SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    HWND edc = GetDlgItem(hwnd, IDC_TEXTFONT);
    auto fontFileName = Utf8ToUnicode(gConfig.fontFile);
    SetWindowTextW(edc, fontFileName.c_str());

    setEditUpDownIntAndRange(hwnd, IDC_TEXTSIZE, gConfig.originalFontSize, 1, 256);
    setEditUpDownIntAndRange(hwnd, IDC_TEXTCOLORA, gConfig.textColor.a, 0, 255);
    setEditUpDownIntAndRange(hwnd, IDC_SHADOW, gConfig.textShadow, 0, 10);
    setEditUpDownIntAndRange(hwnd, IDC_SHADOWOFFSET_X, gConfig.textShadowOffset[0], -20, 20);
    setEditUpDownIntAndRange(hwnd, IDC_SHADOWOFFSET_Y, gConfig.textShadowOffset[1], -20, 20);
    setEditUpDownIntAndRange(hwnd, IDC_SHADOWCOLORA, gConfig.textShadowColor.a, 0, 255);
    setEditUpDownIntAndRange(hwnd, IDC_BGCOLORA, gConfig.colorsInt[0].a, 0, 255);
    setEditUpDownIntAndRange(hwnd, IDC_BORDER, gConfig.cellBorder, 0, 100);
    setEditUpDownIntAndRange(hwnd, IDC_CELLSPACING, gConfig.cellSpacing, 0, 100);
    setEditUpDownIntAndRange(hwnd, IDC_CELLWIDTH, gConfig.originCellSizeX, 1, 1000);
    setEditUpDownIntAndRange(hwnd, IDC_CELLHEIGHT, gConfig.cellSize[1], 1, 1000);

    HWND cbc = GetDlgItem(hwnd, IDC_AUTOSIZE);
    SendMessageW(cbc, CB_ADDSTRING, 0, (LPARAM)L"自动缩小文字");
    SendMessageW(cbc, CB_ADDSTRING, 0, (LPARAM)L"统一缩小文字");
    SendMessageW(cbc, CB_ADDSTRING, 0, (LPARAM)L"统一扩展宽度");
    SendMessageW(cbc, CB_SETCURSEL, gConfig.cellAutoFit, 0);
    cbc = GetDlgItem(hwnd, IDC_PLAYER1USETEXTURE);
    SendMessageW(cbc, BM_SETCHECK, gConfig.useColorTexture[0] ? BST_CHECKED : BST_UNCHECKED, 0);
    cbc = GetDlgItem(hwnd, IDC_PLAYER2USETEXTURE);
    SendMessageW(cbc, BM_SETCHECK, gConfig.useColorTexture[1] ? BST_CHECKED : BST_UNCHECKED, 0);

    edc = GetDlgItem(hwnd, IDC_SCOREFONT);
    fontFileName = Utf8ToUnicode(gConfig.scoreFontFile);
    SetWindowTextW(edc, fontFileName.c_str());
    setEditUpDownIntAndRange(hwnd, IDC_SCORESIZE, gConfig.scoreFontSize, 1, 256);
    setEditUpDownIntAndRange(hwnd, IDC_SCORESHADOW, gConfig.scoreTextShadow, 0, 10);
    setEditUpDownIntAndRange(hwnd, IDC_SCORESHADOWOFFSET_X, gConfig.scoreTextShadowOffset[0], -20, 20);
    setEditUpDownIntAndRange(hwnd, IDC_SCORESHADOWOFFSET_Y, gConfig.scoreTextShadowOffset[1], -20, 20);
    setEditUpDownIntAndRange(hwnd, IDC_SCORESHADOWCOLORA, gConfig.scoreTextShadowColor.a, 0, 255);
    setEditUpDownIntAndRange(hwnd, IDC_SCOREBGCOLORA, gConfig.scoreBackgroundColor.a, 0, 255);
    edc = GetDlgItem(hwnd, IDC_SCOREWINTEXT);
    SetWindowTextW(edc, Utf8ToUnicode(gConfig.scoreWinText).c_str());
    edc = GetDlgItem(hwnd, IDC_SCOREBINGOTEXT);
    SetWindowTextW(edc, Utf8ToUnicode(gConfig.scoreBingoText).c_str());

    edc = GetDlgItem(hwnd, IDC_NAMEFONT);
    fontFileName = Utf8ToUnicode(gConfig.scoreNameFontFile);
    SetWindowTextW(edc, fontFileName.c_str());
    setEditUpDownIntAndRange(hwnd, IDC_NAMESIZE, gConfig.scoreNameFontSize, 1, 256);
    setEditUpDownIntAndRange(hwnd, IDC_NAMESHADOW, gConfig.scoreNameTextShadow, 0, 10);
    setEditUpDownIntAndRange(hwnd, IDC_NAMESHADOWOFFSET_X, gConfig.scoreNameTextShadowOffset[0], -20, 20);
    setEditUpDownIntAndRange(hwnd, IDC_NAMESHADOWOFFSET_Y, gConfig.scoreNameTextShadowOffset[1], -20, 20);
    setEditUpDownIntAndRange(hwnd, IDC_NAMESHADOWCOLORA, gConfig.scoreNameTextShadowColor.a, 0, 255);
    edc = GetDlgItem(hwnd, IDC_NAMEWINTEXT);
    SetWindowTextW(edc, Utf8ToUnicode(gConfig.scoreNameWinText).c_str());
    edc = GetDlgItem(hwnd, IDC_NAMEBINGOTEXT);
    SetWindowTextW(edc, Utf8ToUnicode(gConfig.scoreNameBingoText).c_str());
    edc = GetDlgItem(hwnd, IDC_PLAYER1NAME);
    SetWindowTextW(edc, gConfig.playerName[0].c_str());
    edc = GetDlgItem(hwnd, IDC_PLAYER2NAME);
    SetWindowTextW(edc, gConfig.playerName[1].c_str());

    noEnChangeNotification = false;
}

std::wstring openDialogForFontFileSelection(HWND hwnd) {
    IFileOpenDialog *pFileOpen;
    auto hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void **>(&pFileOpen));
    wchar_t szFile[MAX_PATH] = {0};
    if (SUCCEEDED(hr)) {
        pFileOpen->SetOptions(FOS_ALLNONSTORAGEITEMS);
        hr = pFileOpen->Show(hwnd);
        if (SUCCEEDED(hr)) {
            IShellItem *pItem;
            hr = pFileOpen->GetResult(&pItem);
            if (SUCCEEDED(hr)) {
                PWSTR pszFilePath;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                if (SUCCEEDED(hr)) {
                    lstrcpyW(szFile, pszFilePath);
                    CoTaskMemFree(pszFilePath);
                } else {
                    SFGAOF attr;
                    pItem->GetAttributes(0xFFFFFFFFU, &attr);
                    if (!(attr & SFGAO_CANMONIKER)) {
                        IDataObject *pDataObject;
                        hr = pItem->BindToHandler(nullptr, BHID_DataObject, IID_IDataObject, (void **)&pDataObject);
                        if (SUCCEEDED(hr)) {
                            FORMATETC fmt = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
                            STGMEDIUM stg = {TYMED_HGLOBAL};
                            hr = pDataObject->GetData(&fmt, &stg);
                            if (SUCCEEDED(hr)) {
                                auto hDrop = (HDROP)GlobalLock(stg.hGlobal);
                                if (hDrop) {
                                    DragQueryFileW(hDrop, 0, szFile, MAX_PATH);
                                    GlobalUnlock(stg.hGlobal);
                                }
                                ReleaseStgMedium(&stg);
                            }
                            pDataObject->Release();
                        }
                    }
                }
                pItem->Release();
            }
        }
        pFileOpen->Release();
    }
    return szFile;
}

bool chooseColor(HWND hwnd, SDL_Color &color, COLORREF custColors[16]) {
    CHOOSECOLORW cc = {sizeof(CHOOSECOLORW)};
    cc.hInstance = (HWND)GetModuleHandleW(nullptr);
    cc.hwndOwner = hwnd;
    cc.lpCustColors = custColors;
    cc.rgbResult = RGB(color.r, color.g, color.b);
    cc.Flags = CC_RGBINIT | CC_FULLOPEN | CC_ANYCOLOR;
    if (!ChooseColorW(&cc)) return false;
    auto r = GetRValue(cc.rgbResult);
    auto g = GetGValue(cc.rgbResult);
    auto b = GetBValue(cc.rgbResult);
    if (color.r == r && color.g == g && color.b == b) return false;
    color.r = r;
    color.g = g;
    color.b = b;
    return true;
}

INT_PTR handleButtonClick(HWND hwnd, unsigned int id, LPARAM lParam) {
    static COLORREF custColors[16] = {0};
    switch (id) {
        case IDOK:
        case IDCANCEL: {
            gConfig.save();
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        case IDC_BTN_SEL_TEXTFONT: {
            auto fontSelected = openDialogForFontFileSelection(hwnd);
            if (fontSelected.empty()) return 0;
            wchar_t szFileRel[MAX_PATH] = {0};
            wchar_t szCurrentPath[MAX_PATH];
            GetModuleFileNameW(nullptr, szCurrentPath, MAX_PATH);
            PathRemoveFileSpecW(szCurrentPath);
            PathRelativePathToW(szFileRel, szCurrentPath, FILE_ATTRIBUTE_NORMAL, fontSelected.c_str(), FILE_ATTRIBUTE_NORMAL);
            if (szFileRel[0] == 0 || szFileRel[0] == '.') {
                lstrcpyW(szFileRel, fontSelected.c_str());
            }
            for (int i = 0; szFileRel[i]; i++) {
                if (szFileRel[i] == '\\') szFileRel[i] = '/';
            }
            auto fontFileName = UnicodeToUtf8(szFileRel);
            if (strcasecmp(fontFileName.c_str(), gConfig.fontFile.c_str()) != 0) {
                gConfig.fontFile = fontFileName;
                auto newFont = TTF_OpenFont(fontFileName.c_str(), gConfig.fontSize);
                if (newFont == nullptr) {
                    MessageBoxW(hwnd, L"Failed to load font", L"Error", MB_OK);
                } else {
                    if (gConfig.font) {
                        TTF_CloseFont(gConfig.font);
                    }
                    gConfig.font = newFont;
                    gCells.updateTextures();
                    auto edc = GetDlgItem(hwnd, IDC_TEXTFONT);
                    SetWindowTextW(edc, szFileRel);
                }
            }
            return 0;
        }
        case IDC_TEXTCOLOR: {
            if (!chooseColor(hwnd, gConfig.textColor, custColors)) return false;
            if (textColorBrush) {
                DeleteObject(textColorBrush);
            }
            textColorBrush = CreateSolidBrush(RGB(gConfig.textColor.r, gConfig.textColor.g, gConfig.textColor.b));
            InvalidateRect(GetDlgItem(hwnd, IDC_TEXTCOLOR), nullptr, TRUE);
            gCells.updateTextures(false);
            break;
        }
        case IDC_BGCOLOR: {
            if (!chooseColor(hwnd, gConfig.colorsInt[0], custColors)) return false;
            if (backgroundColorBrush) {
                DeleteObject(backgroundColorBrush);
            }
            backgroundColorBrush = CreateSolidBrush(RGB(gConfig.colorsInt[0].r, gConfig.colorsInt[0].g, gConfig.colorsInt[0].b));
            InvalidateRect(GetDlgItem(hwnd, IDC_BGCOLOR), nullptr, TRUE);
            break;
        }
        case IDC_SHADOWCOLOR: {
            if (!chooseColor(hwnd, gConfig.textShadowColor, custColors)) return false;
            if (textShadowColorBrush) {
                DeleteObject(textShadowColorBrush);
            }
            textShadowColorBrush = CreateSolidBrush(RGB(gConfig.textShadowColor.r, gConfig.textShadowColor.g, gConfig.textShadowColor.b));
            InvalidateRect(GetDlgItem(hwnd, IDC_SHADOWCOLOR), nullptr, TRUE);
            gCells.updateTextures(false);
            break;
        }
        case IDC_BORDERCOLOR: {
            if (!chooseColor(hwnd, gConfig.cellBorderColor, custColors)) return false;
            if (cellBorderColorBrush) {
                DeleteObject(cellBorderColorBrush);
            }
            cellBorderColorBrush = CreateSolidBrush(RGB(gConfig.cellBorderColor.r, gConfig.cellBorderColor.g, gConfig.cellBorderColor.b));
            InvalidateRect(GetDlgItem(hwnd, IDC_BORDERCOLOR), nullptr, TRUE);
            break;
        }
        case IDC_SPACINGCOLOR: {
            if (!chooseColor(hwnd, gConfig.cellSpacingColor, custColors)) return false;
            if (cellSpacingColorBrush) {
                DeleteObject(cellSpacingColorBrush);
            }
            cellSpacingColorBrush = CreateSolidBrush(RGB(gConfig.cellSpacingColor.r, gConfig.cellSpacingColor.g, gConfig.cellSpacingColor.b));
            InvalidateRect(GetDlgItem(hwnd, IDC_SPACINGCOLOR), nullptr, TRUE);
            break;
        }
        case IDC_BTN_SEL_SCOREFONT: {
            auto fontSelected = openDialogForFontFileSelection(hwnd);
            if (fontSelected.empty()) return 0;
            wchar_t szFileRel[MAX_PATH] = {0};
            wchar_t szCurrentPath[MAX_PATH];
            GetModuleFileNameW(nullptr, szCurrentPath, MAX_PATH);
            PathRemoveFileSpecW(szCurrentPath);
            PathRelativePathToW(szFileRel, szCurrentPath, FILE_ATTRIBUTE_NORMAL, fontSelected.c_str(), FILE_ATTRIBUTE_NORMAL);
            if (szFileRel[0] == 0 || szFileRel[0] == '.') {
                lstrcpyW(szFileRel, fontSelected.c_str());
            }
            for (int i = 0; szFileRel[i]; i++) {
                if (szFileRel[i] == '\\') szFileRel[i] = '/';
            }
            auto fontFileName = UnicodeToUtf8(szFileRel);
            if (strcasecmp(fontFileName.c_str(), gConfig.scoreFontFile.c_str()) != 0) {
                gConfig.scoreFontFile = fontFileName;
                auto newFont = TTF_OpenFont(fontFileName.c_str(), gConfig.scoreFontSize);
                if (newFont == nullptr) {
                    MessageBoxW(hwnd, L"Failed to load font", L"Error", MB_OK);
                } else {
                    if (gConfig.scoreFont) {
                        TTF_CloseFont(gConfig.scoreFont);
                    }
                    gConfig.scoreFont = newFont;
                    if (gConfig.simpleMode)
                        gCells.resizeWindow();
                    gScoreWindows[0].updateTexture();
                    gScoreWindows[1].updateTexture();
                    auto edc = GetDlgItem(hwnd, IDC_SCOREFONT);
                    SetWindowTextW(edc, szFileRel);
                }
            }
            return 0;
        }
        case IDC_SCORESHADOWCOLOR: {
            if (!chooseColor(hwnd, gConfig.scoreTextShadowColor, custColors)) return false;
            if (scoreShadowColorBrush) {
                DeleteObject(scoreShadowColorBrush);
            }
            scoreShadowColorBrush = CreateSolidBrush(RGB(gConfig.scoreTextShadowColor.r, gConfig.scoreTextShadowColor.g, gConfig.scoreTextShadowColor.b));
            InvalidateRect(GetDlgItem(hwnd, IDC_SCORESHADOWCOLOR), nullptr, TRUE);
            break;
        }
        case IDC_SCOREBGCOLOR: {
            if (!chooseColor(hwnd, gConfig.scoreBackgroundColor, custColors)) return false;
            if (scoreBackgroundColorBrush) {
                DeleteObject(scoreBackgroundColorBrush);
            }
            scoreBackgroundColorBrush = CreateSolidBrush(RGB(gConfig.scoreBackgroundColor.r, gConfig.scoreBackgroundColor.g, gConfig.scoreBackgroundColor.b));
            InvalidateRect(GetDlgItem(hwnd, IDC_SCOREBGCOLOR), nullptr, TRUE);
            break;
        }
        case IDC_BTN_SEL_NAMEFONT: {
            auto fontSelected = openDialogForFontFileSelection(hwnd);
            if (fontSelected.empty()) return 0;
            wchar_t szFileRel[MAX_PATH] = {0};
            wchar_t szCurrentPath[MAX_PATH];
            GetModuleFileNameW(nullptr, szCurrentPath, MAX_PATH);
            PathRemoveFileSpecW(szCurrentPath);
            PathRelativePathToW(szFileRel, szCurrentPath, FILE_ATTRIBUTE_NORMAL, fontSelected.c_str(), FILE_ATTRIBUTE_NORMAL);
            if (szFileRel[0] == 0 || szFileRel[0] == '.') {
                lstrcpyW(szFileRel, fontSelected.c_str());
            }
            for (int i = 0; szFileRel[i]; i++) {
                if (szFileRel[i] == '\\') szFileRel[i] = '/';
            }
            auto fontFileName = UnicodeToUtf8(szFileRel);
            if (strcasecmp(fontFileName.c_str(), gConfig.scoreNameFontFile.c_str()) != 0) {
                gConfig.scoreNameFontFile = fontFileName;
                auto newFont = TTF_OpenFont(fontFileName.c_str(), gConfig.scoreNameFontSize);
                if (newFont == nullptr) {
                    MessageBoxW(hwnd, L"Failed to load font", L"Error", MB_OK);
                } else {
                    if (gConfig.scoreNameFont) {
                        TTF_CloseFont(gConfig.scoreNameFont);
                    }
                    gConfig.scoreNameFont = newFont;
                    gScoreWindows[0].updateTexture();
                    gScoreWindows[1].updateTexture();
                    auto edc = GetDlgItem(hwnd, IDC_NAMEFONT);
                    SetWindowTextW(edc, szFileRel);
                }
            }
            return 0;
        }
        case IDC_NAMESHADOWCOLOR: {
            if (!chooseColor(hwnd, gConfig.scoreNameTextShadowColor, custColors)) return false;
            if (nameShadowColorBrush) {
                DeleteObject(nameShadowColorBrush);
            }
            nameShadowColorBrush = CreateSolidBrush(RGB(gConfig.scoreNameTextShadowColor.r, gConfig.scoreNameTextShadowColor.g, gConfig.scoreNameTextShadowColor.b));
            InvalidateRect(GetDlgItem(hwnd, IDC_NAMESHADOWCOLOR), nullptr, TRUE);
            break;
        }
        case IDC_AUTOSIZE: {
            auto cbc = GetDlgItem(hwnd, IDC_AUTOSIZE);
            gConfig.cellAutoFit = (int)SendMessageW(cbc, CB_GETCURSEL, 0, 0);
            gCells.updateTextures();
            break;
        }
        case IDC_PLAYER1COLOR: {
            if (!chooseColor(hwnd, gConfig.colorsInt[1], custColors)) return false;
            if (player1ColorBrush) {
                DeleteObject(player1ColorBrush);
            }
            player1ColorBrush = CreateSolidBrush(RGB(gConfig.colorsInt[1].r, gConfig.colorsInt[1].g, gConfig.colorsInt[1].b));
            InvalidateRect(GetDlgItem(hwnd, IDC_PLAYER1COLOR), nullptr, TRUE);
            break;
        }
        case IDC_PLAYER1USETEXTURE: {
            auto cbc = GetDlgItem(hwnd, IDC_PLAYER1USETEXTURE);
            gConfig.useColorTexture[0] = SendMessageW(cbc, BM_GETCHECK, 0, 0) == BST_CHECKED;
            gCells.updateTextures();
            gScoreWindows[0].updateTexture();
            break;
        }
        case IDC_PLAYER1TEXTURE: {
            auto file = selectFile(hwnd, L"选择贴图文件", L"", L"PNG文件|*.png|BMP文件|*.bmp", false, true);
            if (file.empty()) break;
            auto filename = UnicodeToUtf8(file);
            if (gConfig.colorTextureFile[0] == filename) break;
            gConfig.colorTextureFile[0] = filename;
            if (player1Bitmap) {
                DeleteObject(player1Bitmap);
            }
            player1Bitmap = loadBitmap(filename.c_str());
            InvalidateRect(GetDlgItem(hwnd, IDC_PLAYER1TEXTURE), nullptr, TRUE);
            gCells.reloadColorTexture(0);
            gCells.updateTextures();
            gScoreWindows[0].updateTexture(true);
            break;
        }
        case IDC_PLAYER2COLOR: {
            if (!chooseColor(hwnd, gConfig.colorsInt[2], custColors)) return false;
            if (player2ColorBrush) {
                DeleteObject(player2ColorBrush);
            }
            player2ColorBrush = CreateSolidBrush(RGB(gConfig.colorsInt[2].r, gConfig.colorsInt[2].g, gConfig.colorsInt[2].b));
            InvalidateRect(GetDlgItem(hwnd, IDC_PLAYER2COLOR), nullptr, TRUE);
            break;
        }
        case IDC_PLAYER2USETEXTURE: {
            auto cbc = GetDlgItem(hwnd, IDC_PLAYER2USETEXTURE);
            gConfig.useColorTexture[1] = SendMessageW(cbc, BM_GETCHECK, 0, 0) == BST_CHECKED;
            gCells.updateTextures();
            gScoreWindows[1].updateTexture();
            break;
        }
        case IDC_PLAYER2TEXTURE: {
            auto file = selectFile(hwnd, L"选择贴图文件", L"", L"PNG文件|*.png|BMP文件|*.bmp", false, true);
            if (file.empty()) break;
            auto filename = UnicodeToUtf8(file);
            if (gConfig.colorTextureFile[1] == filename) break;
            gConfig.colorTextureFile[1] = filename;
            if (player2Bitmap) {
                DeleteObject(player2Bitmap);
            }
            player2Bitmap = loadBitmap(filename.c_str());
            InvalidateRect(GetDlgItem(hwnd, IDC_PLAYER2TEXTURE), nullptr, TRUE);
            gCells.reloadColorTexture(1);
            gCells.updateTextures();
            gScoreWindows[1].updateTexture(true);
            break;
        }
        default:
            break;
    }
    return 0;
}

template<typename T>
static void setNewValFromControl(HWND hwnd, int id, T &val, int min, int max, const std::function<bool(int)> &postFunc = nullptr) {
    wchar_t str[16];
    auto hwndCtl = GetDlgItem(hwnd, id);
    GetWindowTextW(hwndCtl, str, 16);
    if (str[0] == 0) return;
    auto newVal = std::clamp((int)std::wcstol(str, nullptr, 10), min, max);
    if (T(newVal) == val) return;
    auto oldVal = val;
    val = T(newVal);
    if (!postFunc || postFunc(oldVal)) {
        noEnChangeNotification = true;
        SetWindowTextW(hwndCtl, std::to_wstring(val).c_str());
        noEnChangeNotification = false;
    }
}

INT_PTR handleEditChange(HWND hwnd, unsigned int id, LPARAM lParam) {
    if (noEnChangeNotification)
        return DefWindowProcW(hwnd, WM_COMMAND, MAKEWPARAM(id, EN_CHANGE), lParam);
    switch (id) {
        case IDC_TEXTSIZE: {
            setNewValFromControl(hwnd, IDC_TEXTSIZE, gConfig.originalFontSize, 6, 256, [hwnd](int oldSize) {
                auto ft = TTF_OpenFont(gConfig.fontFile.c_str(), gConfig.originalFontSize);
                if (ft == nullptr) {
                    gConfig.originalFontSize = oldSize;
                    MessageBoxW(hwnd, L"Failed to change font size", L"Error", MB_OK);
                    return true;
                }
                gConfig.fontSize = gConfig.originalFontSize;
                TTF_SetFontStyle(ft, gConfig.fontStyle);
                TTF_SetFontWrappedAlign(ft, TTF_WRAPPED_ALIGN_CENTER);
                if (gConfig.font) {
                    TTF_CloseFont(gConfig.font);
                }
                gConfig.font = ft;
                gCells.updateTextures();
                return true;
            });
            break;
        }
        case IDC_TEXTCOLORA: {
            setNewValFromControl(hwnd, IDC_TEXTCOLORA, gConfig.textColor.a, 0, 255, [](int newVal) {
                gCells.updateTextures(false);
                return true;
            });
            break;
        }
        case IDC_SHADOW: {
            setNewValFromControl(hwnd, IDC_SHADOW, gConfig.textShadow, 0, 10, [hwnd](int newVal) {
                gCells.updateTextures(false);
                return true;
            });
            break;
        }
        case IDC_SHADOWOFFSET_X: {
            setNewValFromControl(hwnd, IDC_SHADOWOFFSET_X, gConfig.textShadowOffset[0], -20, 20, [hwnd](int newVal) {
                gCells.updateTextures(false);
                return true;
            });
            break;
        }
        case IDC_SHADOWOFFSET_Y: {
            setNewValFromControl(hwnd, IDC_SHADOWOFFSET_Y, gConfig.textShadowOffset[1], -20, 20, [hwnd](int newVal) {
                gCells.updateTextures(false);
                return true;
            });
            break;
        }
        case IDC_SHADOWCOLORA: {
            setNewValFromControl(hwnd, IDC_SHADOWCOLORA, gConfig.textShadowColor.a, 0, 255, [](int newVal) {
                gCells.updateTextures(false);
                return true;
            });
            break;
        }
        case IDC_BGCOLORA: {
            setNewValFromControl(hwnd, IDC_BGCOLORA, gConfig.colorsInt[0].a, 0, 255);
            break;
        }
        case IDC_BORDER: {
            setNewValFromControl(hwnd, IDC_BORDER, gConfig.cellBorder, 0, 100, [hwnd](int newVal) {
                gCells.resizeWindow();
                return true;
            });
            break;
        }
        case IDC_CELLSPACING: {
            setNewValFromControl(hwnd, IDC_CELLSPACING, gConfig.cellSpacing, 0, 100, [hwnd](int newVal) {
                gCells.resizeWindow();
                return true;
            });
            break;
        }
        case IDC_CELLWIDTH: {
            setNewValFromControl(hwnd, IDC_CELLWIDTH, gConfig.originCellSizeX, 20, 1000, [hwnd](int newVal) {
                gConfig.cellSize[0] = gConfig.originCellSizeX;
                gCells.resizeWindow();
                gCells.updateTextures();
                return true;
            });
            break;
        }
        case IDC_CELLHEIGHT: {
            setNewValFromControl(hwnd, IDC_CELLHEIGHT, gConfig.cellSize[1], 20, 1000, [hwnd](int newVal) {
                gCells.resizeWindow();
                gCells.updateTextures();
                return true;
            });
            break;
        }
        case IDC_SCORESIZE: {
            setNewValFromControl(hwnd, IDC_SCORESIZE, gConfig.scoreFontSize, 6, 256, [hwnd](int newVal) {
                auto ft = TTF_OpenFont(gConfig.scoreFontFile.c_str(), gConfig.scoreFontSize);
                if (ft == nullptr) {
                    MessageBoxW(hwnd, L"Failed to change font size", L"Error", MB_OK);
                    return true;
                }
                if (gConfig.scoreFont) {
                    TTF_CloseFont(gConfig.scoreFont);
                }
                gConfig.scoreFont = ft;
                if (gConfig.simpleMode)
                    gCells.resizeWindow();
                gScoreWindows[0].updateTexture();
                gScoreWindows[1].updateTexture();
                return true;
            });
            break;
        }
        case IDC_SCORESHADOW: {
            setNewValFromControl(hwnd, IDC_SCORESHADOW, gConfig.scoreTextShadow, 0, 10, [hwnd](int newVal) {
                gScoreWindows[0].updateTexture();
                gScoreWindows[1].updateTexture();
                return true;
            });
            break;
        }
        case IDC_SCORESHADOWOFFSET_X: {
            setNewValFromControl(hwnd, IDC_SCORESHADOWOFFSET_X, gConfig.scoreTextShadowOffset[0], -20, 20, [hwnd](int newVal) {
                gScoreWindows[0].updateTexture();
                gScoreWindows[1].updateTexture();
                return true;
            });
            break;
        }
        case IDC_SCORESHADOWOFFSET_Y: {
            setNewValFromControl(hwnd, IDC_SCORESHADOWOFFSET_Y, gConfig.scoreTextShadowOffset[1], -20, 20, [hwnd](int newVal) {
                gScoreWindows[0].updateTexture();
                gScoreWindows[1].updateTexture();
                return true;
            });
            break;
        }
        case IDC_SCORESHADOWCOLORA: {
            setNewValFromControl(hwnd, IDC_SCORESHADOWCOLORA, gConfig.scoreTextShadowColor.a, 0, 255, [hwnd](int newVal) {
                gScoreWindows[0].updateTexture();
                gScoreWindows[1].updateTexture();
                return true;
            });
            break;
        }
        case IDC_SCOREBGCOLORA: {
            setNewValFromControl(hwnd, IDC_SCOREBGCOLORA, gConfig.scoreBackgroundColor.a, 0, 255);
            break;
        }
        case IDC_SCOREWINTEXT: {
            wchar_t str[256];
            auto hwndCtl = GetDlgItem(hwnd, IDC_SCOREWINTEXT);
            GetWindowTextW(hwndCtl, str, 256);
            auto newText = UnicodeToUtf8(str);
            if (newText == gConfig.scoreWinText) return 0;
            gConfig.scoreWinText = newText;
            gScoreWindows[0].updateTexture();
            gScoreWindows[1].updateTexture();
            break;
        }
        case IDC_SCOREBINGOTEXT: {
            wchar_t str[256];
            auto hwndCtl = GetDlgItem(hwnd, IDC_SCOREBINGOTEXT);
            GetWindowTextW(hwndCtl, str, 256);
            auto newText = UnicodeToUtf8(str);
            if (newText == gConfig.scoreBingoText) return 0;
            gConfig.scoreBingoText = newText;
            gScoreWindows[0].updateTexture();
            gScoreWindows[1].updateTexture();
            break;
        }
        case IDC_NAMESIZE: {
            setNewValFromControl(hwnd, IDC_NAMESIZE, gConfig.scoreNameFontSize, 6, 256, [hwnd](int newVal) {
                auto ft = TTF_OpenFont(gConfig.scoreNameFontFile.c_str(), gConfig.scoreNameFontSize);
                if (ft == nullptr) {
                    MessageBoxW(hwnd, L"Failed to change font size", L"Error", MB_OK);
                    return true;
                }
                if (gConfig.scoreNameFont) {
                    TTF_CloseFont(gConfig.scoreNameFont);
                }
                gConfig.scoreNameFont = ft;
                gScoreWindows[0].updateTexture();
                gScoreWindows[1].updateTexture();
                return true;
            });
            break;
        }
        case IDC_NAMESHADOW: {
            setNewValFromControl(hwnd, IDC_NAMESHADOW, gConfig.scoreNameTextShadow, 0, 10, [hwnd](int newVal) {
                gScoreWindows[0].updateTexture();
                gScoreWindows[1].updateTexture();
                return true;
            });
            break;
        }
        case IDC_NAMESHADOWOFFSET_X: {
            setNewValFromControl(hwnd, IDC_NAMESHADOWOFFSET_X, gConfig.scoreNameTextShadowOffset[0], -20, 20, [hwnd](int newVal) {
                gScoreWindows[0].updateTexture();
                gScoreWindows[1].updateTexture();
                return true;
            });
            break;
        }
        case IDC_NAMESHADOWOFFSET_Y: {
            setNewValFromControl(hwnd, IDC_NAMESHADOWOFFSET_Y, gConfig.scoreNameTextShadowOffset[1], -20, 20, [hwnd](int newVal) {
                gScoreWindows[0].updateTexture();
                gScoreWindows[1].updateTexture();
                return true;
            });
            break;
        }
        case IDC_NAMESHADOWCOLORA: {
            setNewValFromControl(hwnd, IDC_NAMESHADOWCOLORA, gConfig.scoreNameTextShadowColor.a, 0, 255, [hwnd](int newVal) {
                gScoreWindows[0].updateTexture();
                gScoreWindows[1].updateTexture();
                return true;
            });
            break;
        }
        case IDC_NAMEWINTEXT: {
            wchar_t str[256];
            auto hwndCtl = GetDlgItem(hwnd, IDC_NAMEWINTEXT);
            GetWindowTextW(hwndCtl, str, 256);
            auto newText = UnicodeToUtf8(str);
            if (newText == gConfig.scoreNameWinText) return 0;
            gConfig.scoreNameWinText = newText;
            gScoreWindows[0].updateTexture();
            gScoreWindows[1].updateTexture();
            break;
        }
        case IDC_NAMEBINGOTEXT: {
            wchar_t str[256];
            auto hwndCtl = GetDlgItem(hwnd, IDC_NAMEBINGOTEXT);
            GetWindowTextW(hwndCtl, str, 256);
            auto newText = UnicodeToUtf8(str);
            if (newText == gConfig.scoreNameBingoText) return 0;
            gConfig.scoreNameBingoText = newText;
            gScoreWindows[0].updateTexture();
            gScoreWindows[1].updateTexture();
            break;
        }
        case IDC_PLAYER1NAME: {
            wchar_t str[256];
            auto hwndCtl = GetDlgItem(hwnd, IDC_PLAYER1NAME);
            GetWindowTextW(hwndCtl, str, 256);
            if (str == gConfig.playerName[0]) return 0;
            gConfig.playerName[0] = str;
            gScoreWindows[0].playerName = UnicodeToUtf8(str);
            gScoreWindows[0].updateTexture();
            break;
        }
        case IDC_PLAYER2NAME: {
            wchar_t str[256];
            auto hwndCtl = GetDlgItem(hwnd, IDC_PLAYER2NAME);
            GetWindowTextW(hwndCtl, str, 256);
            if (str == gConfig.playerName[1]) return 0;
            gConfig.playerName[1] = str;
            gScoreWindows[1].playerName = UnicodeToUtf8(str);
            gScoreWindows[1].updateTexture();
            break;
        }
        default:
            break;
    }
    return 0;
}

INT_PTR handleCtlColorStatic(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    switch (GetDlgCtrlID((HWND)lParam)) {
        case IDC_TEXTCOLOR: {
            SetBkMode((HDC)wParam, OPAQUE);
            SetBkColor((HDC)wParam, RGB(gConfig.textColor.r, gConfig.textColor.g, gConfig.textColor.b));
            return (INT_PTR)textColorBrush;
        }
        case IDC_SHADOWCOLOR: {
            SetBkMode((HDC)wParam, OPAQUE);
            SetBkColor((HDC)wParam, RGB(gConfig.textShadowColor.r, gConfig.textShadowColor.g, gConfig.textShadowColor.b));
            return (INT_PTR)textShadowColorBrush;
        }
        case IDC_BGCOLOR: {
            SetBkMode((HDC)wParam, OPAQUE);
            SetBkColor((HDC)wParam,
                       RGB(gConfig.colorsInt[0].r, gConfig.colorsInt[0].g, gConfig.colorsInt[0].b));
            return (INT_PTR)backgroundColorBrush;
        }
        case IDC_BORDERCOLOR: {
            SetBkMode((HDC)wParam, OPAQUE);
            SetBkColor((HDC)wParam, RGB(gConfig.cellBorderColor.r, gConfig.cellBorderColor.g, gConfig.cellBorderColor.b));
            return (INT_PTR)cellBorderColorBrush;
        }
        case IDC_SPACINGCOLOR: {
            SetBkMode((HDC)wParam, OPAQUE);
            SetBkColor((HDC)wParam, RGB(gConfig.cellSpacingColor.r, gConfig.cellSpacingColor.g, gConfig.cellSpacingColor.b));
            return (INT_PTR)cellSpacingColorBrush;
        }
        case IDC_SCORESHADOWCOLOR: {
            SetBkMode((HDC)wParam, OPAQUE);
            SetBkColor((HDC)wParam, RGB(gConfig.scoreTextShadowColor.r, gConfig.scoreTextShadowColor.g, gConfig.scoreTextShadowColor.b));
            return (INT_PTR)scoreShadowColorBrush;
        }
        case IDC_SCOREBGCOLOR: {
            SetBkMode((HDC)wParam, OPAQUE);
            SetBkColor((HDC)wParam, RGB(gConfig.scoreBackgroundColor.r, gConfig.scoreBackgroundColor.g, gConfig.scoreBackgroundColor.b));
            return (INT_PTR)scoreBackgroundColorBrush;
        }
        case IDC_NAMESHADOWCOLOR: {
            SetBkMode((HDC)wParam, OPAQUE);
            SetBkColor((HDC)wParam, RGB(gConfig.scoreNameTextShadowColor.r, gConfig.scoreNameTextShadowColor.g, gConfig.scoreNameTextShadowColor.b));
            return (INT_PTR)nameShadowColorBrush;
        }
        case IDC_PLAYER1COLOR: {
            SetBkMode((HDC)wParam, OPAQUE);
            SetBkColor((HDC)wParam, RGB(gConfig.colorsInt[1].r, gConfig.colorsInt[1].g, gConfig.colorsInt[1].b));
            return (INT_PTR)player1ColorBrush;
        }
        case IDC_PLAYER2COLOR: {
            SetBkMode((HDC)wParam, OPAQUE);
            SetBkColor((HDC)wParam, RGB(gConfig.colorsInt[2].r, gConfig.colorsInt[2].g, gConfig.colorsInt[2].b));
            return (INT_PTR)player2ColorBrush;
        }
        default:
            break;
    }
    return DefWindowProcW(hwnd, WM_CTLCOLORSTATIC, wParam, lParam);
}

INT_PTR handleDrawItem(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    auto lpdis = (LPDRAWITEMSTRUCT)lParam;
    switch (wParam) {
        case IDC_PLAYER1TEXTURE: {
            if (player1Bitmap) {
                auto hdc = CreateCompatibleDC(lpdis->hDC);
                SelectObject(hdc, player1Bitmap);
                BITMAP bm;
                GetObject(player1Bitmap, (int)sizeof(bm), &bm);
                StretchBlt(lpdis->hDC, 0, 0, lpdis->rcItem.right, lpdis->rcItem.bottom, hdc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
                DeleteDC(hdc);
            }
            return TRUE;
        }
        case IDC_PLAYER2TEXTURE: {
            if (player2Bitmap) {
                auto hdc = CreateCompatibleDC(lpdis->hDC);
                SelectObject(hdc, player2Bitmap);
                BITMAP bm;
                GetObject(player2Bitmap, (int)sizeof(bm), &bm);
                StretchBlt(lpdis->hDC, 0, 0, lpdis->rcItem.right, lpdis->rcItem.bottom, hdc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
                DeleteDC(hdc);
            }
            return TRUE;
        }
    }
    return DefWindowProcW(hwnd, WM_DRAWITEM, wParam, lParam);
}

INT_PTR WINAPI dlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG: {
            initConfigDialog(hwnd);
            return TRUE;
        }
        case WM_COMMAND:
            switch (HIWORD(wParam)) {
                case CBN_SELCHANGE:
                case BN_CLICKED: {
                    return handleButtonClick(hwnd, wParam & 0xFFFF, lParam);
                }
                case EN_CHANGE: {
                    return handleEditChange(hwnd, wParam & 0xFFFF, lParam);
                }
            }
            break;
        case WM_CTLCOLORSTATIC: {
            return handleCtlColorStatic(hwnd, wParam, lParam);
        }
        case WM_DRAWITEM: {
            return handleDrawItem(hwnd, wParam, lParam);
        }
        default:
            break;
    }
    return 0;
}

void Cells::showSettingsWindow() {
    if (configDialog) {
        initConfigDialog(configDialog);
    } else {
        auto hwnd = (HWND)SDL_GetProperty(SDL_GetWindowProperties(window_), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        configDialog = CreateDialogParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(129), hwnd, dlgProc, (LPARAM)this);
    }
    ShowWindow(configDialog, SW_SHOW);
    SetForegroundWindow(configDialog);
}
