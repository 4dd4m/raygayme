cmake_minimum_required(VERSION 3.20)

set(CLIENT_SOURCES
    main.c
    src/assetLoader.c
    src/cJSON.c
    src/client.c
    src/mycamera.c
    src/network.c
    src/player.c
    src/world.c
    ../shared/src/dummy.c
    ../shared/src/file_io.c
    ../shared/src/terrainAssetLoader.c
)

add_executable(game ${CLIENT_SOURCES})

target_include_directories(game PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/include
)

target_compile_options(game PRIVATE
    -O1
    -Wall
    -Wno-missing-braces
)

target_link_directories(game PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

target_link_libraries(game PRIVATE
    raylib
    opengl32
    gdi32
    ws2_32
    winmm
)

set_target_properties(game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
)
