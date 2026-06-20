#include "render/asset/mtl_loader.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

bool MtlLoader::load(const std::string& mtlPath) {
    materials.clear();

    std::ifstream file(mtlPath);

    if (!file) {
        std::cerr << "[MTL] Failed to open: " << mtlPath << "\n";
        return false;
    }

    std::filesystem::path baseDir = std::filesystem::path(mtlPath).parent_path();

    MtlMaterialInfo* current = nullptr;

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

        if (tag == "newmtl") {
            std::string name;
            iss >> name;

            materials[name] = MtlMaterialInfo();
            materials[name].name = name;
            current = &materials[name];
        }
        else if (tag == "Kd" && current) {
            double r = 0.8;
            double g = 0.8;
            double b = 0.8;
            iss >> r >> g >> b;
            current->kd = Vec3(r, g, b);
        }
        else if (tag == "map_Kd" && current) {
            std::string texturePath;
            iss >> texturePath;

            if (!texturePath.empty()) {
                current->diffuseTexturePath = (baseDir / texturePath).string();
            }
        }
    }

    std::cout << "[MTL] Loaded: " << mtlPath << ", materials: " << materials.size() << "\n";

    return true;
}

const std::unordered_map<std::string, MtlMaterialInfo>& MtlLoader::getMaterials() const {
    return materials;
}
