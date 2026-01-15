#include "M3Loader.h"
#include <iostream>
#include <cstring>
#include <cmath>
#include <algorithm>

static constexpr size_t M3_HEADER_SIZE = 1584;
static constexpr size_t M3_TEX_ENTRY_SIZE = 32;
static constexpr size_t M3_MAT_ENTRY_SIZE = 48;
static constexpr size_t M3_MAT_DESC_SIZE = 296;
static constexpr size_t M3_GEOM_SIZE = 208;
static constexpr size_t M3_SUBMESH_SIZE = 112;

float M3Loader::HalfToFloat(uint16_t h) {
    uint32_t s = (h & 0x8000) << 16;
    int32_t e = (h & 0x7C00) >> 10;
    uint32_t m = (h & 0x03FF) << 13;
    uint32_t v;
    if (e == 0) v = (m == 0) ? s : s | 0x00800000 | (m << 1);
    else if (e == 0x1F) v = s | 0x7F800000 | m;
    else v = s | ((e + 112) << 23) | m;
    float f;
    std::memcpy(&f, &v, sizeof(f));
    return f;
}

glm::vec3 M3Loader::ReadVertexV3(const uint8_t* data, uint8_t type, int& offset) {
    glm::vec3 res(0.0f);
    if (type == 1) {
        std::memcpy(&res, data + offset, 12);
        offset += 12;
    } else if (type == 2) {
        int16_t xyz[3];
        std::memcpy(xyz, data + offset, 6);
        res = glm::vec3(xyz[0], xyz[1], xyz[2]) / 1024.0f;
        offset += 6;
    } else if (type == 3) {
        uint8_t x = data[offset];
        uint8_t y = data[offset + 1];
        float fx = (x - 127.0f) / 127.0f;
        float fy = (y - 127.0f) / 127.0f;
        float fz = std::sqrt(std::max(1.0f - fx*fx - fy*fy, 0.0f));
        res = glm::vec3(fx, fy, fz);
        offset += 2;
    }
    return res;
}

glm::vec2 M3Loader::ReadVertexV2(const uint8_t* data, uint8_t type, int& offset) {
    glm::vec2 res(0.0f);
    if (type == 5) {
        uint16_t xy[2];
        std::memcpy(xy, data + offset, 4);
        res.x = HalfToFloat(xy[0]);
        res.y = HalfToFloat(xy[1]);
        offset += 4;
    }
    return res;
}

M3ModelData M3Loader::LoadFromFile(const ArchivePtr& arc, const std::shared_ptr<FileEntry>& entry) {
    if (!arc || !entry) return {};
    std::vector<uint8_t> buffer;
    arc->getFileData(entry, buffer);
    if (buffer.empty()) return {};
    return Load(buffer);
}

M3ModelData M3Loader::Load(const std::vector<uint8_t>& buffer) {
    M3ModelData model;
    if (buffer.size() < M3_HEADER_SIZE) return model;

    const uint8_t* ptr = buffer.data();
    size_t fileSize = buffer.size();

    uint64_t texCount = 0, texOffset = 0;
    std::memcpy(&texCount, ptr + 0x1C0, 8);
    std::memcpy(&texOffset, ptr + 0x1C8, 8);

    uint64_t matCount = 0, matOffset = 0;
    std::memcpy(&matCount, ptr + 0x1F0, 8);
    std::memcpy(&matOffset, ptr + 0x1F8, 8);

    uint64_t geomCount = 0, geomOffset = 0;
    std::memcpy(&geomCount, ptr + 0x250, 8);
    std::memcpy(&geomOffset, ptr + 0x258, 8);

    if (texCount > 0) {
        size_t texTableStart = M3_HEADER_SIZE + texOffset;
        size_t texDataStart = texTableStart + (texCount * M3_TEX_ENTRY_SIZE);
        for (uint64_t i = 0; i < texCount; ++i) {
            size_t entryPos = texTableStart + (i * M3_TEX_ENTRY_SIZE);
            if (entryPos + M3_TEX_ENTRY_SIZE > fileSize) break;

            uint64_t nrLetters = 0, ofs = 0;
            std::memcpy(&nrLetters, ptr + entryPos + 16, 8);
            std::memcpy(&ofs, ptr + entryPos + 24, 8);

            size_t strPos = texDataStart + ofs;
            size_t byteLen = size_t(nrLetters) * 2;
            if (strPos + byteLen <= fileSize) {
                std::u16string u16name(reinterpret_cast<const char16_t*>(ptr + strPos), size_t(nrLetters));
                std::string name;
                name.reserve(u16name.size());
                for (char16_t c : u16name) name.push_back((c < 128) ? char(c) : '_');
                model.textures.push_back(name);
            }
        }
    }

    if (matCount > 0) {
        size_t matTableStart = M3_HEADER_SIZE + matOffset;
        size_t matDataStart = matTableStart + (matCount * M3_MAT_ENTRY_SIZE);

        for (uint64_t i = 0; i < matCount; ++i) {
            size_t entryPos = matTableStart + (i * M3_MAT_ENTRY_SIZE);
            if (entryPos + M3_MAT_ENTRY_SIZE > fileSize) break;

            uint64_t nrDesc = 0, ofsDesc = 0;
            std::memcpy(&nrDesc, ptr + entryPos + 32, 8);
            std::memcpy(&ofsDesc, ptr + entryPos + 40, 8);

            M3MaterialData mat;
            if (nrDesc > 0) {
                size_t descBase = matDataStart + ofsDesc;
                for (uint64_t d = 0; d < nrDesc; ++d) {
                    size_t descPos = descBase + (d * M3_MAT_DESC_SIZE);
                    if (descPos + 4 > fileSize) break;

                    M3MaterialVariant v{-1, -1};
                    std::memcpy(&v.textureIndexA, ptr + descPos + 0, 2);
                    std::memcpy(&v.textureIndexB, ptr + descPos + 2, 2);
                    mat.variants.push_back(v);
                }
            }
            if (mat.variants.empty()) mat.variants.push_back(M3MaterialVariant{-1, -1});
            model.materials.push_back(std::move(mat));
        }
    }

    if (geomOffset == 0) return model;
    size_t absGeomOffset = M3_HEADER_SIZE + geomOffset;
    if (absGeomOffset + M3_GEOM_SIZE > fileSize) return model;

    const uint8_t* gPtr = ptr + absGeomOffset;

    uint32_t nrVertices = 0;
    uint16_t vertexSize = 0;
    int16_t vertexFlags = 0;
    uint8_t fieldTypes[11]{};
    std::memcpy(&nrVertices, gPtr + 0x18, 4);
    std::memcpy(&vertexSize, gPtr + 0x1C, 2);
    std::memcpy(&vertexFlags, gPtr + 0x1E, 2);
    std::memcpy(fieldTypes, gPtr + 0x20, 11);

    uint32_t nrIndices = 0, ofsIndices = 0;
    int16_t indexFlags = 0;
    std::memcpy(&nrIndices, gPtr + 0x68, 4);
    std::memcpy(&indexFlags, gPtr + 0x6C, 2);
    std::memcpy(&ofsIndices, gPtr + 0x78, 4);

    uint32_t nrSubmeshes = 0, ofsSubmeshes = 0;
    std::memcpy(&nrSubmeshes, gPtr + 0x80, 4);
    std::memcpy(&ofsSubmeshes, gPtr + 0x88, 4);

    size_t vertexStart = absGeomOffset + M3_GEOM_SIZE;
    size_t vertexBytes = size_t(nrVertices) * size_t(vertexSize);
    if (vertexStart + vertexBytes > fileSize) return model;

    model.vertices.reserve(nrVertices);
    for (uint32_t i = 0; i < nrVertices; ++i) {
        const uint8_t* vData = ptr + vertexStart + (size_t(i) * size_t(vertexSize));
        M3Vertex v{};
        int localOffset = 0;
        if (vertexFlags & 0x0001) v.position = ReadVertexV3(vData, fieldTypes[0], localOffset);
        if (vertexFlags & 0x0002) ReadVertexV3(vData, fieldTypes[1], localOffset);
        if (vertexFlags & 0x0004) v.normal = ReadVertexV3(vData, fieldTypes[2], localOffset);
        if (vertexFlags & 0x0008) ReadVertexV3(vData, fieldTypes[3], localOffset);
        if (vertexFlags & 0x0010) localOffset += 4;
        if (vertexFlags & 0x0020) localOffset += 4;
        if (vertexFlags & 0x0040) localOffset += 4;
        if (vertexFlags & 0x0080) localOffset += 16;
        if (vertexFlags & 0x0100) v.uv = ReadVertexV2(vData, fieldTypes[8], localOffset);
        model.vertices.push_back(v);
    }

    size_t indexStart = absGeomOffset + M3_GEOM_SIZE + ofsIndices;
    bool is32Bit = (indexFlags & 0x0200) == 512;
    size_t idxStride = is32Bit ? 4 : 2;
    size_t indexBytes = size_t(nrIndices) * idxStride;
    if (indexStart + indexBytes > fileSize) return model;

    model.indices.reserve(nrIndices);
    for (uint32_t i = 0; i < nrIndices; ++i) {
        if (is32Bit) {
            uint32_t idx;
            std::memcpy(&idx, ptr + indexStart + (size_t(i) * 4), 4);
            model.indices.push_back(idx);
        } else {
            uint16_t idx;
            std::memcpy(&idx, ptr + indexStart + (size_t(i) * 2), 2);
            model.indices.push_back(uint32_t(idx));
        }
    }

    size_t submeshStart = absGeomOffset + M3_GEOM_SIZE + ofsSubmeshes;
    size_t submeshBytes = size_t(nrSubmeshes) * M3_SUBMESH_SIZE;
    if (submeshStart + submeshBytes > fileSize) return model;

    model.submeshes.reserve(nrSubmeshes);
    for (uint32_t i = 0; i < nrSubmeshes; ++i) {
        size_t smOfs = submeshStart + (size_t(i) * M3_SUBMESH_SIZE);
        M3Submesh sm{};
        std::memcpy(&sm.startIndex, ptr + smOfs + 0, 4);
        std::memcpy(&sm.startVertex, ptr + smOfs + 4, 4);
        std::memcpy(&sm.indexCount, ptr + smOfs + 8, 4);
        std::memcpy(&sm.vertexCount, ptr + smOfs + 12, 4);
        std::memcpy(&sm.materialID, ptr + smOfs + 22, 2);
        model.submeshes.push_back(sm);
    }

    model.success = true;
    return model;
}
