set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build the library as a shared library" FORCE)
if (BUILD_BINGOTOOL)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "Build the SDL3_test library" FORCE)
    set(SDL_DISABLE_UNINSTALL ON CACHE BOOL "Disable uninstallation of SDL3" FORCE)
    add_subdirectory(SDL)
    set(SDL3TTF_VENDORED ON CACHE BOOL "Use vendored third-party libraries" FORCE)
    set(SDL3TTF_HARFBUZZ OFF CACHE BOOL "Use harfbuzz to improve text shaping" FORCE)
    add_subdirectory(SDL_gfx)
    add_subdirectory(uv)
endif ()
if (BUILD_LIVESTREAMINGTOOL)
    set(LUA_BUILD_CLI OFF CACHE BOOL "Build lua cli program" FORCE)
    set(LUA_BUILD_LUAC OFF CACHE BOOL "Build luac compiler" FORCE)
    add_subdirectory(lua)
endif ()

add_subdirectory(rapidfuzz)
add_subdirectory(inih)
add_subdirectory(toml11)
add_subdirectory(json)
add_subdirectory(pugixml)

set(MESSAGE_QUIET ON)
add_subdirectory(fmt)
set(MESSAGE_QUIET OFF)

if (BUILD_FLAGCHECKER OR BUILD_LSS_HELPER)
    # wxWidget options
    option(wxWidgets_STATIC "wxWidgets link static libaries" OFF)
endif ()

if (BUILD_MOD_LOADER)
    add_subdirectory(uthash)
    add_subdirectory(detours)
    add_subdirectory(getopt)
endif ()

foreach (target liblua_static luafilesystem cjson memreader SDL3-static SDL3_gfx freetype harfbuzz rapidfuzz fmt toml11 nlohmann_json inih pugixml uthash detours wingetopt)
    if (TARGET ${target})
        set_target_properties(${target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            FOLDER "libs")
    endif ()
endforeach ()
