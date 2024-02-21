project(SDL2_gfx)

if(NOT TARGET SDL3::SDL3-static AND NOT TARGET SDL3::SDL3)
    find_package(SDL3 REQUIRED)
endif()

option(SDL3_GFX_USE_MMX "Use MMX for SDL3_gfx" ON)

add_library(SDL3_gfx STATIC
    #SDL3_framerate.c SDL3_framerate.h
    SDL3_gfxPrimitives.c SDL3_gfxPrimitives.h SDL3_gfxPrimitives_font.h
    #SDL3_imageFilter.c SDL3_imageFilter.h
    SDL3_rotozoom.c SDL3_rotozoom.h)

target_include_directories(SDL3_gfx PUBLIC ${PROJECT_SOURCE_DIR})
if(TARGET SDL3::SDL3-static)
    target_link_libraries(SDL3_gfx PUBLIC SDL3::SDL3-static)
elseif(TARGET SDL3::SDL3)
    target_link_libraries(SDL3_gfx PUBLIC SDL3::SDL3)
else()
    target_include_directories(SDL3_gfx PUBLIC ${SDL3_INCLUDE_DIRS})
    target_link_libraries(SDL3_gfx PUBLIC ${SDL3_LIBRARIES})
endif()

if(SDL3_GFX_USE_MMX)
    target_compile_definitions(SDL3_gfx PRIVATE USE_MMX)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL x86 OR CMAKE_SYSTEM_PROCESSOR STREQUAL x64
        OR CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64 OR CMAKE_SYSTEM_PROCESSOR STREQUAL amd64
        OR CMAKE_SYSTEM_PROCESSOR STREQUAL i686)
        if(NOT MSVC AND "${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
            target_link_options(SDL3_gfx PRIVATE -mmmx)
        endif()
    endif()
endif()
