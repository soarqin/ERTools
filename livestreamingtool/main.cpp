#include "panel.h"

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
#include <luapkgs.h>
}

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

static std::unordered_map<std::string, Panel> gPanels;
static Panel *gCurrentPanel = nullptr;

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

static int luaLog(lua_State *L) {
    fprintf(stdout, "%s\n", luaL_checkstring(L, 1));
    fflush(stdout);
    return 0;
}

static int luaPanelCreate(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    auto *panel = &gPanels[name];
    if (!panel->isWindow(nullptr)) {
        return 0;
    }
    panel->init(name);
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
    gCurrentPanel->clear();
    return 0;
}

static int luaPanelEnd(lua_State *L) {
    (void)L;
    if (gCurrentPanel != nullptr) {
        gCurrentPanel->updateText();
        gCurrentPanel->updateTextTexture();
        gCurrentPanel = nullptr;
    }
    return 0;
}

static int luaPanelOutput(lua_State *L) {
    if (gCurrentPanel) {
        gCurrentPanel->addText(luaL_checkstring(L, 1));
    }
    return 0;
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
