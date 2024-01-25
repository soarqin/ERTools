extern "C" {
#include <lauxlib.h>
#include <lualib.h>
#include <luapkgs.h>
}

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <toml.hpp>

#define WIN32_LEAN_AND_MEAN
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <shobjidl.h>
#undef WIN32_LEAN_AND_MEAN

#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>
#include <shlobj.h>
#include <commdlg.h>

#include "images.inc"

enum : int {
    MOUSE_GRAB_PADDING = 10,
    SETTINGS_ICON_SIZE = 32,
    DLG_SIZE_MIN = MOUSE_GRAB_PADDING * 2 + SETTINGS_ICON_SIZE + 8,
};

SDL_Texture *loadTexture(SDL_Renderer *renderer, const unsigned char *buffer, int len) {
    int width, height, bytesPerPixel;
    void *data = stbi_load_from_memory(buffer, len, &width, &height, &bytesPerPixel, 4);
    int pitch;
    pitch = width * bytesPerPixel;
    pitch = (pitch + 3) & ~3;
    SDL_Surface *surface = SDL_CreateSurfaceFrom(data, width, height, pitch, SDL_PIXELFORMAT_ABGR8888);
    if (surface == nullptr) {
        stbi_image_free(data);
        return nullptr;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    stbi_image_free(data);
    return texture;
}

struct Panel {
    void saveToConfig() {
        toml::value data = {
            {
                "window",
                {
                    {"x", x},
                    {"y", y},
                    {"w", w},
                    {"h", h},
                    {"backgroundColor",
                     toml::array{backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a}},
                    {"autoSize", autoSize},
                }
            },
            {
                "font",
                {
                    {"fontFile", fontFile},
                    {"fontSize", fontSize},
                    {"textColor", toml::array{textColor.r, textColor.g, textColor.b, textColor.a}},
                }
            },
        };
        std::ofstream file("config/" + name + ".toml");
        file << toml::format(data);
    }
    void loadFromConfig() {
        toml::value data;
        try {
            data = toml::parse("config/" + name + ".toml");
        } catch (...) {
            return;
        }
        const auto windowData = toml::find_or(data, "window", toml::value());
        if (windowData.is_table()) {
            x = toml::find_or(windowData, "x", x);
            y = toml::find_or(windowData, "y", y);
            w = toml::find_or(windowData, "w", w);
            h = toml::find_or(windowData, "h", h);
            const auto backgroundColorData = toml::find_or(windowData, "backgroundColor", toml::value());
            if (backgroundColorData.is_array() && backgroundColorData.size() >= 4) {
                backgroundColor.r = toml::get_or(backgroundColorData.at(0), backgroundColor.r);
                backgroundColor.g = toml::get_or(backgroundColorData.at(1), backgroundColor.g);
                backgroundColor.b = toml::get_or(backgroundColorData.at(2), backgroundColor.b);
                backgroundColor.a = toml::get_or(backgroundColorData.at(3), backgroundColor.a);
            }
            autoSize = toml::find_or<bool>(windowData, "autoSize", autoSize);
        }
        const auto fontData = toml::find_or(data, "font", toml::value());
        if (fontData.is_table()) {
            fontFile = toml::find_or(fontData, "fontFile", fontFile);
            fontSize = toml::find_or(fontData, "fontSize", fontSize);
            const auto textColorData = toml::find_or(fontData, "textColor", toml::value());
            if (textColorData.is_array() && textColorData.size() >= 4) {
                textColor.r = toml::get_or(textColorData.at(0), textColor.r);
                textColor.g = toml::get_or(textColorData.at(1), textColor.g);
                textColor.b = toml::get_or(textColorData.at(2), textColor.b);
                textColor.a = toml::get_or(textColorData.at(3), textColor.a);
            }
        }
    }
    void close() {
        saveToConfig();
        if (textColorBrush) {
            DeleteObject(textColorBrush);
            textColorBrush = nullptr;
        }
        if (backgroundColorBrush) {
            DeleteObject(backgroundColorBrush);
            backgroundColorBrush = nullptr;
        }
        if (settingsTexture) {
            SDL_DestroyTexture(settingsTexture);
            settingsTexture = nullptr;
        }
        if (texture) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
        if (font) {
            TTF_CloseFont(font);
            font = nullptr;
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

    void updateTextTexture() {
        if (texture != nullptr) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
        if (!text.empty()) {
            auto *surface = TTF_RenderUTF8_Blended_Wrapped(font, text.c_str(), textColor, 0);
            if (surface != nullptr) {
                texture = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_DestroySurface(surface);
                if (texture != nullptr) {
                    updateTextRenderRect();
                }
            }
        }
    }

    void updateTextRenderRect() {
        int ww, hh;
        SDL_QueryTexture(texture, nullptr, nullptr, &ww, &hh);
        dstRect = {(float)border, (float)border, (float)ww, (float)hh};
        srcRect = {0, 0, (float)ww, (float)hh};
        auto fw = ww + border * 2;
        auto fh = hh + border * 2;
        if (autoSize) {
            if (fw < DLG_SIZE_MIN) fw = DLG_SIZE_MIN;
            if (fh < DLG_SIZE_MIN) fh = DLG_SIZE_MIN;
            if (w != fw || h != fh) {
                SDL_SetWindowSize(window, fw, fh);
            }
        } else if (w < fw || h < fh) {
            dstRect = {(float)border, (float)border, (float)(w - border * 2), (float)(h - border * 2)};
            srcRect = {0, 0, (float)(w - border * 2), (float)(h - border * 2)};
        }
    }

    bool setNewFontSize(HWND hwnd, int newSize) {
        if (newSize == fontSize) return false;
        auto ft = TTF_OpenFont(fontFile.c_str(), fontSize);
        if (ft == nullptr) {
            MessageBoxW(hwnd, L"Failed to change font size", L"Error", MB_OK);
            return false;
        }
        if (font) {
            TTF_CloseFont(font);
        }
        font = ft;
        fontSize = newSize;
        updateTextTexture();
        return true;
    }

    bool setNewTextAlpha(int newVal) {
        if (newVal == textColor.a) return false;
        textColor.a = newVal;
        updateTextTexture();
        return true;
    }

    bool setNewBackgroundAlpha(int newVal) {
        if (newVal == backgroundColor.a) return false;
        backgroundColor.a = newVal;
        return true;
    }

    bool setNewBorder(int newVal) {
        if (newVal == border) return false;
        border = newVal;
        updateTextRenderRect();
        return true;
    }

    void render() {
        auto &col = backgroundColor;
        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);
        SDL_RenderClear(renderer);
        if (texture) {
            SDL_RenderTexture(renderer, texture, &srcRect, &dstRect);
        }
        if (SDL_GetKeyboardFocus() == window) {
            float mx, my;
            SDL_GetGlobalMouseState(&mx, &my);
            int rx = (int)mx, ry = (int)my;
            int wx = x, wy = y, wx2 = wx + w, wy2 = wy + h;
            if (rx >= wx && rx < wx2 && ry >= wy && ry < wy2) {
                const SDL_FRect rect =
                    {(float)(w - MOUSE_GRAB_PADDING - SETTINGS_ICON_SIZE), MOUSE_GRAB_PADDING,
                     SETTINGS_ICON_SIZE, SETTINGS_ICON_SIZE};
                SDL_RenderTexture(renderer, settingsTexture, nullptr, &rect);
            }
        }
        SDL_RenderPresent(renderer);
    }

    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    std::string text;
    TTF_Font *font = nullptr;
    SDL_Texture *texture = nullptr;
    SDL_Texture *settingsTexture = nullptr;
    SDL_FRect srcRect = {0, 0, 0, 0};
    SDL_FRect dstRect = {0, 0, 0, 0};

    HBRUSH textColorBrush = nullptr;
    HBRUSH backgroundColorBrush = nullptr;

    std::string name;
    int x = 0;
    int y = 0;
    int w = 250;
    int h = 500;
    int border = 10;
    std::string fontFile = "data/font.ttf";
    int fontSize = 24;
    SDL_Color textColor = {0xff, 0xff, 0xff, 0xff};
    SDL_Color backgroundColor = {0, 0, 0, 0x40};
    bool autoSize = false;
};

static std::unordered_map<std::string, Panel> gPanels;
static Panel *gCurrentPanel = nullptr;
static HWND configDialog = nullptr;
static bool noEnChangeNotification = false;

static Panel *findPanelByWindow(SDL_Window *window) {
    if (!window) return nullptr;
    for (auto &p: gPanels) {
        if (p.second.window == window) {
            return &p.second;
        }
    }
    return nullptr;
}

static Panel *findPanelByWindowID(SDL_WindowID windowID) {
    return findPanelByWindow(SDL_GetWindowFromID(windowID));
}

SDL_HitTestResult HitTestCallback(SDL_Window *window, const SDL_Point *pt, void *data) {
    (void)data;
    int Width, Height;
    SDL_GetWindowSize(window, &Width, &Height);

    if (pt->y < MOUSE_GRAB_PADDING) {
        if (pt->x < MOUSE_GRAB_PADDING)
            return SDL_HITTEST_RESIZE_TOPLEFT;
        if (pt->x > Width - MOUSE_GRAB_PADDING)
            return SDL_HITTEST_RESIZE_TOPRIGHT;
        return SDL_HITTEST_RESIZE_TOP;
    }
    if (pt->y > Height - MOUSE_GRAB_PADDING) {
        if (pt->x < MOUSE_GRAB_PADDING)
            return SDL_HITTEST_RESIZE_BOTTOMLEFT;
        if (pt->x > Width - MOUSE_GRAB_PADDING)
            return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
        return SDL_HITTEST_RESIZE_BOTTOM;
    }
    if (pt->x < MOUSE_GRAB_PADDING)
        return SDL_HITTEST_RESIZE_LEFT;
    if (pt->x > Width - MOUSE_GRAB_PADDING)
        return SDL_HITTEST_RESIZE_RIGHT;

    if (pt->x > Width - 42 && pt->y < 42)
        return SDL_HITTEST_NORMAL;
    return SDL_HITTEST_DRAGGABLE;
}

static int luaLog(lua_State *L) {
    fprintf(stdout, "%s\n", luaL_checkstring(L, 1));
    fflush(stdout);
    return 0;
}

static int luaPanelCreate(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    auto *panel = &gPanels[name];
    if (panel->window != nullptr) {
        return 0;
    }
    panel->name = name;
    panel->loadFromConfig();
    panel->window = SDL_CreateWindow(name, panel->w, panel->h,
                                     (panel->autoSize ? 0 : SDL_WINDOW_RESIZABLE)
                                                 | SDL_WINDOW_BORDERLESS | SDL_WINDOW_TRANSPARENT);
    SDL_SetWindowPosition(panel->window, panel->x, panel->y);
    if (panel->renderer != nullptr) SDL_DestroyRenderer(panel->renderer);
    panel->renderer =
        SDL_CreateRenderer(panel->window, "opengl", SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (panel->font != nullptr) TTF_CloseFont(panel->font);
    panel->font = TTF_OpenFont(panel->fontFile.c_str(), panel->fontSize);
    panel->textColorBrush =
        CreateSolidBrush(RGB(panel->textColor.r, panel->textColor.g, panel->textColor.b));
    panel->backgroundColorBrush = CreateSolidBrush(RGB(panel->backgroundColor.r,
                                                       panel->backgroundColor.g,
                                                       panel->backgroundColor.b));
    SDL_SetWindowHitTest(panel->window, HitTestCallback, nullptr);
    panel->text = name;
    panel->updateTextTexture();
    if (panel->settingsTexture == nullptr) {
        panel->settingsTexture = loadTexture(panel->renderer, settings_png, sizeof(settings_png));
    }
    return 0;
}

static int luaPanelBegin(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    auto ite = gPanels.find(name);
    if (ite == gPanels.end()) {
        gCurrentPanel = nullptr;
        return 0;
    }
    gCurrentPanel = &ite->second;
    gCurrentPanel->text.clear();
    return 0;
}

static int luaPanelEnd(lua_State *L) {
    (void)L;
    if (gCurrentPanel != nullptr) {
        gCurrentPanel->updateTextTexture();
        gCurrentPanel = nullptr;
    }
    return 0;
}

static int luaPanelOutput(lua_State *L) {
    if (gCurrentPanel) {
        const auto *newText = luaL_checkstring(L, 1);
        gCurrentPanel->text.push_back('\n');
        gCurrentPanel->text.append(newText);
    }
    return 0;
}

static void initConfigDialog(HWND hwnd, Panel *panel) {
    noEnChangeNotification = true;
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LPARAM)panel);
    SetWindowPos(hwnd, nullptr, panel->x, panel->y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    HWND edc = GetDlgItem(hwnd, 1001);
    wchar_t fontFile[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, panel->fontFile.c_str(), -1, fontFile, MAX_PATH);
    SetWindowTextW(edc, fontFile);
    edc = GetDlgItem(hwnd, 1003);
    SetWindowTextW(edc, std::to_wstring(panel->fontSize).c_str());
    HWND udc = GetDlgItem(hwnd, 1004);
    SendMessageW(udc, UDM_SETRANGE, 0, MAKELPARAM(256, 6));

    edc = GetDlgItem(hwnd, 1007);
    SetWindowTextW(edc, std::to_wstring(panel->textColor.a).c_str());
    udc = GetDlgItem(hwnd, 1008);
    SendMessageW(udc, UDM_SETRANGE, 0, MAKELPARAM(255, 0));

    edc = GetDlgItem(hwnd, 1009);
    SetWindowTextW(edc, std::to_wstring(panel->backgroundColor.a).c_str());
    udc = GetDlgItem(hwnd, 1010);
    SendMessageW(udc, UDM_SETRANGE, 0, MAKELPARAM(255, 0));

    edc = GetDlgItem(hwnd, 1011);
    SetWindowTextW(edc, std::to_wstring(panel->border).c_str());
    udc = GetDlgItem(hwnd, 1012);
    SendMessageW(udc, UDM_SETRANGE, 0, MAKELPARAM(100, 0));

    auto chc = GetDlgItem(hwnd, 1013);
    SendMessageW(chc, BM_SETCHECK, panel->autoSize ? BST_CHECKED : BST_UNCHECKED, 0);
    noEnChangeNotification = false;
}

INT_PTR WINAPI dlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static COLORREF custColors[16] = {0};
    switch (uMsg) {
        case WM_INITDIALOG: {
            initConfigDialog(hwnd, (Panel *)lParam);
            return TRUE;
        }
        case WM_COMMAND:
            switch (wParam) {
                case MAKEWPARAM(IDOK, BN_CLICKED):
                case MAKEWPARAM(IDCANCEL, BN_CLICKED): {
                    auto *panel = (Panel *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
                    panel->saveToConfig();
                    ShowWindow(hwnd, SW_HIDE);
                    return 0;
                }
                case MAKEWPARAM(1002, BN_CLICKED): {
                    auto *panel = (Panel *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
                    IFileOpenDialog *pFileOpen;
                    auto hr = CoCreateInstance(CLSID_FileOpenDialog,
                                               nullptr,
                                               CLSCTX_ALL,
                                               IID_IFileOpenDialog,
                                               reinterpret_cast<void **>(&pFileOpen));
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
                                        hr = pItem
                                            ->BindToHandler(nullptr,
                                                            BHID_DataObject,
                                                            IID_IDataObject,
                                                            (void **)&pDataObject);
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
                    if (!szFile[0]) return 0;
                    wchar_t szFileRel[MAX_PATH] = {0};
                    wchar_t szCurrentPath[MAX_PATH];
                    GetModuleFileNameW(nullptr, szCurrentPath, MAX_PATH);
                    PathRemoveFileSpecW(szCurrentPath);
                    PathRelativePathToW(szFileRel,
                                        szCurrentPath,
                                        FILE_ATTRIBUTE_NORMAL,
                                        szFile,
                                        FILE_ATTRIBUTE_NORMAL);
                    if (szFileRel[0] == 0 || szFileRel[0] == '.') {
                        lstrcpyW(szFileRel, szFile);
                    }
                    for (int i = 0; szFileRel[i]; i++) {
                        if (szFileRel[i] == '\\') szFileRel[i] = '/';
                    }
                    char fontFile[MAX_PATH];
                    WideCharToMultiByte(CP_UTF8, 0, szFileRel, -1, fontFile, MAX_PATH, nullptr, nullptr);
                    if (strcasecmp(fontFile, panel->fontFile.c_str()) != 0) {
                        panel->fontFile = fontFile;
                        auto font = TTF_OpenFont(fontFile, panel->fontSize);
                        if (font == nullptr) {
                            MessageBoxW(hwnd, L"Failed to load font", L"Error", MB_OK);
                        } else {
                            if (panel->font) {
                                TTF_CloseFont(panel->font);
                            }
                            panel->font = font;
                            panel->updateTextTexture();
                            auto edc = GetDlgItem(hwnd, 1001);
                            SetWindowTextW(edc, szFileRel);
                        }
                    }
                    return 0;
                }
                case MAKEWPARAM(1005, STN_CLICKED): {
                    auto *panel = (Panel *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
                    CHOOSECOLORW cc = {sizeof(CHOOSECOLORW)};
                    cc.hInstance = (HWND)GetModuleHandleW(nullptr);
                    cc.hwndOwner = hwnd;
                    cc.lpCustColors = custColors;
                    cc.rgbResult = RGB(panel->textColor.r, panel->textColor.g, panel->textColor.b);
                    cc.Flags = CC_RGBINIT | CC_FULLOPEN | CC_ANYCOLOR;
                    if (!ChooseColorW(&cc)) break;
                    auto r = GetRValue(cc.rgbResult);
                    auto g = GetGValue(cc.rgbResult);
                    auto b = GetBValue(cc.rgbResult);
                    if (panel->textColor.r == r && panel->textColor.g == g && panel->textColor.b == b) break;
                    panel->textColor.r = r;
                    panel->textColor.g = g;
                    panel->textColor.b = b;
                    if (panel->textColorBrush) {
                        DeleteObject(panel->textColorBrush);
                    }
                    panel->textColorBrush = CreateSolidBrush(cc.rgbResult);
                    InvalidateRect(GetDlgItem(hwnd, 1005), nullptr, TRUE);
                    panel->updateTextTexture();
                    break;
                }
                case MAKEWPARAM(1006, STN_CLICKED): {
                    auto *panel = (Panel *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
                    CHOOSECOLORW cc = {sizeof(CHOOSECOLORW)};
                    cc.hInstance = (HWND)GetModuleHandleW(nullptr);
                    cc.hwndOwner = hwnd;
                    cc.lpCustColors = custColors;
                    cc.rgbResult = RGB(panel->backgroundColor.r, panel->backgroundColor.g, panel->backgroundColor.b);
                    cc.Flags = CC_RGBINIT | CC_FULLOPEN | CC_ANYCOLOR;
                    if (!ChooseColorW(&cc)) break;
                    auto r = GetRValue(cc.rgbResult);
                    auto g = GetGValue(cc.rgbResult);
                    auto b = GetBValue(cc.rgbResult);
                    if (panel->backgroundColor.r == r && panel->backgroundColor.g == g
                        && panel->backgroundColor.b == b)
                        break;
                    panel->backgroundColor.r = r;
                    panel->backgroundColor.g = g;
                    panel->backgroundColor.b = b;
                    if (panel->backgroundColorBrush) {
                        DeleteObject(panel->backgroundColorBrush);
                    }
                    panel->backgroundColorBrush = CreateSolidBrush(cc.rgbResult);
                    InvalidateRect(GetDlgItem(hwnd, 1006), nullptr, TRUE);
                    panel->updateTextTexture();
                    break;
                }
                case MAKEWPARAM(1004, EN_CHANGE): {
                    if (noEnChangeNotification) break;
                    auto *panel = (Panel *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
                    if (panel == nullptr) break;
                    wchar_t text[16];
                    GetWindowTextW((HWND)lParam, text, 16);
                    auto newVal = (int)std::wcstol(text, nullptr, 10);
                    auto changed = false;
                    if (newVal < 6) {
                        newVal = 6;
                        changed = true;
                    } else if (newVal > 256) {
                        newVal = 256;
                        changed = true;
                    }
                    if (!panel->setNewFontSize(hwnd, newVal)) {
                        noEnChangeNotification = true;
                        SetWindowTextW((HWND)lParam, std::to_wstring(panel->fontSize).c_str());
                        noEnChangeNotification = false;
                        break;
                    }
                    if (changed) {
                        noEnChangeNotification = true;
                        SetWindowTextW((HWND)lParam, std::to_wstring(newVal).c_str());
                        noEnChangeNotification = false;
                    }
                    break;
                }
                case MAKEWPARAM(1007, EN_CHANGE): {
                    if (noEnChangeNotification) break;
                    auto *panel = (Panel *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
                    if (panel == nullptr) break;
                    wchar_t text[16];
                    GetWindowTextW((HWND)lParam, text, 16);
                    if (text[0] == 0) break;
                    auto newVal = (int)std::wcstol(text, nullptr, 10);
                    auto changed = false;
                    if (newVal < 0) {
                        newVal = 0;
                        changed = true;
                    } else if (newVal > 256) {
                        newVal = 256;
                        changed = true;
                    }
                    if (!panel->setNewTextAlpha(newVal)) {
                        noEnChangeNotification = true;
                        SetWindowTextW((HWND)lParam, std::to_wstring(panel->textColor.a).c_str());
                        noEnChangeNotification = false;
                        break;
                    }
                    if (changed) {
                        noEnChangeNotification = true;
                        SetWindowTextW((HWND)lParam, std::to_wstring(newVal).c_str());
                        noEnChangeNotification = false;
                    }
                    break;
                }
                case MAKEWPARAM(1009, EN_CHANGE): {
                    if (noEnChangeNotification) break;
                    auto *panel = (Panel *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
                    if (panel == nullptr) break;
                    wchar_t text[16];
                    GetWindowTextW((HWND)lParam, text, 16);
                    if (text[0] == 0) break;
                    auto newVal = (int)std::wcstol(text, nullptr, 10);
                    auto changed = false;
                    if (newVal < 0) {
                        newVal = 0;
                        changed = true;
                    } else if (newVal > 256) {
                        newVal = 256;
                        changed = true;
                    }
                    if (!panel->setNewBackgroundAlpha(newVal)) {
                        noEnChangeNotification = true;
                        SetWindowTextW((HWND)lParam, std::to_wstring(panel->backgroundColor.a).c_str());
                        noEnChangeNotification = false;
                        break;
                    }
                    if (changed) {
                        noEnChangeNotification = true;
                        SetWindowTextW((HWND)lParam, std::to_wstring(newVal).c_str());
                        noEnChangeNotification = false;
                    }
                    break;
                }
                case MAKEWPARAM(1011, EN_CHANGE): {
                    if (noEnChangeNotification) break;
                    auto *panel = (Panel *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
                    if (panel == nullptr) break;
                    wchar_t text[16];
                    GetWindowTextW((HWND)lParam, text, 16);
                    if (text[0] == 0) break;
                    auto newVal = (int)std::wcstol(text, nullptr, 10);
                    auto changed = false;
                    if (newVal < 0) {
                        newVal = 0;
                        changed = true;
                    } else if (newVal > 100) {
                        newVal = 100;
                        changed = true;
                    }
                    if (!panel->setNewBorder(newVal)) {
                        noEnChangeNotification = true;
                        SetWindowTextW((HWND)lParam, std::to_wstring(panel->border).c_str());
                        noEnChangeNotification = false;
                        break;
                    }
                    if (changed) {
                        noEnChangeNotification = true;
                        SetWindowTextW((HWND)lParam, std::to_wstring(newVal).c_str());
                        noEnChangeNotification = false;
                    }
                    break;
                }
                case MAKEWPARAM(1013, BN_CLICKED): {
                    auto *panel = (Panel *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
                    auto newAuto = SendMessageW((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED;
                    if (newAuto == panel->autoSize) break;
                    panel->autoSize = newAuto;
                    SDL_SetWindowResizable(panel->window, newAuto ? SDL_FALSE : SDL_TRUE);
                    break;
                }
                default:
                    break;
            }
            break;
        case WM_CTLCOLORSTATIC:
            switch (GetDlgCtrlID((HWND)lParam)) {
                case 1005: {
                    auto *panel = (Panel *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
                    SetBkMode((HDC)wParam, OPAQUE);
                    SetBkColor((HDC)wParam, RGB(panel->textColor.r, panel->textColor.g, panel->textColor.b));
                    return (INT_PTR)panel->textColorBrush;
                }
                case 1006: {
                    auto *panel = (Panel *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
                    SetBkMode((HDC)wParam, OPAQUE);
                    SetBkColor((HDC)wParam,
                               RGB(panel->backgroundColor.r, panel->backgroundColor.g, panel->backgroundColor.b));
                    return (INT_PTR)panel->backgroundColorBrush;
                }
                default:
                    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
            }
        default:
            break;
    }
    return 0;
}

static void showSettingsWindow(Panel *panel) {
    auto hwnd =
        (HWND)SDL_GetProperty(SDL_GetWindowProperties(panel->window), SDL_PROPERTY_WINDOW_WIN32_HWND_POINTER, nullptr);
    if (configDialog) {
        initConfigDialog(hwnd, panel);
    } else {
        configDialog =
            CreateDialogParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(129), hwnd, dlgProc, (LPARAM)panel);
    }
    ShowWindow(configDialog, SW_SHOW);
    SetForegroundWindow(configDialog);
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
    INITCOMMONCONTROLSEX iccex = {sizeof(INITCOMMONCONTROLSEX), ICC_UPDOWN_CLASS | ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&iccex);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    TTF_Init();

    SDL_SetHint("SDL_BORDERLESS_RESIZABLE_STYLE", "1");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
    uint64_t nextTick = SDL_GetTicks() / 1000ULL * 1000ULL + 1000ULL;
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    luaL_openpkgs(L);
    lua_pushcfunction(L, luaLog);
    lua_setglobal(L, "log");
    lua_pushcfunction(L, luaPanelCreate);
    lua_setglobal(L, "panel_create");
    lua_pushcfunction(L, luaPanelBegin);
    lua_setglobal(L, "panel_begin");
    lua_pushcfunction(L, luaPanelEnd);
    lua_setglobal(L, "panel_end");
    lua_pushcfunction(L, luaPanelOutput);
    lua_setglobal(L, "panel_output");
    lua_gc(L, LUA_GCRESTART);
    lua_gc(L, LUA_GCGEN, 0, 0);
    if (luaL_loadfile(L, "scripts/_main_.lua") != 0) {
        fprintf(stderr, "Error: %s\n", lua_tostring(L, -1));
        fflush(stderr);
    } else {
        if (lua_pcall(L, 0, LUA_MULTRET, 0) != 0) {
            fprintf(stderr, "Error: %s\n", lua_tostring(L, -1));
            fflush(stderr);
        }
    }
    while (true) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_EVENT_QUIT:
                    goto QUIT;
                case SDL_EVENT_WINDOW_RESIZED: {
                    auto *panel = findPanelByWindowID(e.window.windowID);
                    if (panel) {
                        panel->w = e.window.data1;
                        panel->h = e.window.data2;
                    }
                    break;
                }
                case SDL_EVENT_WINDOW_MOVED: {
                    auto *panel = findPanelByWindowID(e.window.windowID);
                    if (panel) {
                        panel->x = e.window.data1;
                        panel->y = e.window.data2;
                    }
                    break;
                }
                case SDL_EVENT_MOUSE_BUTTON_UP: {
                    if (e.button.button != SDL_BUTTON_LEFT) break;
                    SDL_Window *win = SDL_GetKeyboardFocus();
                    auto *panel = findPanelByWindow(win);
                    if (panel) showSettingsWindow(panel);
                    break;
                }
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
                    SDL_Window *win = SDL_GetWindowFromID(e.window.windowID);
                    for (auto it = gPanels.begin(); it != gPanels.end(); it++) {
                        if (it->second.window == win) {
                            it->second.close();
                            gPanels.erase(it);
                            break;
                        }
                    }
                    break;
                }
            }
        }
        uint64_t currentTick = SDL_GetTicks();
        if (currentTick >= nextTick) {
            nextTick = currentTick / 1000ULL * 1000ULL + 1000ULL;
            lua_getglobal(L, "update");
            if (lua_pcall(L, 0, 0, 0) != 0) {
                fprintf(stderr, "Error: %s\n", lua_tostring(L, -1));
                fflush(stderr);
            }
        }
        for (auto &p: gPanels) {
            auto *panel = &p.second;
            panel->render();
        }
    }
    QUIT:
    lua_close(L);

    for (auto &p: gPanels) {
        p.second.close();
    }
    TTF_Quit();
    CoUninitialize();
    SDL_Quit();
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    int argc;
    wchar_t **argv = CommandLineToArgvW(lpCmdLine, &argc);
    return wmain(argc, argv);
}
