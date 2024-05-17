project(saveimporter CXX)

option(SAVEIMPORTER_USE_CLI_ARGS "Use command line arguments" OFF)

# Check for AVX2 support
include(CheckCXXSourceRuns)
if(MSVC)
    set(CMAKE_REQUIRED_FLAGS "/arch:AVX2")
else()
    set(CMAKE_REQUIRED_FLAGS "-mavx2")
endif()
check_cxx_source_runs("#include <immintrin.h>
int main() {
    __m256i a, b, c;
    const int src[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    int dst[8];
    a =  _mm256_loadu_si256((__m256i*)src);
    b =  _mm256_loadu_si256((__m256i*)src);
    c = _mm256_add_epi32(a, b);
    _mm256_storeu_si256((__m256i*)dst, c);
    for(int i = 0; i < 8; i++) {
        if((src[i] + src[i]) != dst[i]) {
            return -1;
        }
    }
    return 0;
}" HAVE_AVX2_EXTENSIONS)

# build
if(HAVE_AVX2_EXTENSIONS)
    set(MD5_SRC md5_simd.cpp md5_simd.h)
else()
    set(MD5_SRC md5.c md5.h)
endif()
if(NOT MSVC)
    set(RES_FILES res/commctrl.rc)
endif()
add_executable(${PROJECT_NAME} WIN32
    main.cpp
    mem.cpp mem.h
    savefile.cpp savefile.h
    ${MD5_SRC}
    common.cpp common.h
    res/saveimporter.rc ${RES_FILES}
)
add_executable(${PROJECT_NAME}_maker WIN32
    maker.cpp
    mem.cpp mem.h
    savefile.cpp savefile.h
    ${MD5_SRC}
    common.cpp common.h
    res/saveimporter_maker.rc ${RES_FILES}
)
if(SAVEIMPORTER_USE_CLI_ARGS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_CLI_ARGS)
endif()
target_include_directories(${PROJECT_NAME} PRIVATE .)
target_include_directories(${PROJECT_NAME}_maker PRIVATE .)
target_link_libraries(${PROJECT_NAME} PRIVATE toml11::headers shlwapi comctl32)
target_link_libraries(${PROJECT_NAME}_maker PRIVATE toml11::headers shlwapi)
if(HAVE_AVX2_EXTENSIONS)
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PUBLIC /arch:AVX2)
        target_compile_options(${PROJECT_NAME}_maker PUBLIC /arch:AVX2)
    else()
        target_compile_options(${PROJECT_NAME} PUBLIC -mavx2)
        target_compile_options(${PROJECT_NAME}_maker PUBLIC -mavx2)
    endif()
endif()
set_target_properties(${PROJECT_NAME} ${PROJECT_NAME}_maker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    FOLDER "apps")
if(MINGW)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-ignored-attributes)
    target_link_options(${PROJECT_NAME} PRIVATE -municode)
    target_compile_options(${PROJECT_NAME}_maker PRIVATE -Wno-ignored-attributes)
    target_link_options(${PROJECT_NAME}_maker PRIVATE -municode)
endif()
