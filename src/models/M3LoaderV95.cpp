#include "M3LoaderV95.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <iomanip>

bool M3LoaderV95::CanLoad(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < 8) return false;

    char sig[5] = {0};
    std::memcpy(sig, buffer.data(), 4);
    uint32_t version = Read<uint32_t>(buffer.data(), 4);

    if (sig[0] != 'M' && sig[0] != 'L') return false;
    return (version >= 90 && version < 100);
}

float M3LoaderV95::HalfToFloat(uint16_t h) {
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

M3ModelData M3LoaderV95::Load(const std::vector<uint8_t>& buffer) {
    M3ModelData model;

    if (buffer.size() < HEADER_SIZE_V95) {
        std::cerr << "[M3V95] Buffer too small: " << buffer.size() << " < " << HEADER_SIZE_V95 << std::endl;
        return model;
    }

    const uint8_t* ptr = buffer.data();
    size_t size = buffer.size();

    char sig[5] = {0};
    std::memcpy(sig, ptr, 4);
    uint32_t version = Read<uint32_t>(ptr, 4);

    std::cout << "[M3V95] Loading version " << version << " file, size=" << size << std::endl;

    // =========================================================================
    // DEBUG: Dump header to find correct offsets
    // =========================================================================

    std::cout << "[M3V95] Scanning header for non-zero uint64 pairs (count, offset):" << std::endl;
    for (size_t ofs = 0x100; ofs < 0x650; ofs += 16) {
        uint64_t val1 = Read<uint64_t>(ptr, ofs);
        uint64_t val2 = Read<uint64_t>(ptr, ofs + 8);

        // Look for reasonable count/offset pairs
        if (val1 > 0 && val1 < 100000 && val2 > 0 && val2 < size) {
            std::cout << "[M3V95]   0x" << std::hex << ofs << ": count=" << std::dec << val1
                      << " offset=0x" << std::hex << val2 << std::dec << std::endl;
        }
    }

    // =========================================================================
    // V95 Header Layout (determined from analysis):
    // V95 does NOT have nVertices/ofsVertices/nIndices/ofsIndices at 0x200-0x218.
    // The geometry data is accessed through the Views/Skin structure.
    //
    // Known V95 offsets:
    //   0x1a0: nTextures
    //   0x1a8: ofsTextures
    //   0x1d0: nMaterials
    //   0x1d8: ofsMaterials
    //   0x230: nSubMeshes
    //   0x238: ofsSubMeshes
    //   0x470: nViews
    //   0x478: ofsViews
    // =========================================================================

    uint64_t nTextures = Read<uint64_t>(ptr, 0x1A0);
    uint64_t ofsTextures = Read<uint64_t>(ptr, 0x1A8);
    uint64_t nMaterials = Read<uint64_t>(ptr, 0x1D0);
    uint64_t ofsMaterials = Read<uint64_t>(ptr, 0x1D8);
    uint64_t nSubMeshes = Read<uint64_t>(ptr, 0x230);
    uint64_t ofsSubMeshes = Read<uint64_t>(ptr, 0x238);
    uint64_t nViews = Read<uint64_t>(ptr, 0x470);
    uint64_t ofsViews = Read<uint64_t>(ptr, 0x478);

    std::cout << "[M3V95] Header fields:" << std::endl;
    std::cout << "[M3V95]   Textures:  " << nTextures << " @ 0x" << std::hex << ofsTextures << std::dec << std::endl;
    std::cout << "[M3V95]   Materials: " << nMaterials << " @ 0x" << std::hex << ofsMaterials << std::dec << std::endl;
    std::cout << "[M3V95]   Submeshes: " << nSubMeshes << " @ 0x" << std::hex << ofsSubMeshes << std::dec << std::endl;
    std::cout << "[M3V95]   Views:     " << nViews << " @ 0x" << std::hex << ofsViews << std::dec << std::endl;

    // DEBUG: Dump bytes at views offset to understand skin structure
    if (ofsViews > 0 && ofsViews + 72 <= size) {
        std::cout << "[M3V95] Raw bytes at views offset 0x" << std::hex << ofsViews << std::dec << ":" << std::endl;
        std::cout << "[M3V95]   ";
        for (size_t i = 0; i < 72; i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)ptr[ofsViews + i] << " ";
            if ((i + 1) % 8 == 0) std::cout << " ";
            if ((i + 1) % 16 == 0) std::cout << std::endl << "[M3V95]   ";
        }
        std::cout << std::dec << std::endl;

        std::cout << "[M3V95] As uint64s:" << std::endl;
        for (size_t i = 0; i < 9; i++) {
            uint64_t val = Read<uint64_t>(ptr, ofsViews + i * 8);
            std::cout << "[M3V95]   +" << (i * 8) << ": " << val << " (0x" << std::hex << val << std::dec << ")" << std::endl;
        }
    }

    if (nViews == 0 || ofsViews == 0) {
        std::cerr << "[M3V95] No views found in model" << std::endl;
        return model;
    }

    // Read the Skin/View structure
    // NOTE: Offsets might be ABSOLUTE (from file start) not relative to header end
    size_t skinStart = ofsViews;  // Try absolute offset first

    std::cout << "[M3V95] Trying skin at absolute offset 0x" << std::hex << skinStart << std::dec << std::endl;

    if (skinStart + sizeof(M3SkinV95) > size) {
        // Try relative offset
        skinStart = HEADER_SIZE_V95 + ofsViews;
        std::cout << "[M3V95] Absolute failed, trying relative offset 0x" << std::hex << skinStart << std::dec << std::endl;
    }

    if (skinStart + sizeof(M3SkinV95) > size) {
        std::cerr << "[M3V95] Skin data out of bounds" << std::endl;
        return model;
    }

    // DEBUG: Dump bytes at skin offset
    std::cout << "[M3V95] Raw bytes at skin offset 0x" << std::hex << skinStart << std::dec << ":" << std::endl;
    std::cout << "[M3V95]   ";
    for (size_t i = 0; i < 72 && skinStart + i < size; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)ptr[skinStart + i] << " ";
        if ((i + 1) % 8 == 0) std::cout << " ";
        if ((i + 1) % 16 == 0) std::cout << std::endl << "[M3V95]   ";
    }
    std::cout << std::dec << std::endl;

    // Try reading as uint64s
    std::cout << "[M3V95] As uint64s:" << std::endl;
    for (size_t i = 0; i < 9; i++) {
        uint64_t val = Read<uint64_t>(ptr, skinStart + i * 8);
        std::cout << "[M3V95]   +" << (i * 8) << ": " << val << " (0x" << std::hex << val << std::dec << ")" << std::endl;
    }

    M3SkinV95 skin;
    std::memcpy(&skin, ptr + skinStart, sizeof(M3SkinV95));

    std::cout << "[M3V95] Skin structure:" << std::endl;
    std::cout << "[M3V95]   sizeOfStruct: " << skin.sizeOfStruct << std::endl;
    std::cout << "[M3V95]   vertexLookup: " << skin.nVertexLookup << " @ " << skin.ofsVertexLookup << std::endl;
    std::cout << "[M3V95]   unk1:         " << skin.nUnk1 << " @ " << skin.ofsUnk1 << std::endl;
    std::cout << "[M3V95]   indexLookup:  " << skin.nIndexLookup << " @ " << skin.ofsIndexLookup << std::endl;
    std::cout << "[M3V95]   unk2:         " << skin.nUnk2 << " @ " << skin.ofsUnk2 << std::endl;

    size_t skinDataBase = skinStart + skin.sizeOfStruct;

    // =========================================================================
    // Scan the file for vertex data (3 floats that look like coordinates)
    // =========================================================================

    std::cout << "[M3V95] Scanning file for vertex data..." << std::endl;

    uint64_t nVertices = 0;
    size_t vertexFileOffset = 0;

    for (size_t scanOfs = HEADER_SIZE_V95; scanOfs < size - VERTEX_SIZE_V95 * 10; scanOfs += 16) {
        float v1x = Read<float>(ptr, scanOfs);
        float v1y = Read<float>(ptr, scanOfs + 4);
        float v1z = Read<float>(ptr, scanOfs + 8);

        if (std::isnan(v1x) || std::isnan(v1y) || std::isnan(v1z)) continue;
        if (std::abs(v1x) > 10000 || std::abs(v1y) > 10000 || std::abs(v1z) > 10000) continue;
        if (v1x == 0 && v1y == 0 && v1z == 0) continue;

        // Check if subsequent vertices are also valid
        bool allValid = true;
        int validCount = 0;
        for (int i = 1; i < 10 && scanOfs + i * VERTEX_SIZE_V95 + 12 < size; i++) {
            size_t nextOfs = scanOfs + i * VERTEX_SIZE_V95;
            float vx = Read<float>(ptr, nextOfs);
            float vy = Read<float>(ptr, nextOfs + 4);
            float vz = Read<float>(ptr, nextOfs + 8);

            if (std::isnan(vx) || std::isnan(vy) || std::isnan(vz)) {
                allValid = false;
                break;
            }
            if (std::abs(vx) > 10000 || std::abs(vy) > 10000 || std::abs(vz) > 10000) {
                allValid = false;
                break;
            }
            validCount++;
        }

        if (allValid && validCount >= 5) {
            // Count total vertices
            size_t count = 0;
            for (size_t i = 0; scanOfs + i * VERTEX_SIZE_V95 + 12 < size; i++) {
                size_t vOfs = scanOfs + i * VERTEX_SIZE_V95;
                float vx = Read<float>(ptr, vOfs);
                float vy = Read<float>(ptr, vOfs + 4);
                float vz = Read<float>(ptr, vOfs + 8);

                if (std::isnan(vx) || std::isnan(vy) || std::isnan(vz)) break;
                if (std::abs(vx) > 10000 || std::abs(vy) > 10000 || std::abs(vz) > 10000) break;
                count++;
                if (count > 100000) break;
            }

            if (count >= 10) {
                std::cout << "[M3V95] Found vertex data!" << std::endl;
                std::cout << "[M3V95]   Offset: 0x" << std::hex << scanOfs << std::dec << std::endl;
                std::cout << "[M3V95]   Count:  " << count << std::endl;
                std::cout << "[M3V95]   First vertex: (" << v1x << ", " << v1y << ", " << v1z << ")" << std::endl;

                nVertices = count;
                vertexFileOffset = scanOfs;
                break;
            }
        }
    }

    if (nVertices == 0) {
        std::cerr << "[M3V95] Could not find vertex data in file" << std::endl;
        return model;
    }

    // =========================================================================
    // Find index data - try skin's indexLookup first
    // =========================================================================

    std::cout << "[M3V95] Looking for index data..." << std::endl;

    uint64_t nIndices = 0;
    size_t indexFileOffset = 0;

    // Try skin index lookup
    if (skin.nIndexLookup > 0 && skin.ofsIndexLookup > 0) {
        size_t skinIndexOfs = skinDataBase + skin.ofsIndexLookup;
        std::cout << "[M3V95]   Testing skin indexLookup at 0x" << std::hex << skinIndexOfs << std::dec << std::endl;

        if (skinIndexOfs + skin.nIndexLookup * 4 <= size) {
            bool valid = true;
            for (size_t i = 0; i < std::min((uint64_t)10, skin.nIndexLookup); i++) {
                uint32_t idx = Read<uint32_t>(ptr, skinIndexOfs + i * 4);
                if (idx >= nVertices) {
                    valid = false;
                    break;
                }
            }

            if (valid) {
                nIndices = skin.nIndexLookup;
                indexFileOffset = skinIndexOfs;
                std::cout << "[M3V95]   Found indices via skin: " << nIndices << " @ 0x" << std::hex << indexFileOffset << std::dec << std::endl;
            }
        }
    }

    // Scan for indices if not found
    if (nIndices == 0) {
        for (size_t scanOfs = HEADER_SIZE_V95; scanOfs < vertexFileOffset && scanOfs < size - 40; scanOfs += 4) {
            size_t count = 0;
            for (size_t i = 0; scanOfs + i * 4 + 4 <= size && i < 50000; i++) {
                uint32_t idx = Read<uint32_t>(ptr, scanOfs + i * 4);
                if (idx >= nVertices) break;
                count++;
            }

            if (count >= 100) {
                nIndices = count;
                indexFileOffset = scanOfs;
                std::cout << "[M3V95]   Found indices via scan: " << nIndices << " @ 0x" << std::hex << indexFileOffset << std::dec << std::endl;
                break;
            }
        }
    }

    std::cout << "[M3V95] Final geometry:" << std::endl;
    std::cout << "[M3V95]   Vertices: " << nVertices << " @ 0x" << std::hex << vertexFileOffset << std::dec << std::endl;
    std::cout << "[M3V95]   Indices:  " << nIndices << " @ 0x" << std::hex << indexFileOffset << std::dec << std::endl;

    // =========================================================================
    // Read vertex data
    // =========================================================================

    std::vector<M3VertexV95> rawVertices(nVertices);
    if (vertexFileOffset + nVertices * VERTEX_SIZE_V95 <= size) {
        std::memcpy(rawVertices.data(), ptr + vertexFileOffset, nVertices * VERTEX_SIZE_V95);
    } else {
        std::cerr << "[M3V95] Vertex data extends past end of file" << std::endl;
        return model;
    }

    // =========================================================================
    // Read index data
    // =========================================================================

    std::vector<uint32_t> rawIndices;
    if (nIndices > 0 && indexFileOffset + nIndices * 4 <= size) {
        rawIndices.resize(nIndices);
        std::memcpy(rawIndices.data(), ptr + indexFileOffset, nIndices * sizeof(uint32_t));
    }

    // =========================================================================
    // Read submeshes
    // =========================================================================

    size_t submeshStart = HEADER_SIZE_V95 + ofsSubMeshes;
    std::vector<M3SubMeshV95> rawSubmeshes;
    if (nSubMeshes > 0 && submeshStart + nSubMeshes * sizeof(M3SubMeshV95) <= size) {
        rawSubmeshes.resize(nSubMeshes);
        std::memcpy(rawSubmeshes.data(), ptr + submeshStart, nSubMeshes * sizeof(M3SubMeshV95));

        for (size_t i = 0; i < rawSubmeshes.size(); i++) {
            std::cout << "[M3V95] Submesh " << i << ": startIndex=" << rawSubmeshes[i].startIndex
                      << " startVertex=" << rawSubmeshes[i].startVertex
                      << " nIndices=" << rawSubmeshes[i].nIndices
                      << " nVertices=" << rawSubmeshes[i].nVertices
                      << " material=" << rawSubmeshes[i].material << std::endl;
        }
    }

    // =========================================================================
    // Convert vertices
    // =========================================================================

    model.geometry.vertices.resize(nVertices);
    for (size_t i = 0; i < nVertices; i++) {
        const auto& src = rawVertices[i];
        auto& dst = model.geometry.vertices[i];

        dst.position = glm::vec3(src.x, -src.y, src.z);
        dst.normal = glm::vec3(
            src.normals[0] / 127.0f,
            -src.normals[1] / 127.0f,
            src.normals[2] / 127.0f
        );
        dst.tangent = glm::vec3(
            src.tangents[0] / 127.0f,
            src.tangents[1] / 127.0f,
            src.tangents[2] / 127.0f
        );
        dst.uv1 = glm::vec2(HalfToFloat(src.s), HalfToFloat(src.t));
        dst.uv2 = glm::vec2(HalfToFloat(src.u), HalfToFloat(src.v));
        dst.boneIndices = glm::uvec4(src.boneIndices[0], src.boneIndices[1], src.boneIndices[2], src.boneIndices[3]);
        dst.boneWeights = glm::vec4(0.25f, 0.25f, 0.25f, 0.25f);
    }

    model.geometry.nrVertices = static_cast<uint32_t>(nVertices);
    model.geometry.vertexSize = VERTEX_SIZE_V95;

    // =========================================================================
    // Build indices
    // =========================================================================

    if (!rawSubmeshes.empty() && !rawIndices.empty()) {
        for (size_t smIdx = 0; smIdx < rawSubmeshes.size(); smIdx++) {
            const auto& srcSm = rawSubmeshes[smIdx];
            M3Submesh sm;

            sm.startIndex = static_cast<uint32_t>(model.geometry.indices.size());
            sm.startVertex = srcSm.startVertex;
            sm.indexCount = srcSm.nIndices;
            sm.vertexCount = srcSm.nVertices;
            sm.materialID = srcSm.material;

            for (uint32_t j = 0; j < srcSm.nIndices && (srcSm.startIndex + j) < rawIndices.size(); j++) {
                uint32_t index = rawIndices[srcSm.startIndex + j];
                model.geometry.indices.push_back(index);
            }

            model.geometry.submeshes.push_back(sm);
        }
    } else if (!rawIndices.empty()) {
        M3Submesh sm;
        sm.startIndex = 0;
        sm.startVertex = 0;
        sm.indexCount = static_cast<uint32_t>(rawIndices.size());
        sm.vertexCount = static_cast<uint32_t>(nVertices);
        sm.materialID = 0;

        model.geometry.indices = rawIndices;
        model.geometry.submeshes.push_back(sm);
    }

    model.geometry.nrIndices = static_cast<uint32_t>(model.geometry.indices.size());
    model.geometry.nrSubmeshes = static_cast<uint32_t>(model.geometry.submeshes.size());

    // =========================================================================
    // Read textures
    // =========================================================================

    if (nTextures > 0 && ofsTextures > 0) {
        size_t texStart = HEADER_SIZE_V95 + ofsTextures;
        size_t texEnd = texStart + nTextures * TEXTURE_ENTRY_SIZE;
        texEnd = (texEnd + 15) & ~(size_t)15;

        for (uint64_t i = 0; i < nTextures; i++) {
            size_t entryOfs = texStart + i * TEXTURE_ENTRY_SIZE;
            if (entryOfs + TEXTURE_ENTRY_SIZE > size) break;

            M3TextureV95 texEntry;
            std::memcpy(&texEntry, ptr + entryOfs, sizeof(M3TextureV95));

            M3Texture tex;
            tex.textureType = static_cast<int32_t>(i);

            if (texEntry.ofsName > 0 && texEntry.lenName > 0) {
                size_t nameOfs = texEnd + texEntry.ofsName;
                if (nameOfs + texEntry.lenName * 2 <= size) {
                    const wchar_t* wname = reinterpret_cast<const wchar_t*>(ptr + nameOfs);
                    std::wstring wstr(wname, texEntry.lenName);
                    tex.path.assign(wstr.begin(), wstr.end());
                }
            }

            model.textures.push_back(tex);
        }
    }

    std::cout << "[M3V95] Loaded " << model.geometry.vertices.size() << " vertices, "
              << model.geometry.indices.size() << " indices, "
              << model.geometry.submeshes.size() << " submeshes, "
              << model.textures.size() << " textures" << std::endl;

    model.success = !model.geometry.vertices.empty();
    return model;
}