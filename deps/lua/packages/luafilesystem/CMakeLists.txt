cmake_minimum_required (VERSION 3.5)
project(luafilesystem C)

add_library(luafilesystem STATIC src/lfs.c)
if(NOT MSVC)
    target_compile_options(luafilesystem PRIVATE -fPIC)
endif()
target_link_libraries(luafilesystem liblua_static)
set_target_properties(luafilesystem PROPERTIES PREFIX "")
