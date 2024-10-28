project(bingotool C CXX)

if(NOT MSVC)
    set(RES_FILES res/commctrl.rc)
endif()

add_executable(${PROJECT_NAME} WIN32
    main.cpp
    config.cpp config.h
    cell.cpp cell.h
    scorewindow.cpp scorewindow.h
    randomtable.cpp randomtable.h
    common.cpp common.h
    sync.cpp sync.h
    textsource.cpp textsource.h

    res/resource.h
    res/bingotool.rc ${RES_FILES}
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3_gfx SDL3::SDL3 toml11::headers fmt::fmt uv_a comctl32 shlwapi gdiplus)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<0:>"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<0:>"
    FOLDER "apps")
if(MINGW)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-ignored-attributes)
    target_link_options(${PROJECT_NAME} PRIVATE -municode)
endif()
