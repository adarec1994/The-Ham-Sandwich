#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#pragma pack(push, 1)

struct M3MetaDef {
    int64_t count;
    int64_t offset;
};

struct M3TrackDef {
    int64_t count;
    int64_t offsetA;
    int64_t offsetB;
};

struct M3Bounds {
    glm::vec3 bbMin;
    float pad0;
    glm::vec3 bbMax;
    float pad1;
    glm::vec3 sphereCenter;
    float pad2;
    float sphereRadius;
    float pad3;

    bool isSet() const {
        float total = bbMin.x + bbMin.y + bbMin.z + bbMax.x + bbMax.y + bbMax.z +
                      sphereCenter.x + sphereCenter.y + sphereCenter.z + sphereRadius;
        return total != 0.0f;
    }
};

struct M3KeyFrame {
    uint32_t timestamp = 0;
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 translation{0.0f, 0.0f, 0.0f};
    glm::vec4 unknown{0.0f, 0.0f, 0.0f, 0.0f};

    float getTimeSeconds() const { return timestamp / 1000.0f; }
};

struct M3AnimationTrack {
    int64_t duration = 0;
    int64_t timeOffset = 0;
    int64_t valueOffset = 0;
    int trackType = 0;
    std::vector<M3KeyFrame> keyframes;
};

struct M3Bone {
    std::string name;
    int32_t id = 0;
    int16_t globalId = -1;
    uint16_t flags = 0;
    int16_t parentId = -1;
    int16_t unk01 = 0;
    uint8_t unk02 = 0;
    uint8_t unk03 = 0;
    uint8_t unk04 = 0;
    uint8_t unk05 = 0;
    uint32_t unk06 = 0;

    M3AnimationTrack tracks[8];

    glm::mat4 globalMatrix{1.0f};
    glm::mat4 inverseGlobalMatrix{1.0f};
    glm::vec3 position{0.0f};
    std::string parentPath;
};

struct M3ModelAnimation {
    uint16_t sequenceId = 0;
    uint16_t unk1 = 0;
    uint16_t unk2 = 0;
    uint16_t unk3 = 0;
    uint16_t unk4 = 0;
    uint16_t fallbackSequence = 0;
    uint32_t timestampStart = 0;
    uint32_t timestampEnd = 0;
    uint16_t unk10 = 0;
    uint16_t unk11 = 0;
    uint16_t unk12 = 0;
    uint16_t unk13 = 0;
    uint16_t unk14 = 0;
    uint16_t unk15 = 0;
    glm::vec3 bound1{0.0f};
    glm::vec3 bound2{0.0f};
    glm::vec3 bound3{0.0f};
    glm::vec3 bound4{0.0f};
    uint32_t unk19 = 0;
    uint32_t unk23 = 0;
    uint32_t unk25 = 0;
    uint32_t unk27 = 0;
    uint64_t unk28 = 0;
    uint64_t unk29 = 0;
};

struct M3Texture {
    int16_t unk0 = 0;
    uint8_t type = 0;
    uint8_t unk1 = 0;
    int32_t flags = 0;
    float intensity = 0.0f;
    uint8_t unk4 = 0;
    uint8_t unk5 = 0;
    uint8_t unk6 = 0;
    uint8_t unk7 = 0;
    uint64_t nrLetters = 0;
    uint64_t offset = 0;
    std::string path;
    std::string textureType;
};

struct M3MaterialVariant {
    int16_t textureIndexA = -1;
    int16_t textureIndexB = -1;
    std::array<uint16_t, 146> unkValues{};
    std::string textureColorPath;
    std::string textureNormalPath;
};

struct M3Material {
    uint8_t unk0 = 0;
    uint8_t unk1 = 0;
    uint8_t unk2 = 0;
    uint8_t unk3 = 0;
    uint8_t unk4 = 0;
    uint8_t unk5 = 0;
    uint8_t unk6 = 0;
    uint8_t unk7 = 0;
    uint8_t unk8 = 0;
    uint8_t unk9 = 0;
    uint8_t unk10 = 0;
    uint8_t unk11 = 0;
    uint16_t unk12 = 0;
    uint16_t unk14 = 0;
    uint32_t unk16 = 0;
    uint32_t unk20 = 0;
    int32_t specularX = 0;
    int32_t specularY = 0;
    uint64_t nrDescriptions = 0;
    uint64_t ofsDescriptions = 0;
    std::vector<M3MaterialVariant> variants;
};

struct M3Submesh {
    uint32_t startIndex = 0;
    uint32_t startVertex = 0;
    uint32_t indexCount = 0;
    uint32_t vertexCount = 0;
    uint16_t startBoneMapping = 0;
    uint16_t nrBoneMapping = 0;
    uint16_t unk1 = 0;
    uint16_t materialID = 0;
    uint16_t unk2 = 0;
    uint16_t unk3 = 0;
    uint16_t unk4 = 0;
    uint8_t groupId = 0;
    uint8_t unkGroupRelated = 0;
    std::array<uint8_t, 4> color0{};
    std::array<uint8_t, 4> color1{};
    glm::vec4 boundMin{0.0f};
    glm::vec4 boundMax{0.0f};
    glm::vec4 unkVec4{0.0f};
};

struct M3Vertex {
    glm::vec3 position{0.0f};
    glm::vec3 tangent{0.0f};
    glm::vec3 normal{0.0f};
    glm::vec3 bitangent{0.0f};
    glm::uvec4 boneIndices{0};
    glm::vec4 boneWeights{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec4 color{1.0f};
    glm::vec4 blend{0.0f};
    glm::vec2 uv1{0.0f};
    glm::vec2 uv2{0.0f};
};

struct M3Geometry {
    uint32_t nrVertices = 0;
    uint16_t vertexSize = 0;
    int16_t vertexFlags = 0;
    std::array<uint8_t, 11> fieldTypes{};
    uint32_t nrIndices = 0;
    int16_t indexFlags = 0;
    uint32_t ofsIndices = 0;
    uint32_t nrSubmeshes = 0;
    uint32_t ofsSubmeshes = 0;

    std::vector<M3Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<M3Submesh> submeshes;
};

struct M3Light {
    uint16_t boneId = 0;
    uint16_t unk00 = 0;
    int16_t unk01 = 0;
    int16_t unk02 = 0;
    int16_t unk03 = 0;
    std::array<int16_t, 39> values1{};
    M3TrackDef trackdefs[6]{};
    std::array<int16_t, 60> values2{};
    std::array<int16_t, 24> values3{};
};

struct M3SubmeshGroup {
    uint16_t submeshId = 0;
    uint16_t unk1 = 0;
};

struct M3Header {
    char signature[4]{};
    uint32_t version = 0;
    uint32_t unk008 = 0;

    M3MetaDef animationsMeta{};
    M3TrackDef trackdefAnim[4]{};
    M3MetaDef struct080{};
    M3TrackDef trackdef090{};
    M3TrackDef trackdef0A8{};
    M3TrackDef trackdef0C0{};
    M3TrackDef trackdef0D8{};
    M3MetaDef struct0F0{};
    M3TrackDef trackdef100{};
    M3TrackDef trackdef118{};
    M3TrackDef trackdef130{};
    M3TrackDef trackdef148{};
    M3TrackDef trackdef160{};
    float unkFloat178 = 0.0f;
    M3MetaDef bones{};
    M3MetaDef lut190{};
    M3MetaDef lut1A0{};
    M3MetaDef lutBoneIds{};
    M3MetaDef textures{};
    M3MetaDef lut1D0{};
    M3MetaDef struct1E0{};
    M3MetaDef materials{};
    M3MetaDef submeshIds{};
    M3MetaDef lut210{};
    M3MetaDef geometry{};
    M3MetaDef lut260{};
    M3MetaDef lut270{};
    M3MetaDef lut280{};
    M3TrackDef trackdef290{};
    M3MetaDef struct2B8{};
    M3MetaDef lut2C8{};
    M3MetaDef struct2F8{};
    M3MetaDef struct308{};
    M3MetaDef lights{};
    M3MetaDef struct328{};
    M3MetaDef lut338{};
    int64_t idUnk348 = 0;
    M3TrackDef trackdef350{};
    int64_t idUnk368 = 0;
    M3TrackDef trackdef370{};
    float floatUnk380[2]{};
    M3Bounds bounds[5]{};
    M3MetaDef struct490{};
    M3MetaDef lut4A0{};
    float floatUnk4F0 = 0.0f;
    float floatUnk4F8 = 0.0f;
    glm::vec3 posUnk500{0.0f};
    M3MetaDef lut510{};
    M3MetaDef lut520{};
    M3MetaDef lut530{};
    M3MetaDef struct540{};
    M3MetaDef lut550{};
    M3MetaDef struct560{};
    M3MetaDef struct570{};
    int64_t idUnk580 = 0;
    M3MetaDef struct588{};
    M3MetaDef customBoneMinMax{};
    M3MetaDef lutBoneToCustom{};
    M3TrackDef trackdef5C0{};
    float floatUnk5D8 = 0.0f;
    float floatUnk5E0 = 0.0f;
    float floatUnk5E8 = 0.0f;
    float floatUnk5F0 = 0.0f;
    float floatUnk5F8 = 0.0f;
    float floatUnk600 = 0.0f;
    float floatUnk608 = 0.0f;
    float floatUnk610 = 0.0f;
    float floatUnk618 = 0.0f;
    float floatUnk620 = 0.0f;
};

struct M3ModelData {
    M3Header header;
    M3Geometry geometry;
    std::vector<M3Bone> bones;
    std::vector<M3Texture> textures;
    std::vector<M3Material> materials;
    std::vector<M3ModelAnimation> animations;
    std::vector<M3Light> lights;
    std::vector<M3SubmeshGroup> submeshGroups;

    std::vector<int16_t> lutBoneMapping;
    std::vector<int16_t> lut190;
    std::vector<int16_t> lut1A0;
    std::vector<int16_t> lut1D0;
    std::vector<int16_t> lut210;
    std::vector<int16_t> lut260;
    std::vector<int16_t> lut270;
    std::vector<int16_t> lut280;
    std::vector<int16_t> lut338;
    std::vector<int16_t> lut4A0;
    std::vector<glm::vec4> lut510;
    std::vector<int32_t> lut520;
    std::vector<int32_t> lut530;
    std::vector<int32_t> lut550;

    bool success = false;
};

#pragma pack(pop)