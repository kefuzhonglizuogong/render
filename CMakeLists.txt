cmake_minimum_required(VERSION 3.16)

project(mini_path_tracer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(mini_path_tracer
    src/main.cpp
    src/camera.cpp
    src/film.cpp
    src/material.cpp
    src/plane.cpp
    src/quad.cpp
    src/triangle.cpp
    src/renderer.cpp
    src/scene.cpp
    src/sphere.cpp
    src/light.cpp
    src/geometry/bvh.cpp
    "include/frame.h"
    "include/light.h"
    "include/quad.h"
    "include/triangle.h"
    "include/core/aabb.h"
    "include/geometry/bvh.h"
    "include/core/stats.h"
    src/core/stats.cpp
)

target_compile_definitions(mini_path_tracer PRIVATE PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")
