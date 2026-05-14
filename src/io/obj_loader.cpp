#include "io/obj_loader.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
    struct FaceVertex {
        int vertexIndex = -1;
        int normalIndex = -1;
    };

    bool parseInteger(const std::string& text, int& value) {
        if (text.empty()) {
            return false;
        }

        try {
            size_t parsedLength = 0;
            value = std::stoi(text, &parsedLength);
            return parsedLength == text.size();
        }
        catch (...) {
            return false;
        }
    }

    int resolveOBJIndex(int objIndex, int count) {
        if (objIndex > 0) {
            return objIndex - 1;
        }

        if (objIndex < 0) {
            return count + objIndex;
        }

        return -1;
    }

    bool parseFaceVertex(
        const std::string& token,
        int vertexCount,
        int normalCount,
        FaceVertex& faceVertex
    ) {
        std::vector<std::string> parts;
        std::string part;

        for (char c : token) {
            if (c == '/') {
                parts.push_back(part);
                part.clear();
            }
            else {
                part.push_back(c);
            }
        }

        parts.push_back(part);

        int objVertexIndex = 0;
        if (!parseInteger(parts[0], objVertexIndex) || objVertexIndex == 0) {
            std::cerr << "Invalid OBJ vertex index token: " << token << std::endl;
            return false;
        }

        faceVertex.vertexIndex = resolveOBJIndex(objVertexIndex, vertexCount);

        if (faceVertex.vertexIndex < 0 || faceVertex.vertexIndex >= vertexCount) {
            std::cerr << "OBJ vertex index out of range: " << objVertexIndex << std::endl;
            return false;
        }

        faceVertex.normalIndex = -1;

        if (parts.size() >= 3 && !parts[2].empty()) {
            int objNormalIndex = 0;
            if (!parseInteger(parts[2], objNormalIndex) || objNormalIndex == 0) {
                std::cerr << "Invalid OBJ normal index token: " << token << std::endl;
                return false;
            }

            faceVertex.normalIndex = resolveOBJIndex(objNormalIndex, normalCount);

            if (faceVertex.normalIndex < 0 || faceVertex.normalIndex >= normalCount) {
                std::cerr << "OBJ normal index out of range: " << objNormalIndex << std::endl;
                return false;
            }
        }

        return true;
    }

    bool hasCompleteVertexNormals(
        const std::vector<FaceVertex>& face,
        size_t i0,
        size_t i1,
        size_t i2
    ) {
        return face[i0].normalIndex >= 0 &&
            face[i1].normalIndex >= 0 &&
            face[i2].normalIndex >= 0;
    }
}

std::shared_ptr<Mesh> loadOBJ(
    const std::string& filename,
    const std::shared_ptr<Material>& material,
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
    std::vector<Vec3> normals;
    std::vector<std::vector<FaceVertex>> faces;

    std::string line;

    while (std::getline(file, line)) {
        size_t commentStart = line.find('#');
        if (commentStart != std::string::npos) {
            line = line.substr(0, commentStart);
        }

        std::istringstream iss(line);

        std::string prefix;
        if (!(iss >> prefix)) {
            continue;
        }

        if (prefix == "v") {
            double x, y, z;
            if (iss >> x >> y >> z) {
                vertices.emplace_back(x, y, z);
            }
        }
        else if (prefix == "vn") {
            double x, y, z;
            if (iss >> x >> y >> z) {
                normals.emplace_back(Vec3(x, y, z).normalized());
            }
        }
        else if (prefix == "f") {
            std::vector<FaceVertex> face;
            std::string token;
            bool validFace = true;

            while (iss >> token) {
                FaceVertex faceVertex;

                if (!parseFaceVertex(
                    token,
                    static_cast<int>(vertices.size()),
                    static_cast<int>(normals.size()),
                    faceVertex
                )) {
                    validFace = false;
                    break;
                }

                face.push_back(faceVertex);
            }

            if (validFace && face.size() >= 3) {
                faces.push_back(face);
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

    Point3 originalCenter = 0.5 * (minP + maxP);
    Vec3 extent = maxP - minP;
    double maxExtent = std::max(extent.x, std::max(extent.y, extent.z));

    double scale = 1.0;

    if (maxExtent > 0.0) {
        scale = targetSize / maxExtent;
    }

    std::cout << "\n=== OBJ Normalize Info ===\n";
    std::cout << "Original bbox min: " << minP.x << ", " << minP.y << ", " << minP.z << "\n";
    std::cout << "Original bbox max: " << maxP.x << ", " << maxP.y << ", " << maxP.z << "\n";
    std::cout << "Original center: " << originalCenter.x << ", " << originalCenter.y << ", " << originalCenter.z << "\n";
    std::cout << "Scale factor: " << scale << "\n";

    std::vector<Point3> transformedVertices;
    transformedVertices.reserve(vertices.size());

    for (const auto& v : vertices) {
        Point3 local = v - originalCenter;
        Point3 scaled = local * scale;
        Point3 transformed = scaled + targetCenter;

        transformedVertices.push_back(transformed);
    }

    auto addTriangleFromFace = [&](const std::vector<FaceVertex>& face, size_t i0, size_t i1, size_t i2) {
        const FaceVertex& fv0 = face[i0];
        const FaceVertex& fv1 = face[i1];
        const FaceVertex& fv2 = face[i2];

        if (hasCompleteVertexNormals(face, i0, i1, i2)) {
            mesh->addTriangle(
                transformedVertices[fv0.vertexIndex],
                transformedVertices[fv1.vertexIndex],
                transformedVertices[fv2.vertexIndex],
                normals[fv0.normalIndex],
                normals[fv1.normalIndex],
                normals[fv2.normalIndex],
                material
            );
        }
        else {
            mesh->addTriangle(
                transformedVertices[fv0.vertexIndex],
                transformedVertices[fv1.vertexIndex],
                transformedVertices[fv2.vertexIndex],
                material
            );
        }
    };

    for (const auto& face : faces) {
        for (size_t i = 1; i + 1 < face.size(); ++i) {
            addTriangleFromFace(face, 0, i, i + 1);
        }
    }

    mesh->buildBVH();

    std::cout << "Loaded OBJ: " << filename << std::endl;
    std::cout << "Vertices: " << vertices.size() << std::endl;
    std::cout << "Vertex normals: " << normals.size() << std::endl;
    std::cout << "Triangles: " << mesh->triangles.size() << std::endl;

    return mesh;
}
