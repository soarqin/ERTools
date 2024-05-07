#include "panel.h"

#include "common.h"

#include <toml.hpp>

#include <commctrl.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <fstream>
#include <filesystem>

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
    window = SDL_CreateWindow(n, w, h, (autoSize ? 0 : SDL_WINDOW_RESIZABLE) | SDL_WINDOW_BORDERLESS | SDL_WINDOW_TRANSPARENT | (alwaysOnTop ? SDL_WINDOW_ALWAYS_ON_TOP : 0));
    SDL_SetWindowPosition(window, x, y);
    if (renderer != nullptr) SDL_DestroyRenderer(renderer);
    renderer = SDL_CreateRenderer(window, "direct3d11", 0);
    if (renderer == nullptr) {
        renderer = SDL_CreateRenderer(window, "opengl", 0);
    }

    settings = new TextSettings();
    settings->face = fontFace;
    settings->face_size = fontSize;
    settings->color = textColor.b | (textColor.g << 8) | (textColor.r << 16) | (textColor.a << 24);
    settings->align = textAlign;
    settings->valign = textVAlign;
    settings->shadow_mode =
        textShadow == 0 ? ShadowMode::None : textShadowOffset[0] == 0 && textShadowOffset[1] == 0 ? ShadowMode::Outline : ShadowMode::Shadow;
    settings->shadow_size = float(textShadow);
    settings->shadow_offset[0] = float(textShadowOffset[0]);
    settings->shadow_offset[1] = float(textShadowOffset[1]);
    settings->shadow_color = textShadowColor.b | (textShadowColor.g << 8) | (textShadowColor.r << 16) | (textShadowColor.a << 24);
    source = new TextSource(settings);
    textColorBrush = CreateSolidBrush(RGB(textColor.r, textColor.g, textColor.b));
    backgroundColorBrush = CreateSolidBrush(RGB(backgroundColor.r, backgroundColor.g, backgroundColor.b));
    shadowColorBrush = CreateSolidBrush(RGB(textShadowColor.r, textShadowColor.g, textShadowColor.b));
    SDL_SetWindowHitTest(window, HitTestCallback, nullptr);
    text = n;
    source->text = Utf8ToUnicode(n);
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
                {"anchor", int(anchorPoint)},
                {"alwaysOnTop", int(alwaysOnTop)}
            }
        },
        {
            "font",
            {
                {"fontFace", UnicodeToUtf8(fontFace)},
                {"fontSize", fontSize},
                {"fontStyle", fontStyle},
                {"textColor", toml::array{textColor.r, textColor.g, textColor.b, textColor.a}},
                {"textAlign", int(textAlign)},
                {"textVAlign", int(textVAlign)},
                {"textShadow", textShadow},
                {"textShadowColor", toml::array{textShadowColor.r, textShadowColor.g, textShadowColor.b, textShadowColor.a}},
                {"textShadowOffset", toml::array{textShadowOffset[0], textShadowOffset[1]}},
            }
        },
    };
    CreateDirectoryW(L"config", nullptr);
    std::ofstream file(std::filesystem::path(L"config/" + Utf8ToUnicode(name) + L".toml"));
    file << toml::format(data);
}

void Panel::loadFromConfig() {
    toml::value data;
    try {
        std::ifstream ifs(std::filesystem::path(L"config/" + Utf8ToUnicode(name) + L".toml"), std::ios::binary);
        data = toml::parse(ifs);
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
        anchorPoint = Anchor(toml::find_or(windowData, "anchor", int(anchorPoint)));
        alwaysOnTop = toml::find_or(windowData, "alwaysOnTop", alwaysOnTop);
    }
    const auto fontData = toml::find_or(data, "font", toml::value());
    if (fontData.is_table()) {
        fontFace = Utf8ToUnicode(toml::find_or(fontData, "fontFace", UnicodeToUtf8(fontFace)));
        fontSize = toml::find_or(fontData, "fontSize", fontSize);
        fontStyle = toml::find_or(fontData, "fontStyle", fontStyle);
        const auto textColorData = toml::find_or(fontData, "textColor", toml::value());
        if (textColorData.is_array() && textColorData.size() >= 4) {
            textColor.r = toml::get_or(textColorData.at(0), textColor.r);
            textColor.g = toml::get_or(textColorData.at(1), textColor.g);
            textColor.b = toml::get_or(textColorData.at(2), textColor.b);
            textColor.a = toml::get_or(textColorData.at(3), textColor.a);
        }
        textAlign = static_cast<Align>(toml::find_or(fontData, "textAlign", int(textAlign)));
        textVAlign = static_cast<VAlign>(toml::find_or(fontData, "textVAlign", int(textVAlign)));
        textShadow = toml::find_or(fontData, "textShadow", textShadow);
        const auto textShadowColorData = toml::find_or(fontData, "textShadowColor", toml::value());
        if (textShadowColorData.is_array() && textShadowColorData.size() >= 4) {
            textShadowColor.r = toml::get_or(textShadowColorData.at(0), textShadowColor.r);
            textShadowColor.g = toml::get_or(textShadowColorData.at(1), textShadowColor.g);
            textShadowColor.b = toml::get_or(textShadowColorData.at(2), textShadowColor.b);
            textShadowColor.a = toml::get_or(textShadowColorData.at(3), textShadowColor.a);
        }
        const auto textShadowOffsetData = toml::find_or(fontData, "textShadowOffset", toml::value());
        if (textShadowOffsetData.is_array() && textShadowOffsetData.size() >= 2) {
            textShadowOffset[0] = toml::get_or(textShadowOffsetData.at(0), textShadowOffset[0]);
            textShadowOffset[1] = toml::get_or(textShadowOffsetData.at(1), textShadowOffset[1]);
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
    if (shadowColorBrush) {
        DeleteObject(shadowColorBrush);
        shadowColorBrush = nullptr;
    }
    if (settingsTexture) {
        SDL_DestroyTexture(settingsTexture);
        settingsTexture = nullptr;
    }
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
    if (source) {
        delete source;
        source = nullptr;
    }
    if (settings) {
        delete settings;
        settings = nullptr;
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

void Panel::updateText() {
    source->text = Utf8ToUnicode(text);
}

void Panel::updateTextTexture() {
    if (texture != nullptr) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
    if (!text.empty()) {
        source->update();
        if (source->surface != nullptr) {
            texture = SDL_CreateTextureFromSurface(renderer, source->surface);
            if (texture != nullptr) {
                updateTextRenderRect();
            }
        }
    }
}

void Panel::updateTextRenderRect() {
    int ww, hh;
    SDL_QueryTexture(texture, nullptr, nullptr, &ww, &hh);
    if (autoSize) {
        auto fw = ((ww + 1) & ~1) + border * 2;
        auto fh = ((hh + 1) & ~1) + border * 2;
        if (fw < DLG_SIZE_MIN) fw = DLG_SIZE_MIN;
        if (fh < DLG_SIZE_MIN) fh = DLG_SIZE_MIN;
        if (w != fw || h != fh) {
            switch (anchorPoint) {
                case Anchor::TopLeft:
                    break;
                case Anchor::Top:
                    x += (w - fw) >> 1;
                    break;
                case Anchor::TopRight:
                    x += w - fw;
                    break;
                case Anchor::Left:
                    y += (h - fh) >> 1;
                    break;
                case Anchor::Center:
                    x += (w - fw) >> 1;
                    y += (h - fh) >> 1;
                    break;
                case Anchor::Right:
                    x += w - fw;
                    y += (h - fh) >> 1;
                    break;
                case Anchor::BottomLeft:
                    y += h - fh;
                    break;
                case Anchor::Bottom:
                    x += (w - fw) >> 1;
                    y += h - fh;
                    break;
                case Anchor::BottomRight:
                    x += w - fw;
                    y += h - fh;
                    break;
            }
            w = fw;
            h = fh;
            SDL_SetWindowPosition(window, x, y);
            SDL_SetWindowSize(window, fw, fh);
        }
        dstRect = {(float)border, (float)border, (float)ww, (float)hh};
        srcRect = {0, 0, (float)ww, (float)hh};
    } else {
        auto nw = ((w + 1) & ~1) - border * 2;
        auto nh = ((h + 1) & ~1) - border * 2;
        dstRect = {0, 0, (float)ww, (float)hh};
        srcRect = {0, 0, (float)ww, (float)hh};
        switch (textAlign) {
            case Align::Left:
                dstRect.x = (float)border;
                if (nw < ww) {
                    srcRect.w = dstRect.w = (float)nw;
                }
                break;
            case Align::Center:
                if (nw < ww) {
                    dstRect.x = (float)border;
                    dstRect.w = (float)nw;
                    srcRect.x = (float)((ww - nw) >> 1);
                    srcRect.w = (float)nw;
                } else {
                    dstRect.x = (float)(border + ((nw - ww) >> 1));
                }
                break;
            case Align::Right:
                if (nw < ww) {
                    dstRect.x = (float)border;
                    dstRect.w = (float)nw;
                    srcRect.x = (float)(ww - nw);
                    srcRect.w = (float)nw;
                } else {
                    dstRect.x = (float)(nw + border - ww);
                }
                break;
        }
        switch (textVAlign) {
            case VAlign::Top:
                dstRect.y = (float)border;
                if (nh < hh) {
                    srcRect.h = dstRect.h = (float)nh;
                }
                break;
            case VAlign::Center:
                if (nh < hh) {
                    dstRect.y = (float)border;
                    dstRect.h = (float)nh;
                    srcRect.y = (float)((hh - nh) >> 1);
                    srcRect.h = (float)nh;
                } else {
                    dstRect.y = (float)(border + ((nh - hh) >> 1));
                }
                break;
            case VAlign::Bottom:
                if (nh < hh) {
                    dstRect.y = (float)border;
                    dstRect.h = (float)nh;
                    srcRect.y = (float)(hh - nh);
                    srcRect.h = (float)nh;
                } else {
                    dstRect.y = (float)(nh + border - hh);
                }
                break;
        }
    }
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

void Panel::setAlwaysOnTop(bool top) {
    if (top == alwaysOnTop) return;
    alwaysOnTop = top;
    SDL_SetWindowAlwaysOnTop(window, top ? SDL_TRUE : SDL_FALSE);
    saveToConfig();
}

void Panel::setAutoSize(bool as) {
    if (as == autoSize) return;
    autoSize = as;
    SDL_SetWindowResizable(window, as ? SDL_FALSE : SDL_TRUE);
    updateTextRenderRect();
    saveToConfig();
}

static INT_PTR WINAPI dlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void Panel::showSettingsWindow() {
    if (configDialog) {
        initConfigDialog(configDialog);
    } else {
        auto hwnd = (HWND)SDL_GetProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        configDialog = CreateDialogParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(129), hwnd, dlgProc, (LPARAM)this);
    }
    ShowWindow(configDialog, SW_SHOW);
    SetForegroundWindow(configDialog);
}

void Panel::initConfigDialog(HWND hwnd) {
    noEnChangeNotification = true;
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LPARAM)this);
    SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    HWND edc = GetDlgItem(hwnd, 1001);
    SetWindowTextW(edc, (fontFace + L", " + std::to_wstring(fontSize) + L"pt").c_str());

    edc = GetDlgItem(hwnd, 1007);
    SetWindowTextW(edc, std::to_wstring(textColor.a).c_str());
    HWND udc = GetDlgItem(hwnd, 1008);
    SendMessageW(udc, UDM_SETRANGE, 0, MAKELPARAM(255, 0));

    edc = GetDlgItem(hwnd, 1009);
    SetWindowTextW(edc, std::to_wstring(backgroundColor.a).c_str());
    udc = GetDlgItem(hwnd, 1010);
    SendMessageW(udc, UDM_SETRANGE, 0, MAKELPARAM(255, 0));

    edc = GetDlgItem(hwnd, 1011);
    SetWindowTextW(edc, std::to_wstring(border).c_str());
    udc = GetDlgItem(hwnd, 1012);
    SendMessageW(udc, UDM_SETRANGE, 0, MAKELPARAM(100, 0));

    edc = GetDlgItem(hwnd, 1014);
    SetWindowTextW(edc, std::to_wstring(textShadow).c_str());
    udc = GetDlgItem(hwnd, 1015);
    SendMessageW(udc, UDM_SETRANGE, 0, MAKELPARAM(100, 0));

    edc = GetDlgItem(hwnd, 1016);
    SetWindowTextW(edc, std::to_wstring(textShadowOffset[0]).c_str());
    udc = GetDlgItem(hwnd, 1017);
    SendMessageW(udc, UDM_SETRANGE, 0, MAKELPARAM(100, 0));

    edc = GetDlgItem(hwnd, 1018);
    SetWindowTextW(edc, std::to_wstring(textShadowOffset[1]).c_str());
    udc = GetDlgItem(hwnd, 1019);
    SendMessageW(udc, UDM_SETRANGE, 0, MAKELPARAM(100, 0));

    edc = GetDlgItem(hwnd, 1021);
    SetWindowTextW(edc, std::to_wstring(textShadowColor.a).c_str());
    udc = GetDlgItem(hwnd, 1022);
    SendMessageW(udc, UDM_SETRANGE, 0, MAKELPARAM(255, 0));

    auto cbc = GetDlgItem(hwnd, 1003);
    SendMessageW(cbc, CB_RESETCONTENT, 0, 0);
    SendMessageW(cbc, CB_ADDSTRING, 0, (LPARAM)L"Left");
    SendMessageW(cbc, CB_ADDSTRING, 0, (LPARAM)L"Middle");
    SendMessageW(cbc, CB_ADDSTRING, 0, (LPARAM)L"Right");
    SendMessageW(cbc, CB_SETCURSEL, int(textAlign), 0);
    cbc = GetDlgItem(hwnd, 1004);
    SendMessageW(cbc, CB_RESETCONTENT, 0, 0);
    SendMessageW(cbc, CB_ADDSTRING, 0, (LPARAM)L"Top");
    SendMessageW(cbc, CB_ADDSTRING, 0, (LPARAM)L"Center");
    SendMessageW(cbc, CB_ADDSTRING, 0, (LPARAM)L"Bottom");
    SendMessageW(cbc, CB_SETCURSEL, int(textVAlign), 0);

    cbc = GetDlgItem(hwnd, 1023);
    SendMessageW(cbc, CB_RESETCONTENT, 0, 0);
    SendMessageW(cbc, CB_ADDSTRING, 0, (LPARAM)L"Top-Left");
    SendMessageW(cbc, CB_ADDSTRING, 0, (LPARAM)L"Top-Center");
    SendMessageW(cbc, CB_ADDSTRING, 0, (LPARAM)L"Top-Right");
    SendMessageW(cbc, CB_ADDSTRING, 0, (LPARAM)L"Center-Left");
    SendMessageW(cbc, CB_ADDSTRING, 0, (LPARAM)L"Center-Center");
    SendMessageW(cbc, CB_ADDSTRING, 0, (LPARAM)L"Center-Right");
    SendMessageW(cbc, CB_ADDSTRING, 0, (LPARAM)L"Bottom-Left");
    SendMessageW(cbc, CB_ADDSTRING, 0, (LPARAM)L"Bottom-Center");
    SendMessageW(cbc, CB_ADDSTRING, 0, (LPARAM)L"Bottom-Right");
    SendMessageW(cbc, CB_SETCURSEL, int(anchorPoint), 0);

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
            LOGFONTW lf = {};
            lf.lfHeight = -MulDiv(fontSize, GetDeviceCaps(GetDC(nullptr), LOGPIXELSY), 72);
            lf.lfWeight = fontStyle & 1 ? FW_BOLD : FW_DONTCARE;
            lf.lfItalic = fontStyle & 2;
            lf.lfQuality = CLEARTYPE_NATURAL_QUALITY;
            lf.lfCharSet = DEFAULT_CHARSET;
            lstrcpyW(lf.lfFaceName, fontFace.c_str());
            CHOOSEFONTW cf = {sizeof(CHOOSEFONTW)};
            cf.hwndOwner = hwnd;
            cf.lpLogFont = &lf;
            cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_FORCEFONTEXIST | CF_NOSCRIPTSEL | CF_NOVERTFONTS | CF_LIMITSIZE;
            cf.nSizeMin = 6;
            cf.nSizeMax = 256;
            if (ChooseFontW(&cf)) {
                auto newStyle = (lf.lfWeight >= FW_BOLD ? 1 : 0) | (lf.lfItalic ? 2 : 0);
                if (fontFace != lf.lfFaceName || fontSize != cf.iPointSize / 10 || fontStyle != newStyle) {
                    fontFace = settings->face = lf.lfFaceName;
                    fontSize = settings->face_size = cf.iPointSize / 10;
                    fontStyle = newStyle;
                    settings->bold = lf.lfWeight >= FW_BOLD;
                    settings->italic = lf.lfItalic;
                    settings->updateFont();
                    updateTextTexture();
                    auto edc = GetDlgItem(hwnd, 1001);
                    SetWindowTextW(edc, (fontFace + L", " + std::to_wstring(fontSize) + L"pt").c_str());
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
            settings->color = textColor.b | (textColor.g << 8) | (textColor.r << 16) | (textColor.a << 24);
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
        case 1020: {
            CHOOSECOLORW cc = {sizeof(CHOOSECOLORW)};
            cc.hInstance = (HWND)GetModuleHandleW(nullptr);
            cc.hwndOwner = hwnd;
            cc.lpCustColors = custColors;
            cc.rgbResult = RGB(textShadowColor.r, textShadowColor.g, textShadowColor.b);
            cc.Flags = CC_RGBINIT | CC_FULLOPEN | CC_ANYCOLOR;
            if (!ChooseColorW(&cc)) break;
            auto r = GetRValue(cc.rgbResult);
            auto g = GetGValue(cc.rgbResult);
            auto b = GetBValue(cc.rgbResult);
            if (textShadowColor.r == r && textShadowColor.g == g
                && textShadowColor.b == b)
                break;
            textShadowColor.r = r;
            textShadowColor.g = g;
            textShadowColor.b = b;
            if (shadowColorBrush) {
                DeleteObject(shadowColorBrush);
            }
            shadowColorBrush = CreateSolidBrush(cc.rgbResult);
            InvalidateRect(GetDlgItem(hwnd, 1006), nullptr, TRUE);
            updateTextTexture();
            break;
        }
        case 1003: {
            auto cbc = GetDlgItem(hwnd, 1003);
            textAlign = Align(int(SendMessageW(cbc, CB_GETCURSEL, 0, 0)));
            settings->align = textAlign;
            updateTextTexture();
            break;
        }
        case 1004: {
            auto cbc = GetDlgItem(hwnd, 1004);
            textVAlign = VAlign(int(SendMessageW(cbc, CB_GETCURSEL, 0, 0)));
            settings->valign = textVAlign;
            updateTextTexture();
            break;
        }
        case 1023: {
            auto cbc = GetDlgItem(hwnd, 1023);
            anchorPoint = Anchor(int(SendMessageW(cbc, CB_GETCURSEL, 0, 0)));
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
        case 1007: {
            wchar_t str[16];
            GetWindowTextW((HWND)lParam, str, 16);
            if (str[0] == 0) break;
            auto newVal = (int)std::wcstol(str, nullptr, 10);
            if (newVal < 0) {
                newVal = 0;
            } else if (newVal > 256) {
                newVal = 256;
            }
            if (newVal != textColor.a) {
                textColor.a = newVal;
                updateTextTexture();
            }
            noEnChangeNotification = true;
            SetWindowTextW((HWND)lParam, std::to_wstring(newVal).c_str());
            noEnChangeNotification = false;
            break;
        }
        case 1009: {
            wchar_t str[16];
            GetWindowTextW((HWND)lParam, str, 16);
            if (str[0] == 0) break;
            auto newVal = (int)std::wcstol(str, nullptr, 10);
            if (newVal < 0) {
                newVal = 0;
            } else if (newVal > 256) {
                newVal = 256;
            }
            if (newVal != backgroundColor.a) {
                backgroundColor.a = newVal;
            }
            noEnChangeNotification = true;
            SetWindowTextW((HWND)lParam, std::to_wstring(newVal).c_str());
            noEnChangeNotification = false;
            break;
        }
        case 1011: {
            wchar_t str[16];
            GetWindowTextW((HWND)lParam, str, 16);
            if (str[0] == 0) break;
            auto newVal = (int)std::wcstol(str, nullptr, 10);
            if (newVal < 0) {
                newVal = 0;
            } else if (newVal > 100) {
                newVal = 100;
            }
            if (newVal != border) {
                border = newVal;
                updateTextRenderRect();
            }
            noEnChangeNotification = true;
            SetWindowTextW((HWND)lParam, std::to_wstring(newVal).c_str());
            noEnChangeNotification = false;
            break;
        }
        case 1014: {
            wchar_t str[16];
            GetWindowTextW((HWND)lParam, str, 16);
            if (str[0] == 0) break;
            auto newVal = (int)std::wcstol(str, nullptr, 10);
            if (newVal < 0) {
                newVal = 0;
            } else if (newVal > 100) {
                newVal = 100;
            }
            if (newVal != textShadow) {
                textShadow = newVal;
                settings->shadow_mode =
                    textShadow == 0 ? ShadowMode::None : textShadowOffset[0] == 0 && textShadowOffset[1] == 0 ? ShadowMode::Outline : ShadowMode::Shadow;
                settings->shadow_size = float(newVal);
                updateTextTexture();
            }
            noEnChangeNotification = true;
            SetWindowTextW((HWND)lParam, std::to_wstring(newVal).c_str());
            noEnChangeNotification = false;
            break;
        }
        case 1016: {
            wchar_t str[16];
            GetWindowTextW((HWND)lParam, str, 16);
            if (str[0] == 0) break;
            auto newVal = (int)std::wcstol(str, nullptr, 10);
            if (newVal < 0) {
                newVal = 0;
            } else if (newVal > 100) {
                newVal = 100;
            }
            if (newVal != textShadowOffset[0]) {
                textShadowOffset[0] = newVal;
                settings->shadow_mode =
                    textShadow == 0 ? ShadowMode::None : textShadowOffset[0] == 0 && textShadowOffset[1] == 0 ? ShadowMode::Outline : ShadowMode::Shadow;
                settings->shadow_offset[0] = float(newVal);
                updateTextTexture();
            }
            noEnChangeNotification = true;
            SetWindowTextW((HWND)lParam, std::to_wstring(newVal).c_str());
            noEnChangeNotification = false;
            break;
        }
        case 1018: {
            wchar_t str[16];
            GetWindowTextW((HWND)lParam, str, 16);
            if (str[0] == 0) break;
            auto newVal = (int)std::wcstol(str, nullptr, 10);
            if (newVal < 0) {
                newVal = 0;
            } else if (newVal > 100) {
                newVal = 100;
            }
            if (newVal != textShadowOffset[1]) {
                textShadowOffset[1] = newVal;
                settings->shadow_mode =
                    textShadow == 0 ? ShadowMode::None : textShadowOffset[0] == 0 && textShadowOffset[1] == 0 ? ShadowMode::Outline : ShadowMode::Shadow;
                settings->shadow_offset[1] = float(newVal);
                updateTextTexture();
            }
            noEnChangeNotification = true;
            SetWindowTextW((HWND)lParam, std::to_wstring(newVal).c_str());
            noEnChangeNotification = false;
            break;
        }
        case 1021: {
            wchar_t str[16];
            GetWindowTextW((HWND)lParam, str, 16);
            if (str[0] == 0) break;
            auto newVal = (int)std::wcstol(str, nullptr, 10);
            if (newVal < 0) {
                newVal = 0;
            } else if (newVal > 256) {
                newVal = 256;
            }
            if (newVal != textShadowColor.a) {
                textShadowColor.a = newVal;
                settings->shadow_color = textShadowColor.b | (textShadowColor.g << 8) | (textShadowColor.r << 16) | (textShadowColor.a << 24);
                updateTextTexture();
            }
            noEnChangeNotification = true;
            SetWindowTextW((HWND)lParam, std::to_wstring(newVal).c_str());
            noEnChangeNotification = false;
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
        case 1020: {
            SetBkMode((HDC)wParam, OPAQUE);
            SetBkColor((HDC)wParam,
                       RGB(textShadowColor.r, textShadowColor.g, textShadowColor.b));
            return (INT_PTR)shadowColorBrush;
        }
        default:
            break;
    }
    return DefWindowProcW(hwnd, WM_CTLCOLORSTATIC, wParam, lParam);
}

static INT_PTR WINAPI dlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG: {
            ((Panel *)lParam)->initConfigDialog(hwnd);
            return TRUE;
        }
        case WM_COMMAND:
            switch (HIWORD(wParam)) {
                case CBN_SELCHANGE:
                case BN_CLICKED: {
                    auto *panel = (Panel *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
                    if (panel == nullptr) break;
                    return panel->handleButtonClick(hwnd, wParam & 0xFFFF, lParam);
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
            return panel->handleCtlColorStatic(hwnd, wParam, lParam);
        }
        default:
            break;
    }
    return 0;
}
