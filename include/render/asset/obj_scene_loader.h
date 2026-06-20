#pragma once

#include "render/scene.h"

#include <string>

class ObjSceneLoader {
public:
    bool load(const std::string& objPath, Scene& scene);
};
