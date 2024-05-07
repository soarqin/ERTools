#include "panel.h"
#include "luavm.h"

#define WIN32_LEAN_AND_MEAN
#include <SDL3/SDL.h>
#include <windows.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <commctrl.h>
#include <shlwapi.h>
#undef WIN32_LEAN_AND_MEAN

#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>

#if defined(_MSC_VER)
#define strcasecmp _stricmp
#endif

std::unordered_map<std::string, Panel> gPanels;

static Panel *findPanelByWindow(SDL_Window *window) {
    if (!window) return nullptr;
    for (auto &p: gPanels) {
        if (p.second.isWindow(window)) {
            return &p.second;
        }
    }
    return nullptr;
}

static Panel *findPanelByWindowID(SDL_WindowID windowID) {
    return findPanelByWindow(SDL_GetWindowFromID(windowID));
}


int wmain(int argc, wchar_t *argv[]) {
    // Unused argc, argv
    (void)argc;
    (void)argv;

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) < 0) {
        printf("SDL could not be initialized!\n"
               "SDL_Error: %s\n", SDL_GetError());
        return 0;
    }
    INITCOMMONCONTROLSEX iccex = {sizeof(INITCOMMONCONTROLSEX), ICC_UPDOWN_CLASS | ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&iccex);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    SDL_SetHint("SDL_BORDERLESS_RESIZABLE_STYLE", "1");
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");

    auto *luaVM = new LuaVM();
    luaVM->registerFunctions();
    luaVM->loadFile("scripts/_main_.lua");
    auto freq = SDL_GetPerformanceFrequency() / 1000000ULL;
    auto nextRenderTime = SDL_GetPerformanceCounter() / freq;
    auto nextUpdateTime = nextRenderTime / 1000000ULL * 1000000ULL;
    while (true) {
        SDL_Delay(5);
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_EVENT_QUIT:
                    goto QUIT;
                case SDL_EVENT_WINDOW_RESIZED: {
                    auto *panel = findPanelByWindowID(e.window.windowID);
                    if (panel) {
                        panel->setWindowSize(e.window.data1, e.window.data2);
                    }
                    break;
                }
                case SDL_EVENT_WINDOW_MOVED: {
                    auto *panel = findPanelByWindowID(e.window.windowID);
                    if (panel) {
                        panel->setWindowPos(e.window.data1, e.window.data2);
                    }
                    break;
                }
                case SDL_EVENT_MOUSE_BUTTON_UP: {
                    switch (e.button.button) {
                    case SDL_BUTTON_LEFT:
                    case SDL_BUTTON_RIGHT: {
                        SDL_Window *win = SDL_GetKeyboardFocus();
                        auto *panel = findPanelByWindow(win);
                        if (panel == nullptr) break;
                        auto menu = CreatePopupMenu();
                        AppendMenuW(menu, MF_STRING | (panel->isAlwaysOnTop() ? MF_CHECKED : 0), 1, L"&Always on top");
                        AppendMenuW(menu, MF_STRING | (panel->isAutoSize() ? MF_CHECKED : 0), 2, L"Auto&size");
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING, 3, L"&Settings");
                        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(menu, MF_STRING, 4, L"&Quit");
                        auto hwnd = (HWND)SDL_GetProperty(SDL_GetWindowProperties(win), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
                        POINT pt;
                        GetCursorPos(&pt);
                        auto cmd = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, nullptr);
                        switch (cmd) {
                            case 1:
                                panel->setAlwaysOnTop(!panel->isAlwaysOnTop());
                                break;
                            case 2:
                                panel->setAutoSize(!panel->isAutoSize());
                                break;
                            case 3:
                                panel->showSettingsWindow();
                                break;
                            case 4:
                                DestroyMenu(menu);
                                goto QUIT;
                        }
                        DestroyMenu(menu);
                        break;
                    }
                    }
                    break;
                }
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
                    SDL_Window *win = SDL_GetWindowFromID(e.window.windowID);
                    for (auto it = gPanels.begin(); it != gPanels.end(); it++) {
                        if (it->second.isWindow(win)) {
                            it->second.close();
                            gPanels.erase(it);
                            break;
                        }
                    }
                    break;
                }
            }
        }
        auto curTime = SDL_GetPerformanceCounter() / freq;
        if (curTime >= nextUpdateTime) {
            nextUpdateTime = curTime / 1000000ULL * 1000000ULL + 1000000ULL;
            luaVM->call("update");
        }
        if (curTime < nextRenderTime) {
            continue;
        }
        nextRenderTime += 1000000ULL / 15ULL;
        for (auto &p: gPanels) {
            auto *panel = &p.second;
            panel->render();
        }
    }
    QUIT:
    delete luaVM;

    for (auto &p: gPanels) {
        p.second.close();
    }

    CoUninitialize();
    SDL_Quit();

    Gdiplus::GdiplusShutdown(gdiplusToken);

    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    int argc;
    wchar_t **argv = CommandLineToArgvW(lpCmdLine, &argc);
    return wmain(argc, argv);
}
