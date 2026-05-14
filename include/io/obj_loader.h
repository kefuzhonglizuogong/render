#pragma once

#include "geometry/mesh.h"
#include "material/material.h"

#include <memory>
#include <string>

std::shared_ptr<Mesh> loadOBJ(
    const std::string& filename,//OBJ file path
    const std::shared_ptr<Material>& material,//Shared material used by the whole mesh
    double targetSize = 1.0,//Normalized longest side length
    const Point3& targetCenter = Point3(0, 0, 0)//Normalized mesh center
);
