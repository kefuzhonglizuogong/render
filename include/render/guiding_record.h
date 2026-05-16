#pragma once

#include "render/path_vertex.h"

#include <vector>

//这表示一整条 camera path 的记录。
struct GuidingRecord {
    std::vector<PathVertex> vertices;

    Color finalRadiance;

    void clear() {
        vertices.clear();
        finalRadiance = Color(0.0, 0.0, 0.0);
    }

    void addVertex(const PathVertex& vertex) {
        vertices.push_back(vertex);
    }
};