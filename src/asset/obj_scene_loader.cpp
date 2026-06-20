#include "render/asset/obj_scene_loader.h"

#include "geometry/mesh.h"
#include "geometry/triangle.h"
#include "material/material.h"
#include "render/asset/mtl_loader.h"
#include "render/core/vec2.h"
#include "render/texture/texture.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
    struct ObjVertexIndex {
        int positionIndex = -1;
        int texcoordIndex = -1;
        int normalIndex = -1;
    };

    bool isNegativeIndexToken(const std::string& token) {
        return !token.empty() && token[0] == '-';
    }

    ObjVertexIndex parseObjVertexIndex(const std::string& token) {
        ObjVertexIndex result;

        size_t firstSlash = token.find('/');
        size_t secondSlash = token.find('/', firstSlash == std::string::npos ? firstSlash : firstSlash + 1);

        if (firstSlash == std::string::npos) {
            result.positionIndex = std::stoi(token) - 1;
            return result;
        }

        std::string vStr = token.substr(0, firstSlash);
        result.positionIndex = std::stoi(vStr) - 1;

        if (secondSlash == std::string::npos) {
            std::string vtStr = token.substr(firstSlash + 1);
            if (!vtStr.empty()) {
                result.texcoordIndex = std::stoi(vtStr) - 1;
            }
            return result;
        }

        std::string vtStr = token.substr(firstSlash + 1, secondSlash - firstSlash - 1);
        std::string vnStr = token.substr(secondSlash + 1);

        if (!vtStr.empty()) {
            result.texcoordIndex = std::stoi(vtStr) - 1;
        }

        if (!vnStr.empty()) {
            result.normalIndex = std::stoi(vnStr) - 1;
        }

        return result;
    }

    bool hasNegativeIndex(const std::string& token) {
        std::string part;

        for (char c : token) {
            if (c == '/') {
                if (isNegativeIndexToken(part)) {
                    return true;
                }
                part.clear();
            }
            else {
                part.push_back(c);
            }
        }

        return isNegativeIndexToken(part);
    }

    bool validIndex(int index, int count) {
        return index >= 0 && index < count;
    }
}

bool ObjSceneLoader::load(const std::string& objPath, Scene& scene) {
    std::ifstream file(objPath);

    if (!file) {
        std::cerr << "[OBJ] Failed to open: " << objPath << "\n";
        return false;
    }

    std::filesystem::path objFilePath(objPath);
    std::filesystem::path baseDir = objFilePath.parent_path();

    std::vector<Point3> positions;
    std::vector<Vec2> texcoords;
    std::vector<Vec3> normals;

    auto defaultMaterial = std::make_shared<Lambertian>(Color(0.8, 0.8, 0.8));
    std::unordered_map<std::string, std::shared_ptr<Material>> materialMap;
    std::unordered_map<std::string, std::shared_ptr<Texture>> textureCache;

    std::shared_ptr<Material> currentMaterial = defaultMaterial;
    bool loadedMtl = false;
    int triangleCount = 0;

    auto mesh = std::make_shared<Mesh>();

    auto loadMaterialLibrary = [&](const std::string& relativePath) {
        MtlLoader mtlLoader;
        std::filesystem::path mtlPath = baseDir / relativePath;

        if (!mtlLoader.load(mtlPath.string())) {
            return;
        }

        loadedMtl = true;

        for (const auto& entry : mtlLoader.getMaterials()) {
            const MtlMaterialInfo& info = entry.second;
            auto lambert = std::make_shared<Lambertian>(info.kd);

            if (!info.diffuseTexturePath.empty()) {
                auto textureIt = textureCache.find(info.diffuseTexturePath);
                std::shared_ptr<Texture> texture;

                if (textureIt != textureCache.end()) {
                    texture = textureIt->second;
                }
                else {
                    texture = std::make_shared<Texture>();
                    if (texture->loadFromFile(info.diffuseTexturePath)) {
                        textureCache[info.diffuseTexturePath] = texture;
                    }
                    else {
                        texture = nullptr;
                    }
                }

                if (texture) {
                    lambert->texture = texture;
                }
            }

            materialMap[info.name] = lambert;
        }
    };

    auto makeTriangle = [&](const ObjVertexIndex& a, const ObjVertexIndex& b, const ObjVertexIndex& c) {
        if (!validIndex(a.positionIndex, static_cast<int>(positions.size())) ||
            !validIndex(b.positionIndex, static_cast<int>(positions.size())) ||
            !validIndex(c.positionIndex, static_cast<int>(positions.size()))) {
            std::cerr << "[OBJ] Warning: face position index out of range, skipping face.\n";
            return;
        }

        std::shared_ptr<Triangle> triangle;

        bool hasNormals =
            validIndex(a.normalIndex, static_cast<int>(normals.size())) &&
            validIndex(b.normalIndex, static_cast<int>(normals.size())) &&
            validIndex(c.normalIndex, static_cast<int>(normals.size()));

        if (hasNormals) {
            triangle = std::make_shared<Triangle>(
                positions[a.positionIndex],
                positions[b.positionIndex],
                positions[c.positionIndex],
                normals[a.normalIndex],
                normals[b.normalIndex],
                normals[c.normalIndex],
                currentMaterial
            );
        }
        else {
            triangle = std::make_shared<Triangle>(
                positions[a.positionIndex],
                positions[b.positionIndex],
                positions[c.positionIndex],
                currentMaterial
            );
        }

        bool hasTexcoords =
            validIndex(a.texcoordIndex, static_cast<int>(texcoords.size())) &&
            validIndex(b.texcoordIndex, static_cast<int>(texcoords.size())) &&
            validIndex(c.texcoordIndex, static_cast<int>(texcoords.size()));

        if (hasTexcoords) {
            triangle->setUVs(
                texcoords[a.texcoordIndex],
                texcoords[b.texcoordIndex],
                texcoords[c.texcoordIndex]
            );
        }

        mesh->addTriangle(triangle);
        ++triangleCount;
    };

    std::string line;
    while (std::getline(file, line)) {
        size_t commentStart = line.find('#');
        if (commentStart != std::string::npos) {
            line = line.substr(0, commentStart);
        }

        std::istringstream iss(line);
        std::string tag;
        if (!(iss >> tag)) {
            continue;
        }

        if (tag == "v") {
            double x = 0.0;
            double y = 0.0;
            double z = 0.0;
            iss >> x >> y >> z;
            positions.emplace_back(x, y, z);
        }
        else if (tag == "vt") {
            float u = 0.0f;
            float v = 0.0f;
            iss >> u >> v;
            texcoords.emplace_back(u, v);
        }
        else if (tag == "vn") {
            double x = 0.0;
            double y = 0.0;
            double z = 0.0;
            iss >> x >> y >> z;
            normals.emplace_back(Vec3(x, y, z).normalized());
        }
        else if (tag == "mtllib") {
            std::string mtlName;
            iss >> mtlName;
            if (!mtlName.empty()) {
                loadMaterialLibrary(mtlName);
            }
        }
        else if (tag == "usemtl") {
            std::string materialName;
            iss >> materialName;

            auto it = materialMap.find(materialName);
            if (it != materialMap.end()) {
                currentMaterial = it->second;
            }
            else {
                std::cerr << "[OBJ] Warning: material not found: " << materialName << ", using default Lambert material.\n";
                currentMaterial = defaultMaterial;
            }
        }
        else if (tag == "f") {
            std::vector<ObjVertexIndex> face;
            std::string token;
            bool validFace = true;

            while (iss >> token) {
                if (hasNegativeIndex(token)) {
                    std::cerr << "[OBJ] Warning: negative indices are not supported, skipping face.\n";
                    validFace = false;
                    break;
                }

                try {
                    face.push_back(parseObjVertexIndex(token));
                }
                catch (const std::exception& e) {
                    std::cerr << "[OBJ] Warning: failed to parse face token '" << token << "': " << e.what() << "\n";
                    validFace = false;
                    break;
                }
            }

            if (!validFace || face.size() < 3) {
                continue;
            }

            for (size_t i = 1; i + 1 < face.size(); ++i) {
                makeTriangle(face[0], face[i], face[i + 1]);
            }
        }
    }

    if (!loadedMtl) {
        std::cout << "[OBJ] Warning: no MTL loaded, using default Lambert material.\n";
    }

    mesh->buildBVH();
    scene.add(mesh);

    std::cout << "[OBJ] Loaded: " << objPath << "\n";
    std::cout << "[OBJ] Positions: " << positions.size() << "\n";
    std::cout << "[OBJ] Texcoords: " << texcoords.size() << "\n";
    std::cout << "[OBJ] Normals: " << normals.size() << "\n";
    std::cout << "[OBJ] Triangles: " << triangleCount << "\n";

    return triangleCount > 0;
}
