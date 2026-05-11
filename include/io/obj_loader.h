#pragma once

#include "geometry/mesh.h"
#include "material/material.h"

#include <memory>
#include <string>

std::shared_ptr<Mesh> loadOBJ(
    const std::string& filename,//OBJ 文件路径
    Material* material,//整个模型统一使用的材质
    double targetSize = 1.0,//模型最长边最终变成多大
    const Point3& targetCenter = Point3(0, 0, 0)//最终模型中心放在哪里
);