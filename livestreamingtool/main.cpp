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
        SDL_Delay(1);
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
                    if (e.button.button != SDL_BUTTON_LEFT) break;
                    SDL_Window *win = SDL_GetKeyboardFocus();
                    auto *panel = findPanelByWindow(win);
                    if (panel) panel->showSettingsWindow();
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
        nextRenderTime += 1000000ULL / 60ULL;
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
