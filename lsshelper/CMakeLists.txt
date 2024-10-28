project(lss_helper CXX)

# find wxWidget
set(wxWidgets_USE_UNICODE ON)
set(wxWidgets_USE_STATIC ${wxWidgets_STATIC})
find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

add_executable(${PROJECT_NAME} WIN32
    main.cpp
    mainwnd.cpp mainwnd.h
    editsegmentdlg.cpp editsegmentdlg.h
    lss.cpp lss.h
    enums.cpp enums.h
    util.h

    res/manifest.rc
)
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME LSSHelper
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<0:>"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<0:>"
    FOLDER app)
target_include_directories(${PROJECT_NAME} PRIVATE .)
target_link_libraries(${PROJECT_NAME} ${wxWidgets_LIBRARIES} nlohmann_json::headers fmt::fmt pugixml::pugixml rapidfuzz::rapidfuzz)
if (wxWidgets_LIBRARIES MATCHES "_lib\/")
    target_link_libraries(${PROJECT_NAME} gdiplus msimg32)
endif ()
