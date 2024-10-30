PROJECT(wingetopt)

add_library(wingetopt getopt.c getopt.h)
target_include_directories(wingetopt PUBLIC .)
target_compile_definitions(wingetopt PRIVATE _CRT_SECURE_NO_WARNINGS)
