project(flagchecker CXX)

# find wxWidget
set(wxWidgets_USE_UNICODE ON)
set(wxWidgets_USE_STATIC ${wxWidgets_STATIC})
find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

add_executable(${PROJECT_NAME} WIN32
    res/manifest.rc

    main.cpp
    mainwnd.cpp mainwnd.h
    version.h

    game/eldenring.cpp game/eldenring.h
    game/flags.cpp game/flags.h
)
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME FlagChecker
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<0:>"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<0:>"
    FOLDER app)
target_include_directories(${PROJECT_NAME} PRIVATE .)
target_link_libraries(${PROJECT_NAME} ${wxWidgets_LIBRARIES} nlohmann_json::headers psapi)
