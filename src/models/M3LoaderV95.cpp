#include "M3LoaderV95.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <iostream>

bool M3LoaderV95::CanLoad(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < 8) return false;
    
    char sig[5] = {0};
    std::memcpy(sig, buffer.data(), 4);
    uint32_t version = Read<uint32_t>(buffer.data(), 4);
    
    // Check for MODL signature (little endian = "LDOM")
    if (sig[0] != 'M' && sig[0] != 'L') return false;
    
    // Version 95 or similar older versions
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

    // Check signature
    char sig[5] = {0};
    std::memcpy(sig, ptr, 4);
    uint32_t version = Read<uint32_t>(ptr, 4);
    
    std::cout << "[M3V95] Loading version " << version << " file" << std::endl;

    // Read header
    M3HeaderV95 header;
    std::memcpy(&header, ptr, sizeof(M3HeaderV95));

    std::cout << "[M3V95] Vertices: " << header.nVertices << " at offset " << header.ofsVertices << std::endl;
    std::cout << "[M3V95] Indices: " << header.nIndices << " at offset " << header.ofsIndices << std::endl;
    std::cout << "[M3V95] Submeshes: " << header.nSubMeshes << " at offset " << header.ofsSubMeshes << std::endl;
    std::cout << "[M3V95] Views: " << header.nViews << " at offset " << header.ofsViews << std::endl;
    std::cout << "[M3V95] Textures: " << header.nTextures << " at offset " << header.ofsTextures << std::endl;
    std::cout << "[M3V95] Materials: " << header.nMaterials << " at offset " << header.ofsMaterials << std::endl;

    // Validate offsets
    if (header.nVertices == 0 || header.ofsVertices == 0) {
        std::cerr << "[M3V95] No vertices in model" << std::endl;
        return model;
    }

    size_t vertexStart = HEADER_SIZE_V95 + header.ofsVertices;
    size_t vertexEnd = vertexStart + header.nVertices * VERTEX_SIZE_V95;
    
    if (vertexEnd > size) {
        std::cerr << "[M3V95] Vertex data out of bounds: " << vertexEnd << " > " << size << std::endl;
        return model;
    }

    // Read vertices
    std::vector<M3VertexV95> rawVertices(header.nVertices);
    std::memcpy(rawVertices.data(), ptr + vertexStart, header.nVertices * VERTEX_SIZE_V95);

    // Read indices
    size_t indexStart = HEADER_SIZE_V95 + header.ofsIndices;
    std::vector<uint32_t> rawIndices(header.nIndices);
    if (indexStart + header.nIndices * 4 <= size) {
        std::memcpy(rawIndices.data(), ptr + indexStart, header.nIndices * sizeof(uint32_t));
    }

    // Read submeshes
    size_t submeshStart = HEADER_SIZE_V95 + header.ofsSubMeshes;
    std::vector<M3SubMeshV95> rawSubmeshes(header.nSubMeshes);
    if (submeshStart + header.nSubMeshes * sizeof(M3SubMeshV95) <= size) {
        std::memcpy(rawSubmeshes.data(), ptr + submeshStart, header.nSubMeshes * sizeof(M3SubMeshV95));
    }

    // Read skins/views for vertex and index lookup
    size_t skinStart = HEADER_SIZE_V95 + header.ofsViews;
    M3SkinV95 skin = {0};
    if (header.nViews > 0 && skinStart + sizeof(M3SkinV95) <= size) {
        std::memcpy(&skin, ptr + skinStart, sizeof(M3SkinV95));
        std::cout << "[M3V95] Skin: vertexLookup=" << skin.nVertexLookup << " indexLookup=" << skin.nIndexLookup << std::endl;
    }

    // Get vertex and index lookups
    std::vector<uint32_t> vertexLookup;
    std::vector<uint32_t> indexLookup;
    
    if (skin.nVertexLookup > 0 && skin.ofsVertexLookup > 0) {
        size_t vlOffset = HEADER_SIZE_V95 + header.ofsViews + skin.sizeOfStruct + skin.ofsVertexLookup;
        if (vlOffset + skin.nVertexLookup * 4 <= size) {
            vertexLookup.resize(skin.nVertexLookup);
            std::memcpy(vertexLookup.data(), ptr + vlOffset, skin.nVertexLookup * sizeof(uint32_t));
        }
    }
    
    if (skin.nIndexLookup > 0 && skin.ofsIndexLookup > 0) {
        size_t ilOffset = HEADER_SIZE_V95 + header.ofsViews + skin.sizeOfStruct + skin.ofsIndexLookup;
        if (ilOffset + skin.nIndexLookup * 4 <= size) {
            indexLookup.resize(skin.nIndexLookup);
            std::memcpy(indexLookup.data(), ptr + ilOffset, skin.nIndexLookup * sizeof(uint32_t));
        }
    }

    // Convert vertices to M3Vertex format
    model.geometry.vertices.resize(header.nVertices);
    for (size_t i = 0; i < header.nVertices; i++) {
        size_t srcIdx = i;
        if (!vertexLookup.empty() && i < vertexLookup.size()) {
            srcIdx = vertexLookup[i];
        }
        if (srcIdx >= rawVertices.size()) srcIdx = i;
        
        const auto& src = rawVertices[srcIdx];
        auto& dst = model.geometry.vertices[i];
        
        dst.position = glm::vec3(src.x, -src.y, src.z);  // Note: Y is negated
        dst.normal = glm::vec3(
            src.normals[0] / 127.0f,
            -src.normals[1] / 127.0f,  // Y negated
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
        dst.boneWeights = glm::vec4(0.25f, 0.25f, 0.25f, 0.25f);  // Default weights
    }

    model.geometry.nrVertices = static_cast<uint32_t>(header.nVertices);
    model.geometry.vertexSize = VERTEX_SIZE_V95;

    // Build indices from submeshes
    for (size_t smIdx = 0; smIdx < rawSubmeshes.size(); smIdx++) {
        const auto& srcSm = rawSubmeshes[smIdx];
        M3Submesh sm;
        
        sm.startIndex = static_cast<uint32_t>(model.geometry.indices.size());
        sm.startVertex = srcSm.startVertex;
        sm.indexCount = srcSm.nIndices;
        sm.vertexCount = srcSm.nVertices;
        sm.materialID = srcSm.material;
        
        // Add indices for this submesh
        for (uint32_t j = 0; j < srcSm.nIndices; j++) {
            uint32_t lookupIdx = srcSm.startIndex + j;
            uint32_t index = srcSm.startVertex;
            
            if (!indexLookup.empty() && lookupIdx < indexLookup.size()) {
                index = indexLookup[lookupIdx] + srcSm.startVertex;
            } else if (lookupIdx < rawIndices.size()) {
                index = rawIndices[lookupIdx] + srcSm.startVertex;
            }
            
            model.geometry.indices.push_back(index);
        }
        
        model.geometry.submeshes.push_back(sm);
    }

    model.geometry.nrIndices = static_cast<uint32_t>(model.geometry.indices.size());
    model.geometry.nrSubmeshes = static_cast<uint32_t>(model.geometry.submeshes.size());

    // Read textures
    if (header.nTextures > 0 && header.ofsTextures > 0) {
        size_t texStart = HEADER_SIZE_V95 + header.ofsTextures;
        size_t texEnd = texStart + header.nTextures * TEXTURE_ENTRY_SIZE;
        texEnd = (texEnd + 15) & ~(size_t)15;  // Align to 16 bytes
        
        for (uint64_t i = 0; i < header.nTextures; i++) {
            size_t entryOfs = texStart + i * TEXTURE_ENTRY_SIZE;
            if (entryOfs + TEXTURE_ENTRY_SIZE > size) break;
            
            M3TextureV95 texEntry;
            std::memcpy(&texEntry, ptr + entryOfs, sizeof(M3TextureV95));
            
            M3Texture tex;
            tex.textureType = static_cast<int32_t>(i);

            // Read texture name
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

    model.success = true;
    return model;
}