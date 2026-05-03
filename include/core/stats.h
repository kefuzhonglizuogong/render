#pragma once

#include <cstdint>

struct RenderStats {
    std::uint64_t sceneIntersectCalls = 0;

    std::uint64_t bvhNodeIntersectCalls = 0;
    std::uint64_t aabbHitCalls = 0;

    std::uint64_t sphereIntersectCalls = 0;
    std::uint64_t quadIntersectCalls = 0;
    std::uint64_t triangleIntersectCalls = 0;

    void reset() {
        sceneIntersectCalls = 0;

        bvhNodeIntersectCalls = 0;
        aabbHitCalls = 0;

        sphereIntersectCalls = 0;
        quadIntersectCalls = 0;
        triangleIntersectCalls = 0;
    }
};

extern RenderStats gStats;