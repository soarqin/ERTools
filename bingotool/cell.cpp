#include "cell.h"

#include "common.h"
#include "config.h"

#if defined(_MSC_VER)
#define strcasecmp _stricmp
#endif

#include <SDL3_gfxPrimitives.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <shobjidl.h>
#include <shlguid.h>
#include <shellapi.h>
#undef WIN32_LEAN_AND_MEAN

extern SDL_Window *gWindow;
Cells gCells;

static HWND configDialog = nullptr;
static bool noEnChangeNotification = true;
static HBRUSH textColorBrush = nullptr;
static HBRUSH backgroundColorBrush = nullptr;

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
    if (st > 0 && gConfig.colorTexture[st - 1]) {
        SDL_FRect dstrect = {fx, fy, fcx, fcy};
        SDL_RenderTexture(renderer, gConfig.colorTexture[st - 1], nullptr, &dstrect);
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

void Cells::init(SDL_Renderer *renderer) {
    fitCellsForText();
    for (auto &r : cells_) {
        for (auto &c : r) {
            c.renderer = renderer;
            c.updateTexture();
        }
    }
}

void Cells::fitCellsForText() {
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
                SDL_SetWindowSize(gWindow,
                                  gConfig.cellSize[0] * 5 + gConfig.cellSpacing * 4 + gConfig.cellBorder * 2,
                                  gConfig.cellSize[1] * 5 + gConfig.cellSpacing * 4 + gConfig.cellBorder * 2);
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

void Cells::updateTextures() {
    fitCellsForText();
    foreach([](Cell &cell, int, int) {
        cell.updateTexture();
    });
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
    textColorBrush = CreateSolidBrush(RGB(gConfig.textColor.r, gConfig.textColor.g, gConfig.textColor.b));
    backgroundColorBrush = CreateSolidBrush(RGB(gConfig.colorsInt[0].r, gConfig.colorsInt[0].g, gConfig.colorsInt[0].b));

    int x, y;
    SDL_GetWindowPosition(gWindow, &x, &y);
    noEnChangeNotification = true;
    SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    HWND edc = GetDlgItem(hwnd, 1001);
    auto fontFileName = Utf8ToUnicode(gConfig.fontFile);
    SetWindowTextW(edc, fontFileName.c_str());

    setEditUpDownIntAndRange(hwnd, 1003, gConfig.originalFontSize, 6, 256);
    setEditUpDownIntAndRange(hwnd, 1007, gConfig.textColor.a, 0, 255);
    setEditUpDownIntAndRange(hwnd, 1009, gConfig.colorsInt[0].a, 0, 255);
    setEditUpDownIntAndRange(hwnd, 1011, gConfig.cellBorder, 0, 100);
    setEditUpDownIntAndRange(hwnd, 1013, gConfig.cellSpacing, 0, 100);
    setEditUpDownIntAndRange(hwnd, 1015, gConfig.cellSize[0], 20, 1000);
    setEditUpDownIntAndRange(hwnd, 1017, gConfig.cellSize[1], 20, 1000);

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
        case 1002: {
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
                    auto edc = GetDlgItem(hwnd, 1001);
                    SetWindowTextW(edc, szFileRel);
                }
            }
            return 0;
        }
        case 1005: {
            if (!chooseColor(hwnd, gConfig.textColor, custColors)) return false;
            if (textColorBrush) {
                DeleteObject(textColorBrush);
            }
            textColorBrush = CreateSolidBrush(RGB(gConfig.textColor.r, gConfig.textColor.g, gConfig.textColor.b));
            InvalidateRect(GetDlgItem(hwnd, 1005), nullptr, TRUE);
            gCells.updateTextures();
            break;
        }
        case 1006: {
            CHOOSECOLORW cc = {sizeof(CHOOSECOLORW)};
            cc.hInstance = (HWND)GetModuleHandleW(nullptr);
            cc.hwndOwner = hwnd;
            cc.lpCustColors = custColors;
            cc.rgbResult = RGB(gConfig.colorsInt[0].r, gConfig.colorsInt[0].g, gConfig.colorsInt[0].b);
            cc.Flags = CC_RGBINIT | CC_FULLOPEN | CC_ANYCOLOR;
            if (!ChooseColorW(&cc)) break;
            auto r = GetRValue(cc.rgbResult);
            auto g = GetGValue(cc.rgbResult);
            auto b = GetBValue(cc.rgbResult);
            if (gConfig.colorsInt[0].r == r && gConfig.colorsInt[0].g == g
                && gConfig.colorsInt[0].b == b)
                break;
            gConfig.colorsInt[0].r = r;
            gConfig.colorsInt[0].g = g;
            gConfig.colorsInt[0].b = b;
            if (backgroundColorBrush) {
                DeleteObject(backgroundColorBrush);
            }
            backgroundColorBrush = CreateSolidBrush(cc.rgbResult);
            InvalidateRect(GetDlgItem(hwnd, 1006), nullptr, TRUE);
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
        case 1003: {
            setNewValFromControl(hwnd, 1003, gConfig.originalFontSize, 6, 256, [hwnd](int oldSize) {
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
        case 1007: {
            setNewValFromControl(hwnd, 1007, gConfig.textColor.a, 0, 255, [](int newVal) {
                gCells.updateTextures();
                return true;
            });
            break;
        }
        case 1009: {
            setNewValFromControl(hwnd, 1009, gConfig.colorsInt[0].a, 0, 255);
            break;
        }
        case 1011: {
            setNewValFromControl(hwnd, 1011, gConfig.cellBorder, 0, 100, [hwnd](int newVal) {
                SDL_SetWindowSize(gWindow,
                                  gConfig.cellSize[0] * 5 + gConfig.cellSpacing * 4 + gConfig.cellBorder * 2,
                                  gConfig.cellSize[1] * 5 + gConfig.cellSpacing * 4 + gConfig.cellBorder * 2);
                return true;
            });
            break;
        }
        case 1013: {
            setNewValFromControl(hwnd, 1013, gConfig.cellSpacing, 0, 100, [hwnd](int newVal) {
                SDL_SetWindowSize(gWindow,
                                  gConfig.cellSize[0] * 5 + gConfig.cellSpacing * 4 + gConfig.cellBorder * 2,
                                  gConfig.cellSize[1] * 5 + gConfig.cellSpacing * 4 + gConfig.cellBorder * 2);
                return true;
            });
            break;
        }
        case 1015: {
            setNewValFromControl(hwnd, 1015, gConfig.cellSize[0], 20, 1000, [hwnd](int newVal) {
                SDL_SetWindowSize(gWindow,
                                  gConfig.cellSize[0] * 5 + gConfig.cellSpacing * 4 + gConfig.cellBorder * 2,
                                  gConfig.cellSize[1] * 5 + gConfig.cellSpacing * 4 + gConfig.cellBorder * 2);
                gCells.updateTextures();
                return true;
            });
            break;
        }
        case 1017: {
            setNewValFromControl(hwnd, 1017, gConfig.cellSize[1], 20, 1000, [hwnd](int newVal) {
                SDL_SetWindowSize(gWindow,
                                  gConfig.cellSize[0] * 5 + gConfig.cellSpacing * 4 + gConfig.cellBorder * 2,
                                  gConfig.cellSize[1] * 5 + gConfig.cellSpacing * 4 + gConfig.cellBorder * 2);
                gCells.updateTextures();
                return true;
            });
            break;
        }
        default:
            break;
    }
    return 0;
}

INT_PTR handleCtlColorStatic(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    switch (GetDlgCtrlID((HWND)lParam)) {
        case 1005: {
            SetBkMode((HDC)wParam, OPAQUE);
            SetBkColor((HDC)wParam, RGB(gConfig.textColor.r, gConfig.textColor.g, gConfig.textColor.b));
            return (INT_PTR)textColorBrush;
        }
        case 1006: {
            SetBkMode((HDC)wParam, OPAQUE);
            SetBkColor((HDC)wParam,
                       RGB(gConfig.colorsInt[0].r, gConfig.colorsInt[0].g, gConfig.colorsInt[0].b));
            return (INT_PTR)backgroundColorBrush;
        }
        default:
            break;
    }
    return DefWindowProcW(hwnd, WM_CTLCOLORSTATIC, wParam, lParam);
}

INT_PTR WINAPI dlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG: {
            initConfigDialog(hwnd);
            return TRUE;
        }
        case WM_COMMAND:
            switch (HIWORD(wParam)) {
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
        default:
            break;
    }
    return 0;
}

void Cells::showSettingsWindow() {
    if (configDialog) {
        initConfigDialog(configDialog);
    } else {
        auto hwnd = (HWND)SDL_GetProperty(SDL_GetWindowProperties(gWindow), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        configDialog = CreateDialogParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(129), hwnd, dlgProc, (LPARAM)this);
    }
    ShowWindow(configDialog, SW_SHOW);
    SetForegroundWindow(configDialog);
}
