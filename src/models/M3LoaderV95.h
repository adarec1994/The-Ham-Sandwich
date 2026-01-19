#pragma once
#include "M3Common.h"
#include "../Archive.h"
#include <memory>
#include <cstdint>
#include <vector>

class M3LoaderV95 {
public:
    static M3ModelData Load(const std::vector<uint8_t>& buffer);
    static bool CanLoad(const std::vector<uint8_t>& buffer);

private:
    static constexpr size_t HEADER_SIZE_V95 = 0x650;  // 1616 bytes
    static constexpr size_t VERTEX_SIZE_V95 = 48;
    static constexpr size_t SUBMESH_SIZE_V95 = 88;    // Based on the struct
    static constexpr size_t TEXTURE_ENTRY_SIZE = 32;
    static constexpr size_t MATERIAL_SIZE = 48;
    static constexpr size_t SKIN_SIZE = 72;

#pragma pack(push, 1)
    struct M3HeaderV95
    {
        uint8_t various1[0x1A0];      // 416 bytes
        uint64_t nTextures;
        uint64_t ofsTextures;
        uint8_t various5[0x20];       // 32 bytes
        uint64_t nMaterials;
        uint64_t ofsMaterials;
        uint8_t various2[0x20];       // 32 bytes
        uint64_t nVertices;
        uint64_t ofsVertices;
        uint64_t nIndices;
        uint64_t ofsIndices;
        uint64_t nSubMeshes;
        uint64_t ofsSubMeshes;
        uint64_t various3[7];         // 56 bytes
        uint64_t nViews;
        uint64_t ofsViews;
        uint8_t various4[0x3E8];      // 1000 bytes
    };

    struct M3VertexV95
    {
        float x, y, z;
        uint8_t boneIndices[4];
        int8_t normals[4];
        int8_t tangents[4];
        uint32_t unk[4];
        uint16_t s, t, u, v;
    };

    struct M3SubMeshV95
    {
        uint32_t startIndex;
        uint32_t startVertex;
        uint32_t nIndices;
        uint32_t nVertices;
        uint32_t unk1;
        uint16_t unk2;
        uint16_t material;
        uint32_t unk8;
        uint32_t color2;
        uint32_t unk3, unk4, unk5, unk6;
        uint32_t color3;
        uint32_t color4;
        uint32_t unk7;
        uint8_t pad[36];
    };

    struct M3SkinV95
    {
        uint64_t sizeOfStruct;
        uint64_t nVertexLookup;
        uint64_t ofsVertexLookup;
        uint64_t nUnk1;
        uint64_t ofsUnk1;
        uint64_t nIndexLookup;
        uint64_t ofsIndexLookup;
        uint64_t nUnk2;
        uint64_t ofsUnk2;
    };

    struct M3TextureV95
    {
        uint32_t unk[4];
        uint64_t lenName;
        uint64_t ofsName;
    };

    struct M3MaterialV95
    {
        uint32_t unk1[8];
        uint64_t nTextures;
        uint64_t ofsTextures;
    };
#pragma pack(pop)

    static float HalfToFloat(uint16_t h);

    template<typename T>
    static T Read(const uint8_t* data, size_t offset) {
        T val;
        std::memcpy(&val, data + offset, sizeof(T));
        return val;
    }
};