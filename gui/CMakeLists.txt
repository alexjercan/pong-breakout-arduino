cmake_minimum_required(VERSION 3.0)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_COMPILER "/usr/bin/clang")

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY "https://github.com/raysan5/raylib.git"
    GIT_TAG "master"
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(raylib)

project(main C)

add_executable(main "${CMAKE_CURRENT_LIST_DIR}/main.c")
target_link_libraries(main PRIVATE raylib)
