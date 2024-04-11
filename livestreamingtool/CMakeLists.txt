project(livestreamingtool C CXX)

if(NOT MSVC)
    set(COMMCTRL_SRC res/commctrl.rc)
endif()

add_executable(${PROJECT_NAME} WIN32
    # main sources
    main.cpp
    panel.cpp panel.h
    common.cpp common.h
    textsource.cpp textsource.h
    # inlined images
    images.inc
    # stb
    stb_image.h
    # resources
    res/res.rc ${COMMCTRL_SRC})
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3 lua_extra_pkgs liblua_static toml11::headers comctl32 shlwapi gdiplus)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    FOLDER "apps")
if(MINGW)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-ignored-attributes)
    target_link_options(${PROJECT_NAME} PRIVATE -municode)
endif()
