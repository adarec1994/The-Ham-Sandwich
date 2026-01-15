#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <glm/glm.hpp>

#pragma pack(push, 1)

struct M3MetaDef {
    uint64_t count;
    uint64_t offset;
};

struct M3Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct M3Submesh {
    uint32_t startIndex;
    uint32_t startVertex;
    uint32_t indexCount;
    uint32_t vertexCount;
    uint16_t materialID;
};

struct M3MaterialVariant {
    int16_t textureIndexA;
    int16_t textureIndexB;
};

struct M3MaterialData {
    std::vector<M3MaterialVariant> variants;
};

struct M3ModelData {
    std::vector<M3Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<M3Submesh> submeshes;
    std::vector<std::string> textures;
    std::vector<M3MaterialData> materials;
    bool success = false;
};

#pragma pack(pop)
