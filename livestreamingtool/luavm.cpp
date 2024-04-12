#include "luavm.h"

#include "panel.h"

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
#include <luapkgs.h>
}
#include <unordered_map>

extern std::unordered_map<std::string, Panel> gPanels;
Panel *gCurrentPanel;

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

LuaVM::LuaVM() {
    state_ = luaL_newstate();
    luaL_openlibs(state_);
    luaL_openpkgs(state_);
}

LuaVM::~LuaVM() {
    lua_close(state_);
}

void LuaVM::registerFunctions() {
    lua_pushcfunction(state_, luaLog);
    lua_setglobal(state_, "log");
    lua_pushcfunction(state_, luaPanelCreate);
    lua_setglobal(state_, "panel_create");
    lua_pushcfunction(state_, luaPanelBegin);
    lua_setglobal(state_, "panel_begin");
    lua_pushcfunction(state_, luaPanelEnd);
    lua_setglobal(state_, "panel_end");
    lua_pushcfunction(state_, luaPanelOutput);
    lua_setglobal(state_, "panel_output");
    lua_gc(state_, LUA_GCRESTART);
    lua_gc(state_, LUA_GCGEN, 0, 0);
}

void LuaVM::loadFile(const char *filepath) {
    if (luaL_loadfile(state_, filepath) != 0) {
        fprintf(stderr, "Error loading %s: %s\n", filepath, lua_tostring(state_, -1));
        fflush(stderr);
    } else {
        if (lua_pcall(state_, 0, LUA_MULTRET, 0) != 0) {
            fprintf(stderr, "Error executing %s: %s\n", filepath, lua_tostring(state_, -1));
            fflush(stderr);
        }
    }
}

void LuaVM::call(const char *func) {
    lua_getglobal(state_, func);
    if (lua_pcall(state_, 0, 0, 0) != 0) {
        fprintf(stderr, "Error calling %s: %s\n", func, lua_tostring(state_, -1));
        fflush(stderr);
    }
}
