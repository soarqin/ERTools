# If Lua is installed in a non-standard location, please set the LUA_DIR
# environment variable to point to prefix for the install. Eg:
#       Unix: export LUA_DIR=/home/user/pkg
#       Windows: set LUA_DIR=c:\lua51

project(lua-cjson C)
cmake_minimum_required(VERSION 3.5)

# Handle platforms missing isinf() macro (Eg, some Solaris systems).
include(CheckSymbolExists)
CHECK_SYMBOL_EXISTS(isinf math.h HAVE_ISINF)
if(NOT HAVE_ISINF)
    add_definitions(-DUSE_INTERNAL_ISINF)
endif()

if(APPLE)
    set(CMAKE_SHARED_MODULE_CREATE_C_FLAGS "${CMAKE_SHARED_MODULE_CREATE_C_FLAGS} -undefined dynamic_lookup")
endif()

if(WIN32)
    # Windows sprintf()/strtod() handle NaN/inf differently. Not supported.
    add_definitions(-DDISABLE_INVALID_NUMBERS)
endif()

add_library(cjson STATIC src/lua_cjson.c src/strbuf.c src/fpconv.c)
if(NOT MSVC)
    target_compile_options(cjson PRIVATE -fPIC)
endif()
set_target_properties(cjson PROPERTIES PREFIX "")
target_link_libraries(cjson liblua_static)

# vi:ai et sw=4 ts=4:
