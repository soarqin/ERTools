project(modloader C)

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
    OUTPUT_NAME ERModLoader
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<0:>"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<0:>"
    PREFIX ""
    FOLDER app)
target_include_directories(${PROJECT_NAME} PRIVATE .)
if(MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC /arch:AVX2)
else()
    target_compile_options(${PROJECT_NAME} PUBLIC -mavx2 -mlzcnt -mbmi -mbmi2)
endif()
target_link_libraries(${PROJECT_NAME} minhook uthash inih shlwapi)
