project(modloader C CXX)

add_executable(modlauncher WIN32
    launcher.c
    steam/app.c steam/app.h
    steam/vdf.c steam/vdf.h
)

add_library(${PROJECT_NAME} SHARED
    modloader.c
    modloader/config.c modloader/config.h
    modloader/gamehook.c modloader/gamehook.h
    modloader/mod.c modloader/mod.h
    modloader/filecache.c modloader/filecache.h

    proxy/winhttp.c proxy/winhttp.h proxy/winhttp.def
    proxy/dinput8.c proxy/dinput8.h proxy/dinput8.def
    proxy/dxgi.c proxy/dxgi.h proxy/dxgi.def
)
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME YAERModLoader
    PREFIX "")
set_target_properties(modlauncher PROPERTIES
    OUTPUT_NAME YAERModLoader)
set_target_properties(${PROJECT_NAME} modlauncher PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<0:>"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<0:>"
    FOLDER app)
target_include_directories(${PROJECT_NAME} PRIVATE .)
target_link_libraries(${PROJECT_NAME} detours uthash inih shlwapi)
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
    target_compile_options(modlauncher PRIVATE /utf-8)
else ()
    target_link_options(${PROJECT_NAME} PRIVATE -municode)
    target_link_options(modlauncher PRIVATE -municode)
endif ()
target_link_libraries(modlauncher detours wingetopt shlwapi)
