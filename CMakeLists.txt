cmake_minimum_required(VERSION 3.16)

project(mini_path_tracer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(mini_path_tracer
    src/main.cpp
    src/sphere.cpp
    src/plane.cpp
    src/scene.cpp
    src/material.cpp
    src/camera.cpp
    src/film.cpp
    src/renderer.cpp
    src/light.cpp
 "include/frame.h" "include/light.h" "src/light.cpp")

target_compile_definitions(mini_path_tracer PRIVATE PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")
