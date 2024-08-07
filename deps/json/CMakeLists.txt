add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE include)
add_library(nlohmann_json::headers ALIAS nlohmann_json)
