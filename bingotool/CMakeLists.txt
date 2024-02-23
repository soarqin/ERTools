project(bingotool C CXX)

add_executable(${PROJECT_NAME} WIN32
    main.cpp
    randomtable.cpp randomtable.h
    common.cpp common.h
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3_gfx SDL3_ttf::SDL3_ttf SDL3::SDL3 toml11::headers fmt::fmt comctl32 shlwapi)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    FOLDER "apps")
if(MINGW)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-ignored-attributes)
    target_link_options(${PROJECT_NAME} PRIVATE -municode)
endif()
