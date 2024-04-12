#pragma once

extern "C" {
typedef struct lua_State lua_State;
}

class LuaVM {
public:
    LuaVM();
    ~LuaVM();
    void registerFunctions();
    void loadFile(const char *filepath);
    void call(const char *func);

private:
    lua_State *state_;
};
