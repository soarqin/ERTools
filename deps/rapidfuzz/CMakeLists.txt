project(rapidfuzz)

add_library(rapidfuzz INTERFACE)
target_include_directories(rapidfuzz INTERFACE .)
target_compile_features(rapidfuzz INTERFACE cxx_std_17)

add_library(rapidfuzz::rapidfuzz ALIAS rapidfuzz)
