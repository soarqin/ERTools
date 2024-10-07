cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(CustomCompilerOptions)
fix_compile_flags()
fix_release_flags()
add_static_runtime_option(ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

project(ERTools)

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(deps)
add_subdirectory(bingotool)
add_subdirectory(livestreamingtool)
add_subdirectory(saveimporter)
add_subdirectory(flagchecker)
add_subdirectory(imepatcher)

