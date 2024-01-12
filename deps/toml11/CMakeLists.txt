add_library(toml11 INTERFACE)
target_include_directories(toml11 INTERFACE .)
add_library(toml11::headers ALIAS toml11)
