set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build the library as a shared library" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "Build the SDL3_test library" FORCE)
set(SDL_DISABLE_UNINSTALL ON CACHE BOOL "Disable uninstallation of SDL3" FORCE)
add_subdirectory(SDL)
set(SDL3TTF_VENDORED ON CACHE BOOL "Use vendored third-party libraries" FORCE)
set(SDL3TTF_HARFBUZZ OFF CACHE BOOL "Use harfbuzz to improve text shaping" FORCE)
add_subdirectory(SDL_ttf)
add_subdirectory(SDL_gfx)
set(LUA_BUILD_CLI OFF CACHE BOOL "Build lua cli program" FORCE)
set(LUA_BUILD_LUAC OFF CACHE BOOL "Build luac compiler" FORCE)
add_subdirectory(lua)
add_subdirectory(toml11)

set(MESSAGE_QUIET ON)
add_subdirectory(fmt)
set(MESSAGE_QUIET OFF)

foreach(target liblua_static luafilesystem luv uv_a cjson memreader SDL3-static SDL3_ttf-static freetype harfbuzz toml11 SDL3_gfx fmt)
    if(TARGET ${target})
        set_target_properties(${target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            FOLDER "libs")
    endif()
endforeach()