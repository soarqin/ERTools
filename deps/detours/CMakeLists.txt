project(detours CXX)

add_library(detours STATIC
    creatwth.cpp detours.cpp detours.h disasm.cpp modules.cpp
)

target_compile_definitions(detours PUBLIC _WIN32_WINNT=0x501)
target_include_directories(detours PUBLIC .)
