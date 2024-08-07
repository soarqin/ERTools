project(flagchecker CXX)

# wxWidget options
set(wxWidgets_PREFIX "" CACHE STRING "wxWidgets ROOT dir")
option(wxWidgets_STATIC "wxWidgets link static libaries" ON)

# find wxWidget
if(wxWidgets_PREFIX)
    set(wxWidgets_CONFIG_OPTIONS --prefix=${wxWidgets_PREFIX})
endif()
set(wxWidgets_USE_UNICODE ON)
set(wxWidgets_USE_STATIC ${wxWidgets_STATIC})
find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

add_executable(${PROJECT_NAME} WIN32
    main.cpp
    mainwnd.cpp mainwnd.h

    game/eldenring.cpp game/eldenring.h
    game/flags.cpp game/flags.h
)
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME FlagChecker
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    FOLDER app)
target_include_directories(${PROJECT_NAME} PRIVATE .)
target_link_libraries(${PROJECT_NAME} ${wxWidgets_LIBRARIES} nlohmann_json::headers psapi)
