cmake_minimum_required(VERSION 3.21)

project(monster_hero_proto
    LANGUAGES C CXX
)

if (WIN32 AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "This project must be built as 64-bit (x64).")
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(RAYLIB_PATH "C:/devlibs/raylib-5.5")

add_library(raylib SHARED IMPORTED)

set_target_properties(raylib PROPERTIES
    IMPORTED_LOCATION
        "${RAYLIB_PATH}/lib/raylib.dll"
    IMPORTED_IMPLIB
        "${RAYLIB_PATH}/lib/raylib.lib"
    INTERFACE_INCLUDE_DIRECTORIES
        "${RAYLIB_PATH}/include"
)

# windows deps
target_link_libraries(raylib INTERFACE
    winmm
    gdi32
    user32
    shell32
)

file(GLOB_RECURSE GAME_SOURCES
    src/*.c
)

add_executable(game
    ${GAME_SOURCES}
)

target_include_directories(game PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(game PRIVATE
    raylib
)

set_target_properties(game PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY
        "${CMAKE_SOURCE_DIR}"
)

if (MSVC)
    target_compile_options(game PRIVATE
        /W4
        /permissive-
    )
else()
    target_compile_options(game PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

add_custom_command(TARGET game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${RAYLIB_PATH}/lib/raylib.dll"
        "$<TARGET_FILE_DIR:game>"
)
