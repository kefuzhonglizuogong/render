#include "io/obj_loader.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>

namespace {
    int parseVertexIndex(const std::string& token) {
        // 支持：
        // f 1 2 3
        // f 1/1 2/2 3/3
        // f 1//1 2//2 3//3
        // f 1/1/1 2/2/2 3/3/3

        std::string indexText;

        for (char c : token) {
            if (c == '/') {
                break;
            }

            indexText.push_back(c);
        }

        if (indexText.empty()) {
            return -1;
        }

        return std::stoi(indexText);
    }
}

std::shared_ptr<Mesh> loadOBJ(
    const std::string& filename,
    Material* material,
    double targetSize,
    const Point3& targetCenter
) {
    auto mesh = std::make_shared<Mesh>();

    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Failed to open OBJ file: " << filename << std::endl;
        return mesh;
    }

    std::vector<Point3> vertices;

    // 保存所有 face 的顶点索引
    std::vector<std::vector<int>> faces;

    std::string line;

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream iss(line);

        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            double x, y, z;
            iss >> x >> y >> z;

            vertices.emplace_back(x, y, z);
        }
        else if (prefix == "f") {
            std::vector<int> indices;
            std::string token;

            while (iss >> token) {
                int objIndex = parseVertexIndex(token);

                if (objIndex == 0) {
                    std::cerr << "OBJ index 0 is invalid.\n";
                    continue;
                }

                if (objIndex < 0) {
                    // 先支持负索引：-1 表示最后一个顶点
                    objIndex = static_cast<int>(vertices.size()) + objIndex + 1;
                }

                int zeroBasedIndex = objIndex - 1;

                if (
                    zeroBasedIndex < 0 ||
                    zeroBasedIndex >= static_cast<int>(vertices.size())
                    ) {
                    std::cerr << "OBJ vertex index out of range: "
                        << objIndex << std::endl;
                    continue;
                }

                indices.push_back(zeroBasedIndex);
            }

            if (indices.size() >= 3) {
                faces.push_back(indices);
            }
        }
    }

    if (vertices.empty()) {
        std::cerr << "OBJ has no vertices.\n";
        return mesh;
    }

    Point3 minP = vertices[0];
    Point3 maxP = vertices[0];

    for (const auto& v : vertices) {

        minP.x = std::min(minP.x, v.x);
        minP.y = std::min(minP.y, v.y);
        minP.z = std::min(minP.z, v.z);

        maxP.x = std::max(maxP.x, v.x);
        maxP.y = std::max(maxP.y, v.y);
        maxP.z = std::max(maxP.z, v.z);
    }

    Point3 originalCenter =0.5 * (minP + maxP);

    Vec3 extent = maxP - minP;

    double maxExtent =std::max(extent.x,std::max(extent.y, extent.z));

    double scale = 1.0;

    if (maxExtent > 0.0) {
        scale = targetSize / maxExtent;
    }

    // =====================================================
    // 调试输出
    // =====================================================

    std::cout << "\n=== OBJ Normalize Info ===\n";
    std::cout << "Original bbox min: "<< minP.x << ", "<< minP.y << ", "<< minP.z << "\n";
    std::cout << "Original bbox max: "<< maxP.x << ", "<< maxP.y << ", "<< maxP.z << "\n";
    std::cout << "Original center: "<< originalCenter.x << ", "<< originalCenter.y << ", "<< originalCenter.z << "\n";
    std::cout << "Scale factor: "<< scale << "\n";

    // =====================================================
    // 统一变换顶点
    // =====================================================

    std::vector<Point3> transformedVertices;

    transformedVertices.reserve(vertices.size());

    for (const auto& v : vertices) {

        // 移动到局部原点
        Point3 local = v - originalCenter;

        // 统一缩放
        Point3 scaled = local * scale;

        // 放到目标位置
        Point3 transformed =
            scaled + targetCenter;

        transformedVertices.push_back(transformed);
    }

    // =====================================================
    // 真正生成 Triangle
    // =====================================================

    for (const auto& indices : faces) {

        if (indices.size() < 3) {
            continue;
        }

        // 三角形
        if (indices.size() == 3) {

            mesh->addTriangle(
                transformedVertices[indices[0]],
                transformedVertices[indices[1]],
                transformedVertices[indices[2]],
                material
            );
        }

        // 四边形 / 多边形
        // fan triangulation
        if (indices.size() > 3) {

            for (size_t i = 1;
                i + 1 < indices.size();
                ++i)
            {
                mesh->addTriangle(
                    transformedVertices[indices[0]],
                    transformedVertices[indices[i]],
                    transformedVertices[indices[i + 1]],
                    material
                );
            }
        }
    }


    mesh->buildBVH();

    std::cout << "Loaded OBJ: " << filename << std::endl;
    std::cout << "Vertices: " << vertices.size() << std::endl;
    std::cout << "Triangles: " << mesh->triangles.size() << std::endl;

    return mesh;
}
