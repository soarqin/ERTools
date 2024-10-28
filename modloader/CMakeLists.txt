project(modloader CXX)

add_library(${PROJECT_NAME} SHARED
    modloader.cpp
    modloader/config.cpp modloader/config.h
    modloader/gamehook.cpp modloader/gamehook.h

    util/memory.h util/signature.h
)
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ERModLoader
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<0:>"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<0:>"
    FOLDER app)
target_include_directories(${PROJECT_NAME} PRIVATE .)
if(MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC /arch:AVX2)
else()
    target_compile_options(${PROJECT_NAME} PUBLIC -mavx2 -mlzcnt -mbmi -mbmi2)
endif()
target_link_libraries(${PROJECT_NAME} minhook Pattern16 fmt::fmt inih shlwapi)
