project(pugixml CXX)

option(PUGIXML_WCHAR_MODE "Enable wchar_t mode" OFF)
option(PUGIXML_COMPACT "Enable compact mode" OFF)

# Advanced options from pugiconfig.hpp
option(PUGIXML_NO_XPATH "Disable XPath" OFF)
option(PUGIXML_NO_STL "Disable STL" OFF)
option(PUGIXML_NO_EXCEPTIONS "Disable Exceptions" OFF)
mark_as_advanced(PUGIXML_NO_XPATH PUGIXML_NO_STL PUGIXML_NO_EXCEPTIONS)

set(PUGIXML_PUBLIC_DEFINITIONS
    $<$<BOOL:${PUGIXML_WCHAR_MODE}>:PUGIXML_WCHAR_MODE>
    $<$<BOOL:${PUGIXML_COMPACT}>:PUGIXML_COMPACT>
    $<$<BOOL:${PUGIXML_NO_XPATH}>:PUGIXML_NO_XPATH>
    $<$<BOOL:${PUGIXML_NO_STL}>:PUGIXML_NO_STL>
    $<$<BOOL:${PUGIXML_NO_EXCEPTIONS}>:PUGIXML_NO_EXCEPTIONS>)

add_library(pugixml STATIC
    src/pugixml.cpp
    src/pugixml.hpp
    src/pugiconfig.hpp
)
add_library(pugixml::pugixml ALIAS pugixml)
target_include_directories(pugixml PUBLIC src)
target_compile_definitions(pugixml PUBLIC ${PUGIXML_PUBLIC_DEFINITIONS})
set_target_properties(pugixml PROPERTIES
    OUTPUT_NAME pugixml
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    FOLDER deps)
