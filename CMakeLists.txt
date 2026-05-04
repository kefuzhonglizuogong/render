cmake_minimum_required(VERSION 3.16)

project(mini_path_tracer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(mini_path_tracer
    src/main.cpp
    src/render/camera.cpp
    src/render/film.cpp
    src/material/material.cpp
    src/geometry/plane.cpp
    src/geometry/quad.cpp
    src/geometry/triangle.cpp
    src/geometry/mesh.cpp
    src/render/renderer.cpp
    src/render/scene.cpp
    src/geometry/sphere.cpp
    src/light/light.cpp
    src/geometry/bvh.cpp
    src/core/stats.cpp
    include/core/aabb.h
    include/core/frame.h
    include/core/random.h
    include/core/ray.h
    include/core/stats.h
    include/core/vec3.h
    include/geometry/bvh.h
    include/geometry/hit.h
    include/geometry/hittable.h
    include/geometry/mesh.h
    include/geometry/plane.h
    include/geometry/quad.h
    include/geometry/sphere.h
    include/geometry/triangle.h
    include/image/image.h
    include/light/light.h
    include/material/material.h
    include/render/camera.h
    include/render/film.h
    include/render/renderer.h
    include/render/scene.h
 "include/geometry/mesh.h" "src/geometry/mesh.cpp")

target_compile_definitions(mini_path_tracer PRIVATE PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")