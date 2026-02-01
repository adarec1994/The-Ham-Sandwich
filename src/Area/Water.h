#pragma once

#include <cstdint>
#include <vector>
#include <d3d11.h>
#include <wrl/client.h>
#include <glm/glm.hpp>

using Microsoft::WRL::ComPtr;

extern bool gShowWater;
extern bool gWaterWireframe;

#pragma pack(push, 1)
struct WaterVertex
{
    float posX, posY, posZ;
    float normX, normY, normZ;
    float tanX, tanY, tanZ;
    float bitanX, bitanY, bitanZ;
    float u, v;
    uint8_t colorR, colorG, colorB, colorA;
    float unk0;
    int32_t unk1;
    uint8_t blendR, blendG, blendB, blendA;
};
#pragma pack(pop)

static_assert(sizeof(WaterVertex) == 72, "WaterVertex must be 72 bytes");

struct WaterMesh
{
    uint32_t worldWaterTypeID = 0;
    uint32_t waterLayerIDs[4] = {0};
    uint32_t unk0 = 0;
    float unk1 = 0;
    uint32_t unk2 = 0;
    float unk3 = 0;
    float unk4 = 0;
    uint32_t unk5 = 0;
    uint32_t unk6 = 0;
    float shoreLineDistance = 0;
    float unk7 = 0;
    uint32_t shoreLineWaterLayerID = 0;
    uint32_t unk8 = 0;
    uint32_t indexCount = 0;
    uint32_t vertexCount = 0;
    uint32_t unk9 = 0;
    uint32_t unk10 = 0;

    std::vector<uint32_t> indices;
    std::vector<WaterVertex> vertices;

    ComPtr<ID3D11Buffer> vertexBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;
    bool gpuReady = false;

    glm::vec3 boundsMin = glm::vec3(FLT_MAX);
    glm::vec3 boundsMax = glm::vec3(-FLT_MAX);

    bool loadFromRaw(const uint8_t* data, size_t size);
    bool createGPUBuffers(ID3D11Device* device);
    void computeBounds();
    void debugPrint() const;
};

namespace WaterRenderer
{
    bool Initialize(ID3D11Device* device);
    void Shutdown();
    void Render(ID3D11DeviceContext* context, WaterMesh* water,
                const glm::mat4& view, const glm::mat4& proj,
                const glm::vec3& areaOffset, float time);
    void SetDebugMode(bool enabled);
}