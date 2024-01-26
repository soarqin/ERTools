#include "panel.h"

#include "common.h"

#include <toml.hpp>

#include <commctrl.h>
#include <commdlg.h>
#include <shobjidl.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <fstream>

enum : int {
    MOUSE_GRAB_PADDING = 10,
    SETTINGS_ICON_SIZE = 32,
    DLG_SIZE_MIN = MOUSE_GRAB_PADDING * 2 + SETTINGS_ICON_SIZE + 8,
};

static HWND configDialog = nullptr;
static bool noEnChangeNotification = false;

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

void Panel::init(const char *n) {
#include "images.inc"
    name = n;
    loadFromConfig();
    window = SDL_CreateWindow(n, w, h, (autoSize ? 0 : SDL_WINDOW_RESIZABLE) | SDL_WINDOW_BORDERLESS | SDL_WINDOW_TRANSPARENT);
    SDL_SetWindowPosition(window, x, y);
    if (renderer != nullptr) SDL_DestroyRenderer(renderer);
    renderer = SDL_CreateRenderer(window, "opengl", SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (font != nullptr) TTF_CloseFont(font);
    font = TTF_OpenFont(fontFile.c_str(), fontSize);
    textColorBrush = CreateSolidBrush(RGB(textColor.r, textColor.g, textColor.b));
    backgroundColorBrush = CreateSolidBrush(RGB(backgroundColor.r, backgroundColor.g, backgroundColor.b));
    SDL_SetWindowHitTest(window, HitTestCallback, nullptr);
    text = n;
    updateTextTexture();
    if (settingsTexture == nullptr) {
        settingsTexture = loadTexture(renderer, settings_png, sizeof(settings_png));
    }
}

void Panel::saveToConfig() {
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
                {"border", border},
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

void Panel::loadFromConfig() {
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
        autoSize = toml::find_or(windowData, "autoSize", autoSize);
        border = toml::find_or(windowData, "border", border);
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

void Panel::close() {
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

void Panel::updateTextTexture() {
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

void Panel::updateTextRenderRect() {
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

bool Panel::setNewFontSize(HWND hwnd, int newSize) {
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

bool Panel::setNewTextAlpha(int newVal) {
    if (newVal == textColor.a) return false;
    textColor.a = newVal;
    updateTextTexture();
    return true;
}

bool Panel::setNewBackgroundAlpha(int newVal) {
    if (newVal == backgroundColor.a) return false;
    backgroundColor.a = newVal;
    return true;
}

bool Panel::setNewBorder(int newVal) {
    if (newVal == border) return false;
    border = newVal;
    updateTextRenderRect();
    return true;
}

void Panel::render() {
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

void Panel::addText(const char *str) {
    if (!text.empty()) text.push_back('\n');
    text.append(str);
}

void Panel::initConfigDialog(HWND hwnd) {
    noEnChangeNotification = true;
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LPARAM)this);
    SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    HWND edc = GetDlgItem(hwnd, 1001);
    wchar_t fontFileName[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, fontFile.c_str(), -1, fontFileName, MAX_PATH);
    SetWindowTextW(edc, fontFileName);
    edc = GetDlgItem(hwnd, 1003);
    SetWindowTextW(edc, std::to_wstring(fontSize).c_str());
    HWND udc = GetDlgItem(hwnd, 1004);
    SendMessageW(udc, UDM_SETRANGE, 0, MAKELPARAM(256, 6));

    edc = GetDlgItem(hwnd, 1007);
    SetWindowTextW(edc, std::to_wstring(textColor.a).c_str());
    udc = GetDlgItem(hwnd, 1008);
    SendMessageW(udc, UDM_SETRANGE, 0, MAKELPARAM(255, 0));

    edc = GetDlgItem(hwnd, 1009);
    SetWindowTextW(edc, std::to_wstring(backgroundColor.a).c_str());
    udc = GetDlgItem(hwnd, 1010);
    SendMessageW(udc, UDM_SETRANGE, 0, MAKELPARAM(255, 0));

    edc = GetDlgItem(hwnd, 1011);
    SetWindowTextW(edc, std::to_wstring(border).c_str());
    udc = GetDlgItem(hwnd, 1012);
    SendMessageW(udc, UDM_SETRANGE, 0, MAKELPARAM(100, 0));

    auto chc = GetDlgItem(hwnd, 1013);
    SendMessageW(chc, BM_SETCHECK, autoSize ? BST_CHECKED : BST_UNCHECKED, 0);
    noEnChangeNotification = false;
}

INT_PTR Panel::handleButtonClick(HWND hwnd, unsigned int id, LPARAM lParam) {
    static COLORREF custColors[16] = {0};
    switch (id) {
        case IDOK:
        case IDCANCEL: {
            saveToConfig();
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        case 1002: {
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
            if (!szFile[0]) return 0;
            wchar_t szFileRel[MAX_PATH] = {0};
            wchar_t szCurrentPath[MAX_PATH];
            GetModuleFileNameW(nullptr, szCurrentPath, MAX_PATH);
            PathRemoveFileSpecW(szCurrentPath);
            PathRelativePathToW(szFileRel, szCurrentPath, FILE_ATTRIBUTE_NORMAL, szFile, FILE_ATTRIBUTE_NORMAL);
            if (szFileRel[0] == 0 || szFileRel[0] == '.') {
                lstrcpyW(szFileRel, szFile);
            }
            for (int i = 0; szFileRel[i]; i++) {
                if (szFileRel[i] == '\\') szFileRel[i] = '/';
            }
            char fontFileName[MAX_PATH];
            WideCharToMultiByte(CP_UTF8, 0, szFileRel, -1, fontFileName, MAX_PATH, nullptr, nullptr);
            if (strcasecmp(fontFileName, fontFile.c_str()) != 0) {
                fontFile = fontFileName;
                auto newFont = TTF_OpenFont(fontFileName, fontSize);
                if (newFont == nullptr) {
                    MessageBoxW(hwnd, L"Failed to load font", L"Error", MB_OK);
                } else {
                    if (font) {
                        TTF_CloseFont(font);
                    }
                    font = newFont;
                    updateTextTexture();
                    auto edc = GetDlgItem(hwnd, 1001);
                    SetWindowTextW(edc, szFileRel);
                }
            }
            return 0;
        }
        case 1005: {
            CHOOSECOLORW cc = {sizeof(CHOOSECOLORW)};
            cc.hInstance = (HWND)GetModuleHandleW(nullptr);
            cc.hwndOwner = hwnd;
            cc.lpCustColors = custColors;
            cc.rgbResult = RGB(textColor.r, textColor.g, textColor.b);
            cc.Flags = CC_RGBINIT | CC_FULLOPEN | CC_ANYCOLOR;
            if (!ChooseColorW(&cc)) break;
            auto r = GetRValue(cc.rgbResult);
            auto g = GetGValue(cc.rgbResult);
            auto b = GetBValue(cc.rgbResult);
            if (textColor.r == r && textColor.g == g && textColor.b == b) break;
            textColor.r = r;
            textColor.g = g;
            textColor.b = b;
            if (textColorBrush) {
                DeleteObject(textColorBrush);
            }
            textColorBrush = CreateSolidBrush(cc.rgbResult);
            InvalidateRect(GetDlgItem(hwnd, 1005), nullptr, TRUE);
            updateTextTexture();
            break;
        }
        case 1006: {
            CHOOSECOLORW cc = {sizeof(CHOOSECOLORW)};
            cc.hInstance = (HWND)GetModuleHandleW(nullptr);
            cc.hwndOwner = hwnd;
            cc.lpCustColors = custColors;
            cc.rgbResult = RGB(backgroundColor.r, backgroundColor.g, backgroundColor.b);
            cc.Flags = CC_RGBINIT | CC_FULLOPEN | CC_ANYCOLOR;
            if (!ChooseColorW(&cc)) break;
            auto r = GetRValue(cc.rgbResult);
            auto g = GetGValue(cc.rgbResult);
            auto b = GetBValue(cc.rgbResult);
            if (backgroundColor.r == r && backgroundColor.g == g
                && backgroundColor.b == b)
                break;
            backgroundColor.r = r;
            backgroundColor.g = g;
            backgroundColor.b = b;
            if (backgroundColorBrush) {
                DeleteObject(backgroundColorBrush);
            }
            backgroundColorBrush = CreateSolidBrush(cc.rgbResult);
            InvalidateRect(GetDlgItem(hwnd, 1006), nullptr, TRUE);
            updateTextTexture();
            break;
        }
        case 1013: {
            auto newAuto = SendMessageW((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED;
            if (newAuto == autoSize) break;
            autoSize = newAuto;
            SDL_SetWindowResizable(window, newAuto ? SDL_FALSE : SDL_TRUE);
            updateTextRenderRect();
            break;
        }
        default:
            break;
    }
    return 0;
}

INT_PTR Panel::handleEditChange(HWND hwnd, unsigned int id, LPARAM lParam) {
    if (noEnChangeNotification)
        return DefWindowProcW(hwnd, WM_COMMAND, MAKEWPARAM(id, EN_CHANGE), lParam);
    switch (id) {
        case 1003: {
            wchar_t str[16];
            GetWindowTextW((HWND)lParam, str, 16);
            auto newVal = (int)std::wcstol(str, nullptr, 10);
            auto changed = false;
            if (newVal < 6) {
                newVal = 6;
                changed = true;
            } else if (newVal > 256) {
                newVal = 256;
                changed = true;
            }
            if (!setNewFontSize(hwnd, newVal)) {
                noEnChangeNotification = true;
                SetWindowTextW((HWND)lParam, std::to_wstring(fontSize).c_str());
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
        case 1007: {
            wchar_t str[16];
            GetWindowTextW((HWND)lParam, str, 16);
            if (str[0] == 0) break;
            auto newVal = (int)std::wcstol(str, nullptr, 10);
            auto changed = false;
            if (newVal < 0) {
                newVal = 0;
                changed = true;
            } else if (newVal > 256) {
                newVal = 256;
                changed = true;
            }
            if (!setNewTextAlpha(newVal)) {
                noEnChangeNotification = true;
                SetWindowTextW((HWND)lParam, std::to_wstring(textColor.a).c_str());
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
        case 1009: {
            wchar_t str[16];
            GetWindowTextW((HWND)lParam, str, 16);
            if (str[0] == 0) break;
            auto newVal = (int)std::wcstol(str, nullptr, 10);
            auto changed = false;
            if (newVal < 0) {
                newVal = 0;
                changed = true;
            } else if (newVal > 256) {
                newVal = 256;
                changed = true;
            }
            if (!setNewBackgroundAlpha(newVal)) {
                noEnChangeNotification = true;
                SetWindowTextW((HWND)lParam, std::to_wstring(backgroundColor.a).c_str());
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
        case 1011: {
            wchar_t str[16];
            GetWindowTextW((HWND)lParam, str, 16);
            if (str[0] == 0) break;
            auto newVal = (int)std::wcstol(str, nullptr, 10);
            auto changed = false;
            if (newVal < 0) {
                newVal = 0;
                changed = true;
            } else if (newVal > 100) {
                newVal = 100;
                changed = true;
            }
            if (!setNewBorder(newVal)) {
                noEnChangeNotification = true;
                SetWindowTextW((HWND)lParam, std::to_wstring(border).c_str());
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
        default:
            break;
    }
    return 0;
}

INT_PTR Panel::handleCtlColorStatic(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    switch (GetDlgCtrlID((HWND)lParam)) {
        case 1005: {
            SetBkMode((HDC)wParam, OPAQUE);
            SetBkColor((HDC)wParam, RGB(textColor.r, textColor.g, textColor.b));
            return (INT_PTR)textColorBrush;
        }
        case 1006: {
            SetBkMode((HDC)wParam, OPAQUE);
            SetBkColor((HDC)wParam,
                       RGB(backgroundColor.r, backgroundColor.g, backgroundColor.b));
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
            ((Panel *)lParam)->initConfigDialog(hwnd);
            return TRUE;
        }
        case WM_COMMAND:
            switch (HIWORD(wParam)) {
                case BN_CLICKED: {
                    auto *panel = (Panel *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
                    if (panel == nullptr) break;
                    return panel->handleButtonClick(hwnd, wParam & 0xFFFF, lParam);
                    break;
                }
                case EN_CHANGE: {
                    auto *panel = (Panel *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
                    if (panel == nullptr) break;
                    return panel->handleEditChange(hwnd, wParam & 0xFFFF, lParam);
                }
            }
            break;
        case WM_CTLCOLORSTATIC: {
            auto *panel = (Panel *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
            if (panel == nullptr) break;
            panel->handleCtlColorStatic(hwnd, wParam, lParam);
        }
        default:
            break;
    }
    return 0;
}

void Panel::showSettingsWindow() {
    if (configDialog) {
        initConfigDialog(configDialog);
    } else {
        auto hwnd = (HWND)SDL_GetProperty(SDL_GetWindowProperties(window), SDL_PROPERTY_WINDOW_WIN32_HWND_POINTER, nullptr);
        configDialog = CreateDialogParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(129), hwnd, dlgProc, (LPARAM)this);
    }
    ShowWindow(configDialog, SW_SHOW);
    SetForegroundWindow(configDialog);
}
