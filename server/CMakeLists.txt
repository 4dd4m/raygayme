cmake_minimum_required(VERSION 3.20)

set(SERVER_SOURCES
    main.c
    src/server.c
    src/test.c
    ../shared/src/dummy.c
    ../shared/src/file_io.c
    ../shared/src/terrainAssetLoader.c
    ../client/src/cJSON.c
)

add_executable(server ${SERVER_SOURCES})

target_include_directories(server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../client/include
)

target_compile_options(server PRIVATE
    -O1
    -Wall
    -Wno-missing-braces
)

target_link_directories(server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

target_link_libraries(server PRIVATE
    raylib
    opengl32
    gdi32
    ws2_32
    winmm
)

set_target_properties(server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
)
