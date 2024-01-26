#pragma once

#define WIN32_LEAN_AND_MEAN
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#include <string>

class Panel {
public:
    void init(const char *name);
    void saveToConfig();
    void loadFromConfig();
    void close();

    void updateTextTexture();
    void updateTextRenderRect();

    void render();

    inline void clear() { text.clear(); }
    void addText(const char *str);
    inline void setWindowSize(int nw, int nh) {
        w = nw;
        h = nh;
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
    bool setNewFontSize(HWND hwnd, int newSize);
    bool setNewTextAlpha(int newVal);
    bool setNewBackgroundAlpha(int newVal);
    bool setNewBorder(int newVal);

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
