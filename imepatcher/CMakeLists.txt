project(imepatcher C)

add_executable(${PROJECT_NAME} WIN32
    main.c
)
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME EldenRingIMEPatcher
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    FOLDER app)
