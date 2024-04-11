#pragma once

#include "textsource.h"

#define WIN32_LEAN_AND_MEAN
#include <SDL3/SDL.h>
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#include <string>

class Panel {
public:
    void init(const char *name);
    void saveToConfig();
    void loadFromConfig();
    void close();

    void updateText();
    void updateTextTexture();
    void updateTextRenderRect();

    void render();

    inline void clear() { text.clear(); }
    void addText(const char *str);
    inline void setWindowSize(int nw, int nh) {
        w = nw;
        h = nh;
        if (!autoSize) updateTextRenderRect();
    }
    inline void setWindowPos(int nx, int ny) {
        x = nx;
        y = ny;
    }
    inline bool isWindow(SDL_Window *win) {
        return win == window;
    }

    void showSettingsWindow();
    void initConfigDialog(HWND hwnd);
    INT_PTR handleButtonClick(HWND hwnd, unsigned int id, LPARAM lParam);
    INT_PTR handleEditChange(HWND hwnd, unsigned int id, LPARAM lParam);
    INT_PTR handleCtlColorStatic(HWND hwnd, WPARAM wParam, LPARAM lParam);

private:
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    std::string text;
    TextSettings *settings;
    TextSource *source;
    SDL_Texture *texture = nullptr;
    SDL_Texture *settingsTexture = nullptr;
    SDL_FRect srcRect = {0, 0, 0, 0};
    SDL_FRect dstRect = {0, 0, 0, 0};

    HBRUSH textColorBrush = nullptr;
    HBRUSH backgroundColorBrush = nullptr;
    HBRUSH shadowColorBrush = nullptr;

    std::string name;
    int x = 0;
    int y = 0;
    int w = 250;
    int h = 500;
    int border = 10;
    std::wstring fontFace = L"微软雅黑";
    int fontSize = 24;
    int fontStyle = 0;
    SDL_Color textColor = {0xff, 0xff, 0xff, 0xff};
    SDL_Color backgroundColor = {0, 0, 0, 0x40};
    Align textAlign = Align::Left;
    VAlign textVAlign = VAlign::Top;
    int textShadow = 0;
    SDL_Color textShadowColor = {0, 0, 0, 255};
    int textShadowOffset[2] = {0, 0};
    bool autoSize = false;
};
