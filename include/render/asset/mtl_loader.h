#pragma once

#include "core/vec3.h"

#include <string>
#include <unordered_map>

struct MtlMaterialInfo {
    std::string name;
    Vec3 kd = Vec3(0.8, 0.8, 0.8);
    std::string diffuseTexturePath;
};

class MtlLoader {
public:
    bool load(const std::string& mtlPath);
    const std::unordered_map<std::string, MtlMaterialInfo>& getMaterials() const;

private:
    std::unordered_map<std::string, MtlMaterialInfo> materials;
};
