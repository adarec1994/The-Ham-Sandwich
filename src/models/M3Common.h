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
    uint32_t timestamp;
    glm::vec3 scale;
    glm::quat rotation;
    glm::vec3 translation;
    glm::vec4 unknown;

    float getTimeSeconds() const { return timestamp / 1000.0f; }
};

struct M3AnimationTrack {
    int64_t duration;
    int64_t timeOffset;
    int64_t valueOffset;
    int trackType;
    std::vector<M3KeyFrame> keyframes;
};

struct M3Bone {
    std::string name;
    int32_t id;
    int16_t globalId;
    uint16_t flags;
    int16_t parentId;
    int16_t unk01;
    uint8_t unk02, unk03, unk04, unk05;
    uint32_t unk06;

    M3AnimationTrack tracks[8];

    glm::mat4 globalMatrix;
    glm::mat4 inverseGlobalMatrix;
    glm::vec3 position;
    std::string parentPath;
};

struct M3ModelAnimation {
    uint16_t sequenceId;
    uint16_t unk1, unk2, unk3, unk4;
    uint16_t fallbackSequence;
    uint32_t timestampStart;
    uint32_t timestampEnd;
    uint16_t unk10, unk11, unk12, unk13, unk14, unk15;
    glm::vec3 bound1, bound2, bound3, bound4;
    uint32_t unk19, unk23, unk25, unk27;
    uint64_t unk28, unk29;
};

struct M3Texture {
    int16_t unk0;
    uint8_t type;
    uint8_t unk1;
    int32_t flags;
    float intensity;
    uint8_t unk4, unk5, unk6, unk7;
    uint64_t nrLetters;
    uint64_t offset;
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
    uint8_t unk0, unk1, unk2, unk3, unk4, unk5, unk6, unk7;
    uint8_t unk8, unk9, unk10, unk11;
    uint16_t unk12, unk14;
    uint32_t unk16, unk20;
    int32_t specularX, specularY;
    uint64_t nrDescriptions;
    uint64_t ofsDescriptions;
    std::vector<M3MaterialVariant> variants;
};

struct M3Submesh {
    uint32_t startIndex;
    uint32_t startVertex;
    uint32_t indexCount;
    uint32_t vertexCount;
    uint16_t startBoneMapping;
    uint16_t nrBoneMapping;
    uint16_t unk1;
    uint16_t materialID;
    uint16_t unk2, unk3, unk4;
    uint8_t groupId;
    uint8_t unkGroupRelated;
    std::array<uint8_t, 4> color0;
    std::array<uint8_t, 4> color1;
    glm::vec4 boundMin;
    glm::vec4 boundMax;
    glm::vec4 unkVec4;
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
    uint16_t boneId;
    uint16_t unk00;
    int16_t unk01, unk02, unk03;
    std::array<int16_t, 39> values1;
    M3TrackDef trackdefs[6];
    std::array<int16_t, 60> values2;
    std::array<int16_t, 24> values3;
};

struct M3SubmeshGroup {
    uint16_t submeshId;
    uint16_t unk1;
};

struct M3Header {
    char signature[4];
    uint32_t version;
    uint32_t unk008;

    M3MetaDef animationsMeta;
    M3TrackDef trackdefAnim[4];
    M3MetaDef struct080;
    M3TrackDef trackdef090, trackdef0A8, trackdef0C0, trackdef0D8;
    M3MetaDef struct0F0;
    M3TrackDef trackdef100, trackdef118, trackdef130, trackdef148, trackdef160;
    float unkFloat178;
    M3MetaDef bones;
    M3MetaDef lut190, lut1A0, lutBoneIds;
    M3MetaDef textures;
    M3MetaDef lut1D0;
    M3MetaDef struct1E0;
    M3MetaDef materials;
    M3MetaDef submeshIds;
    M3MetaDef lut210;
    M3MetaDef geometry;
    M3MetaDef lut260, lut270, lut280;
    M3TrackDef trackdef290;
    M3MetaDef struct2B8;
    M3MetaDef lut2C8;
    M3MetaDef struct2F8, struct308;
    M3MetaDef lights;
    M3MetaDef struct328;
    M3MetaDef lut338;
    int64_t idUnk348;
    M3TrackDef trackdef350;
    int64_t idUnk368;
    M3TrackDef trackdef370;
    float floatUnk380[2];
    M3Bounds bounds[5];
    M3MetaDef struct490;
    M3MetaDef lut4A0;
    float floatUnk4F0, floatUnk4F8;
    glm::vec3 posUnk500;
    M3MetaDef lut510, lut520, lut530;
    M3MetaDef struct540;
    M3MetaDef lut550;
    M3MetaDef struct560, struct570;
    int64_t idUnk580;
    M3MetaDef struct588;
    M3MetaDef customBoneMinMax;
    M3MetaDef lutBoneToCustom;
    M3TrackDef trackdef5C0;
    float floatUnk5D8, floatUnk5E0, floatUnk5E8, floatUnk5F0, floatUnk5F8;
    float floatUnk600, floatUnk608, floatUnk610, floatUnk618, floatUnk620;
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
    std::vector<int16_t> lut190, lut1A0, lut1D0, lut210;
    std::vector<int16_t> lut260, lut270, lut280, lut338, lut4A0;
    std::vector<glm::vec4> lut510;
    std::vector<int32_t> lut520, lut530, lut550;

    bool success = false;
};

#pragma pack(pop)