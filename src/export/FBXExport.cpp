#include "FBXExport.h"
#include "../Area/AreaFile.h"
#include "../Area/TerrainTexture.h"
#include "../Archive.h"
#include "../tex/tex.h"
#include <glm/glm.hpp>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <filesystem>
#include <set>
#include <cwctype>
#include <charconv>
#include <array>
#include <algorithm>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#include <stb_image_write.h>

namespace FBXExport
{
    static int64_t gUIDCounter = 1000000000;
    static int64_t GenerateUID() { return gUIDCounter++; }

    static std::string SanitizeFilename(const std::string& name)
    {
        std::string result;
        result.reserve(name.size());
        for (char c : name)
        {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '_' || c == '-')
                result += c;
            else if (c == ' ' || c == '.' || c == '/' || c == '\\')
                result += '_';
        }
        return result;
    }

    static std::string WideToNarrow(const std::wstring& wide)
    {
        std::string result;
        result.reserve(wide.size());
        for (wchar_t c : wide)
            result += (c < 128) ? static_cast<char>(c) : '_';
        return result;
    }

    static std::string ExtractAreaName(const std::wstring& path)
    {
        if (path.empty()) return "terrain";

        size_t lastSlash = path.rfind(L'/');
        if (lastSlash == std::wstring::npos) lastSlash = path.rfind(L'\\');

        std::wstring filename = (lastSlash != std::wstring::npos) ? path.substr(lastSlash + 1) : path;

        size_t areaExt = filename.rfind(L".area");
        if (areaExt != std::wstring::npos) filename = filename.substr(0, areaExt);

        if (filename.size() >= 5)
        {
            size_t lastDot = filename.rfind(L'.');
            if (lastDot != std::wstring::npos && filename.size() - lastDot == 5)
            {
                bool isHex = true;
                for (size_t i = lastDot + 1; i < filename.size() && isHex; i++)
                {
                    wchar_t c = filename[i];
                    isHex = (c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F');
                }
                if (isHex) filename = filename.substr(0, lastDot);
            }
        }

        std::string result = WideToNarrow(filename);
        return result.empty() ? "terrain" : SanitizeFilename(result);
    }

    struct MeshData
    {
        std::vector<float> vertices;
        std::vector<float> normals;
        std::vector<float> uvs;
        std::vector<int> indices;
        std::string name;
        int64_t geometryUID;
        int64_t modelUID;
        int64_t materialUID;
        glm::vec3 offset;
        std::string diffuseFilename;
        std::string normalFilename;
    };

    struct TextureData
    {
        int64_t textureUID;
        int64_t videoUID;
        std::string name;
        std::string filename;
        std::string relativeFilename;
    };

    struct MaterialData
    {
        int64_t uid;
        std::string name;
        int64_t diffuseTextureUID = 0;
        int64_t normalTextureUID = 0;
    };

    struct CachedLayerTexture
    {
        std::vector<uint8_t> rgba;
        int width = 0;
        int height = 0;
        float scale = 4.0f;
        bool valid = false;
    };

    class ExportTextureCache
    {
    public:
        static ExportTextureCache& Instance()
        {
            static ExportTextureCache instance;
            return instance;
        }

        void Clear()
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mDiffuseCache.clear();
            mNormalCache.clear();
        }

        void PreloadLayers(const std::vector<AreaFilePtr>& areas, ProgressCallback progress)
        {
            ArchivePtr archive;
            for (const auto& area : areas)
            {
                if (area && area->getArchive()) { archive = area->getArchive(); break; }
            }
            if (!archive) return;

            std::set<uint32_t> uniqueLayerIds;
            for (const auto& area : areas)
            {
                if (!area) continue;
                for (const auto& chunk : area->getChunks())
                {
                    if (!chunk || !chunk->isFullyInitialized()) continue;
                    const uint32_t* ids = chunk->getWorldLayerIDs();
                    for (int i = 0; i < 4; i++)
                        if (ids[i] != 0) uniqueLayerIds.insert(ids[i]);
                }
            }

            auto& texMgr = TerrainTexture::Manager::Instance();
            texMgr.LoadWorldLayerTable(archive);

            int total = static_cast<int>(uniqueLayerIds.size());
            int current = 0;

            for (uint32_t layerId : uniqueLayerIds)
            {
                if (progress) progress(current, total, "Loading textures...");
                LoadLayer(archive, layerId);
                current++;
            }
        }

        const CachedLayerTexture* GetDiffuse(uint32_t layerId)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            auto it = mDiffuseCache.find(layerId);
            return (it != mDiffuseCache.end() && it->second.valid) ? &it->second : nullptr;
        }

        const CachedLayerTexture* GetNormal(uint32_t layerId)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            auto it = mNormalCache.find(layerId);
            return (it != mNormalCache.end() && it->second.valid) ? &it->second : nullptr;
        }

    private:
        void LoadLayer(const ArchivePtr& archive, uint32_t layerId)
        {
            if (layerId == 0) return;

            {
                std::lock_guard<std::mutex> lock(mMutex);
                if (mDiffuseCache.count(layerId)) return;
            }

            auto& texMgr = TerrainTexture::Manager::Instance();
            const auto* entry = texMgr.GetLayerEntry(layerId);
            if (!entry) return;

            float scale = (entry->scaleU > 0.0f) ? entry->scaleU : 4.0f;

            CachedLayerTexture diffuse, normal;
            diffuse.scale = scale;
            normal.scale = scale;

            TerrainTexture::RawTextureData rawData;
            if (!entry->diffusePath.empty() && texMgr.LoadRawTextureFromPath(archive, entry->diffusePath, rawData))
            {
                diffuse.rgba = std::move(rawData.rgba);
                diffuse.width = rawData.width;
                diffuse.height = rawData.height;
                diffuse.valid = true;
            }

            if (!entry->normalPath.empty() && texMgr.LoadRawTextureFromPath(archive, entry->normalPath, rawData))
            {
                normal.rgba = std::move(rawData.rgba);
                normal.width = rawData.width;
                normal.height = rawData.height;
                normal.valid = true;
            }

            std::lock_guard<std::mutex> lock(mMutex);
            mDiffuseCache[layerId] = std::move(diffuse);
            mNormalCache[layerId] = std::move(normal);
        }

        std::unordered_map<uint32_t, CachedLayerTexture> mDiffuseCache;
        std::unordered_map<uint32_t, CachedLayerTexture> mNormalCache;
        std::mutex mMutex;
    };

    static inline glm::vec4 SampleTexture(const CachedLayerTexture& tex, float u, float v)
    {
        if (!tex.valid || tex.rgba.empty())
            return glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);

        u = u - std::floor(u);
        v = v - std::floor(v);

        int x = static_cast<int>(u * (tex.width - 1));
        int y = static_cast<int>(v * (tex.height - 1));
        int idx = (y * tex.width + x) * 4;

        return glm::vec4(
            tex.rgba[idx] / 255.0f,
            tex.rgba[idx + 1] / 255.0f,
            tex.rgba[idx + 2] / 255.0f,
            tex.rgba[idx + 3] / 255.0f
        );
    }

    static inline glm::vec4 SampleBlendMap(const std::vector<uint8_t>& rgba, int size, float u, float v)
    {
        int x = std::clamp(static_cast<int>(u * (size - 1)), 0, size - 1);
        int y = std::clamp(static_cast<int>(v * (size - 1)), 0, size - 1);
        int idx = (y * size + x) * 4;

        return glm::vec4(
            rgba[idx] / 255.0f,
            rgba[idx + 1] / 255.0f,
            rgba[idx + 2] / 255.0f,
            rgba[idx + 3] / 255.0f
        );
    }

    static bool BakeChunkTextures(
        const AreaChunkRenderPtr& chunk,
        const ArchivePtr& archive,
        const std::string& diffusePath,
        const std::string& normalPath,
        int textureSize)
    {
        if (!chunk) return false;

        auto& cache = ExportTextureCache::Instance();
        const uint32_t* layerIds = chunk->getWorldLayerIDs();
        const float* layerScales = chunk->getLayerScales();

        std::vector<uint8_t> blendRGBA;
        constexpr int blendSize = 65;

        const auto& blendMap = chunk->getBlendMap();
        const auto& blendMapDXT = chunk->getBlendMapDXT();

        if (!blendMap.empty())
        {
            blendRGBA = blendMap;
        }
        else if (!blendMapDXT.empty())
        {
            if (!Tex::File::decodeDXT1(blendMapDXT.data(), blendSize, blendSize, blendRGBA))
            {
                blendRGBA.resize(blendSize * blendSize * 4);
                std::fill(blendRGBA.begin(), blendRGBA.end(), 0);
                for (int i = 0; i < blendSize * blendSize; i++) blendRGBA[i * 4] = 255;
            }
            else
            {
                for (size_t i = 0; i < blendRGBA.size(); i += 4)
                {
                    int sum = blendRGBA[i] + blendRGBA[i + 1] + blendRGBA[i + 2];
                    blendRGBA[i + 3] = static_cast<uint8_t>(std::max(0, 255 - sum));
                }
            }
        }
        else
        {
            blendRGBA.resize(blendSize * blendSize * 4, 0);
            for (int i = 0; i < blendSize * blendSize; i++) blendRGBA[i * 4] = 255;
        }

        std::vector<uint8_t> colorRGBA;
        bool hasColorMap = false;
        const auto& colorMap = chunk->getColorMap();
        const auto& colorMapDXT = chunk->getColorMapDXT();

        if (!colorMap.empty())
        {
            colorRGBA = colorMap;
            hasColorMap = true;
        }
        else if (!colorMapDXT.empty())
        {
            if (Tex::File::decodeDXT5(colorMapDXT.data(), blendSize, blendSize, colorRGBA))
                hasColorMap = true;
        }

        const CachedLayerTexture* diffuseTex[4];
        const CachedLayerTexture* normalTex[4];
        float texScales[4];

        for (int i = 0; i < 4; i++)
        {
            diffuseTex[i] = (layerIds[i] != 0) ? cache.GetDiffuse(layerIds[i]) : nullptr;
            normalTex[i] = (layerIds[i] != 0) ? cache.GetNormal(layerIds[i]) : nullptr;

            if (diffuseTex[i] && diffuseTex[i]->valid)
                texScales[i] = 32.0f / diffuseTex[i]->scale;
            else
                texScales[i] = (layerScales[i] > 0.0f) ? (32.0f / layerScales[i]) : 8.0f;
        }

        std::vector<uint8_t> diffuseOut(textureSize * textureSize * 4);
        std::vector<uint8_t> normalOut(textureSize * textureSize * 4);

        const float invSize = 1.0f / static_cast<float>(textureSize - 1);

        for (int py = 0; py < textureSize; py++)
        {
            float v = py * invSize;
            for (int px = 0; px < textureSize; px++)
            {
                float u = px * invSize;

                glm::vec4 blend = SampleBlendMap(blendRGBA, blendSize, u, v);
                float blendSum = blend.r + blend.g + blend.b + blend.a;
                if (blendSum > 0.001f) blend /= blendSum;
                else blend = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);

                glm::vec4 diffuse(0.0f);
                glm::vec3 normal(0.0f);

                for (int i = 0; i < 4; i++)
                {
                    float weight = blend[i];
                    if (weight < 0.001f) continue;

                    float layerU = u * texScales[i];
                    float layerV = v * texScales[i];

                    if (diffuseTex[i] && diffuseTex[i]->valid)
                        diffuse += SampleTexture(*diffuseTex[i], layerU, layerV) * weight;
                    else
                        diffuse += glm::vec4(0.5f, 0.5f, 0.5f, 1.0f) * weight;

                    if (normalTex[i] && normalTex[i]->valid)
                    {
                        glm::vec4 ns = SampleTexture(*normalTex[i], layerU, layerV);
                        normal += glm::vec3(ns.r * 2.0f - 1.0f, ns.g * 2.0f - 1.0f, ns.b * 2.0f - 1.0f) * weight;
                    }
                    else
                    {
                        normal += glm::vec3(0.0f, 0.0f, 1.0f) * weight;
                    }
                }

                if (hasColorMap)
                {
                    glm::vec4 tint = SampleBlendMap(colorRGBA, blendSize, u, v);
                    diffuse.r *= tint.r * 2.0f;
                    diffuse.g *= tint.g * 2.0f;
                    diffuse.b *= tint.b * 2.0f;
                }

                normal = glm::normalize(normal);
                normal = normal * 0.5f + 0.5f;

                int outIdx = (py * textureSize + px) * 4;
                diffuseOut[outIdx + 0] = static_cast<uint8_t>(std::clamp(diffuse.r * 255.0f, 0.0f, 255.0f));
                diffuseOut[outIdx + 1] = static_cast<uint8_t>(std::clamp(diffuse.g * 255.0f, 0.0f, 255.0f));
                diffuseOut[outIdx + 2] = static_cast<uint8_t>(std::clamp(diffuse.b * 255.0f, 0.0f, 255.0f));
                diffuseOut[outIdx + 3] = 255;

                normalOut[outIdx + 0] = static_cast<uint8_t>(std::clamp(normal.x * 255.0f, 0.0f, 255.0f));
                normalOut[outIdx + 1] = static_cast<uint8_t>(std::clamp(normal.y * 255.0f, 0.0f, 255.0f));
                normalOut[outIdx + 2] = static_cast<uint8_t>(std::clamp(normal.z * 255.0f, 0.0f, 255.0f));
                normalOut[outIdx + 3] = 255;
            }
        }

        bool diffuseOk = stbi_write_png(diffusePath.c_str(), textureSize, textureSize, 4, diffuseOut.data(), textureSize * 4) != 0;
        bool normalOk = stbi_write_png(normalPath.c_str(), textureSize, textureSize, 4, normalOut.data(), textureSize * 4) != 0;

        return diffuseOk && normalOk;
    }

    static void CollectMeshData(const AreaFilePtr& area, const ExportSettings& settings,
                                std::vector<MeshData>& meshes, int& totalVerts, int& totalTris)
    {
        const auto& chunks = area->getChunks();
        glm::vec3 worldOffset = area->getWorldOffset();

        static std::vector<int> sharedIndices;
        if (sharedIndices.empty())
        {
            sharedIndices.reserve(16 * 16 * 6);
            for (int y = 0; y < 16; y++)
            {
                for (int x = 0; x < 16; x++)
                {
                    int tl = y * 17 + x;
                    int tr = y * 17 + x + 1;
                    int bl = (y + 1) * 17 + x;
                    int br = (y + 1) * 17 + x + 1;
                    sharedIndices.push_back(tl);
                    sharedIndices.push_back(bl);
                    sharedIndices.push_back(tr);
                    sharedIndices.push_back(tr);
                    sharedIndices.push_back(bl);
                    sharedIndices.push_back(br);
                }
            }
        }

        int chunkIdx = 0;
        for (const auto& chunk : chunks)
        {
            if (!chunk || !chunk->isFullyInitialized()) { chunkIdx++; continue; }

            const auto& verts = chunk->getVertices();
            if (verts.empty()) { chunkIdx++; continue; }

            MeshData mesh;
            mesh.name = "Chunk_" + std::to_string(chunkIdx);
            mesh.geometryUID = GenerateUID();
            mesh.modelUID = GenerateUID();
            mesh.materialUID = GenerateUID();
            mesh.offset = worldOffset;

            mesh.vertices.reserve(verts.size() * 3);
            mesh.normals.reserve(verts.size() * 3);
            mesh.uvs.reserve(verts.size() * 2);

            for (const auto& v : verts)
            {
                mesh.vertices.push_back((v.x + worldOffset.x) * settings.scale);
                mesh.vertices.push_back(v.y * settings.scale);
                mesh.vertices.push_back((v.z + worldOffset.z) * settings.scale);

                mesh.normals.push_back(v.nx);
                mesh.normals.push_back(v.ny);
                mesh.normals.push_back(v.nz);

                mesh.uvs.push_back(v.u);
                mesh.uvs.push_back(1.0f - v.v);
            }

            mesh.indices = sharedIndices;
            totalVerts += static_cast<int>(verts.size());
            totalTris += static_cast<int>(sharedIndices.size()) / 3;

            meshes.push_back(std::move(mesh));
            chunkIdx++;
        }
    }

    static void WriteHeader(std::ofstream& out)
    {
        out << "; FBX 7.4.0 project file\n";
        out << "; Exported by WildStar Tools\n\n";
        out << "FBXHeaderExtension:  {\n";
        out << "\tFBXHeaderVersion: 1003\n";
        out << "\tFBXVersion: 7400\n";
        out << "\tCreator: \"WildStar Terrain Exporter\"\n";
        out << "}\n\n";
    }

    static void WriteGlobalSettings(std::ofstream& out)
    {
        out << "GlobalSettings:  {\n";
        out << "\tVersion: 1000\n";
        out << "\tProperties70:  {\n";
        out << "\t\tP: \"UpAxis\", \"int\", \"Integer\", \"\",2\n";
        out << "\t\tP: \"UpAxisSign\", \"int\", \"Integer\", \"\",1\n";
        out << "\t\tP: \"FrontAxis\", \"int\", \"Integer\", \"\",1\n";
        out << "\t\tP: \"FrontAxisSign\", \"int\", \"Integer\", \"\",1\n";
        out << "\t\tP: \"CoordAxis\", \"int\", \"Integer\", \"\",0\n";
        out << "\t\tP: \"CoordAxisSign\", \"int\", \"Integer\", \"\",1\n";
        out << "\t\tP: \"UnitScaleFactor\", \"double\", \"Number\", \"\",1\n";
        out << "\t}\n";
        out << "}\n\n";
    }

    static void WriteDefinitions(std::ofstream& out, int modelCount, int geoCount, int matCount, int texCount)
    {
        out << "Definitions:  {\n";
        out << "\tVersion: 100\n";
        out << "\tCount: " << (modelCount + geoCount + matCount + texCount * 2 + 1) << "\n";
        out << "\tObjectType: \"GlobalSettings\" { Count: 1 }\n";
        out << "\tObjectType: \"Model\" { Count: " << modelCount << " }\n";
        out << "\tObjectType: \"Geometry\" { Count: " << geoCount << " }\n";
        out << "\tObjectType: \"Material\" { Count: " << matCount << " }\n";
        out << "\tObjectType: \"Texture\" { Count: " << texCount << " }\n";
        out << "\tObjectType: \"Video\" { Count: " << texCount << " }\n";
        out << "}\n\n";
    }

    static void WriteGeometry(std::ofstream& out, const MeshData& mesh)
    {
        out << "\tGeometry: " << mesh.geometryUID << ", \"Geometry::" << mesh.name << "\", \"Mesh\" {\n";

        out << "\t\tVertices: *" << mesh.vertices.size() << " {\n\t\t\ta: ";
        for (size_t i = 0; i < mesh.vertices.size(); i++)
        {
            if (i > 0) out << ",";
            out << mesh.vertices[i];
        }
        out << "\n\t\t}\n";

        out << "\t\tPolygonVertexIndex: *" << mesh.indices.size() << " {\n\t\t\ta: ";
        for (size_t i = 0; i < mesh.indices.size(); i += 3)
        {
            if (i > 0) out << ",";
            out << mesh.indices[i] << "," << mesh.indices[i + 1] << "," << (-(mesh.indices[i + 2] + 1));
        }
        out << "\n\t\t}\n";

        out << "\t\tLayerElementNormal: 0 {\n";
        out << "\t\t\tVersion: 101\n";
        out << "\t\t\tName: \"\"\n";
        out << "\t\t\tMappingInformationType: \"ByVertice\"\n";
        out << "\t\t\tReferenceInformationType: \"Direct\"\n";
        out << "\t\t\tNormals: *" << mesh.normals.size() << " {\n\t\t\t\ta: ";
        for (size_t i = 0; i < mesh.normals.size(); i++)
        {
            if (i > 0) out << ",";
            out << mesh.normals[i];
        }
        out << "\n\t\t\t}\n\t\t}\n";

        out << "\t\tLayerElementUV: 0 {\n";
        out << "\t\t\tVersion: 101\n";
        out << "\t\t\tName: \"UVMap\"\n";
        out << "\t\t\tMappingInformationType: \"ByVertice\"\n";
        out << "\t\t\tReferenceInformationType: \"Direct\"\n";
        out << "\t\t\tUV: *" << mesh.uvs.size() << " {\n\t\t\t\ta: ";
        for (size_t i = 0; i < mesh.uvs.size(); i++)
        {
            if (i > 0) out << ",";
            out << mesh.uvs[i];
        }
        out << "\n\t\t\t}\n\t\t}\n";

        out << "\t\tLayerElementMaterial: 0 {\n";
        out << "\t\t\tVersion: 101\n";
        out << "\t\t\tName: \"\"\n";
        out << "\t\t\tMappingInformationType: \"AllSame\"\n";
        out << "\t\t\tReferenceInformationType: \"IndexToDirect\"\n";
        out << "\t\t\tMaterials: *1 { a: 0 }\n";
        out << "\t\t}\n";

        out << "\t\tLayer: 0 {\n";
        out << "\t\t\tVersion: 100\n";
        out << "\t\t\tLayerElement: { Type: \"LayerElementNormal\", TypedIndex: 0 }\n";
        out << "\t\t\tLayerElement: { Type: \"LayerElementUV\", TypedIndex: 0 }\n";
        out << "\t\t\tLayerElement: { Type: \"LayerElementMaterial\", TypedIndex: 0 }\n";
        out << "\t\t}\n";

        out << "\t}\n";
    }

    static void WriteModel(std::ofstream& out, const MeshData& mesh)
    {
        out << "\tModel: " << mesh.modelUID << ", \"Model::" << mesh.name << "\", \"Mesh\" {\n";
        out << "\t\tVersion: 232\n";
        out << "\t\tProperties70:  {\n";
        out << "\t\t\tP: \"ScalingMax\", \"Vector3D\", \"Vector\", \"\",0,0,0\n";
        out << "\t\t}\n";
        out << "\t\tShading: T\n";
        out << "\t\tCulling: \"CullingOff\"\n";
        out << "\t}\n";
    }

    static void WriteMaterial(std::ofstream& out, const MaterialData& mat)
    {
        out << "\tMaterial: " << mat.uid << ", \"Material::" << mat.name << "\", \"\" {\n";
        out << "\t\tVersion: 102\n";
        out << "\t\tShadingModel: \"phong\"\n";
        out << "\t\tProperties70:  {\n";
        out << "\t\t\tP: \"DiffuseColor\", \"Color\", \"\", \"A\",1,1,1\n";
        out << "\t\t}\n";
        out << "\t}\n";
    }

    static void WriteTexture(std::ofstream& out, const TextureData& tex)
    {
        out << "\tTexture: " << tex.textureUID << ", \"Texture::" << tex.name << "\", \"\" {\n";
        out << "\t\tType: \"TextureVideoClip\"\n";
        out << "\t\tVersion: 202\n";
        out << "\t\tTextureName: \"Texture::" << tex.name << "\"\n";
        out << "\t\tMedia: \"Video::" << tex.name << "\"\n";
        out << "\t\tFileName: \"" << tex.filename << "\"\n";
        out << "\t\tRelativeFilename: \"" << tex.relativeFilename << "\"\n";
        out << "\t}\n";
    }

    static void WriteVideo(std::ofstream& out, const TextureData& tex)
    {
        out << "\tVideo: " << tex.videoUID << ", \"Video::" << tex.name << "\", \"Clip\" {\n";
        out << "\t\tType: \"Clip\"\n";
        out << "\t\tFileName: \"" << tex.filename << "\"\n";
        out << "\t\tRelativeFilename: \"" << tex.relativeFilename << "\"\n";
        out << "\t}\n";
    }

    static void WriteConnections(std::ofstream& out, const std::vector<MeshData>& meshes,
                                  const std::vector<MaterialData>& materials,
                                  const std::vector<TextureData>& textures)
    {
        out << "Connections:  {\n";

        for (const auto& mesh : meshes)
        {
            out << "\tC: \"OO\"," << mesh.modelUID << ",0\n";
            out << "\tC: \"OO\"," << mesh.geometryUID << "," << mesh.modelUID << "\n";
            out << "\tC: \"OO\"," << mesh.materialUID << "," << mesh.modelUID << "\n";
        }

        for (size_t i = 0; i < materials.size(); i++)
        {
            if (materials[i].diffuseTextureUID != 0)
                out << "\tC: \"OP\"," << materials[i].diffuseTextureUID << "," << materials[i].uid << ", \"DiffuseColor\"\n";
            if (materials[i].normalTextureUID != 0)
                out << "\tC: \"OP\"," << materials[i].normalTextureUID << "," << materials[i].uid << ", \"NormalMap\"\n";
        }

        for (const auto& tex : textures)
            out << "\tC: \"OO\"," << tex.videoUID << "," << tex.textureUID << "\n";

        out << "}\n";
    }

    ExportResult ExportAreaToFBX(const AreaFilePtr& area, const ArchivePtr& archive, const ExportSettings& settings, ProgressCallback progress)
    {
        return ExportAreasToFBX({area}, archive, settings, progress);
    }

    ExportResult ExportAreasToFBX(const std::vector<AreaFilePtr>& areas, const ArchivePtr& archive, const ExportSettings& settings, ProgressCallback progress)
    {
        ExportResult result;

        if (areas.empty()) { result.errorMessage = "No areas to export"; return result; }

        std::string outputDir = settings.outputPath;
        if (outputDir.empty()) { result.errorMessage = "No output path specified"; return result; }

        std::filesystem::create_directories(outputDir);

        std::string baseName = (!areas.empty() && areas[0]) ? ExtractAreaName(areas[0]->getPath()) : "terrain";
        if (areas.size() > 1) baseName += "_combined";

        std::string fbxPath = outputDir + "/" + baseName + ".fbx";

        std::vector<MeshData> meshes;
        std::vector<MaterialData> materials;
        std::vector<TextureData> textures;

        int totalVerts = 0, totalTris = 0;

        if (settings.exportTextures)
        {
            auto& cache = ExportTextureCache::Instance();
            cache.Clear();
            if (progress) progress(0, 1, "Loading textures...");
            cache.PreloadLayers(areas, progress);
        }

        if (progress) progress(0, 1, "Collecting mesh data...");
        for (const auto& area : areas)
        {
            if (area) CollectMeshData(area, settings, meshes, totalVerts, totalTris);
        }

        if (meshes.empty()) { result.errorMessage = "No valid chunks found"; return result; }

        if (settings.exportTextures)
        {
            std::string texturesDir = outputDir + "/textures";
            std::filesystem::create_directories(texturesDir);

            int meshIdx = 0;
            int totalChunks = static_cast<int>(meshes.size());

            for (const auto& area : areas)
            {
                if (!area) continue;
                const auto& chunks = area->getChunks();

                for (const auto& chunk : chunks)
                {
                    if (!chunk || !chunk->isFullyInitialized()) continue;
                    if (meshIdx >= static_cast<int>(meshes.size())) break;

                    if (progress) progress(meshIdx, totalChunks, "Baking textures...");

                    std::string diffuseFile = "chunk_" + std::to_string(meshIdx) + "_diffuse.png";
                    std::string normalFile = "chunk_" + std::to_string(meshIdx) + "_normal.png";

                    if (BakeChunkTextures(chunk, area->getArchive(),
                                          texturesDir + "/" + diffuseFile,
                                          texturesDir + "/" + normalFile, 512))
                    {
                        meshes[meshIdx].diffuseFilename = "textures/" + diffuseFile;
                        meshes[meshIdx].normalFilename = "textures/" + normalFile;
                        result.textureCount += 2;
                    }
                    meshIdx++;
                }
            }
        }

        if (progress) progress(0, 1, "Writing FBX...");

        for (size_t i = 0; i < meshes.size(); i++)
        {
            MaterialData mat;
            mat.uid = meshes[i].materialUID;
            mat.name = meshes[i].name + "_Material";

            if (!meshes[i].diffuseFilename.empty())
            {
                TextureData tex;
                tex.textureUID = GenerateUID();
                tex.videoUID = GenerateUID();
                tex.name = meshes[i].name + "_Diffuse";
                tex.filename = tex.relativeFilename = meshes[i].diffuseFilename;
                mat.diffuseTextureUID = tex.textureUID;
                textures.push_back(tex);
            }

            if (!meshes[i].normalFilename.empty())
            {
                TextureData tex;
                tex.textureUID = GenerateUID();
                tex.videoUID = GenerateUID();
                tex.name = meshes[i].name + "_Normal";
                tex.filename = tex.relativeFilename = meshes[i].normalFilename;
                mat.normalTextureUID = tex.textureUID;
                textures.push_back(tex);
            }

            materials.push_back(mat);
        }

        std::ofstream out(fbxPath, std::ios::binary);
        if (!out.is_open()) { result.errorMessage = "Failed to create: " + fbxPath; return result; }

        std::vector<char> buffer(1024 * 1024);
        out.rdbuf()->pubsetbuf(buffer.data(), buffer.size());

        WriteHeader(out);
        WriteGlobalSettings(out);
        WriteDefinitions(out, static_cast<int>(meshes.size()), static_cast<int>(meshes.size()),
                        static_cast<int>(materials.size()), static_cast<int>(textures.size()));

        out << "Objects:  {\n";
        for (const auto& mesh : meshes) WriteGeometry(out, mesh);
        for (const auto& mesh : meshes) WriteModel(out, mesh);
        for (const auto& mat : materials) WriteMaterial(out, mat);
        for (const auto& tex : textures) { WriteTexture(out, tex); WriteVideo(out, tex); }
        out << "}\n\n";

        WriteConnections(out, meshes, materials, textures);
        out.close();

        ExportTextureCache::Instance().Clear();

        result.success = true;
        result.outputFile = fbxPath;
        result.vertexCount = totalVerts;
        result.triangleCount = totalTris;
        result.chunkCount = static_cast<int>(meshes.size());

        return result;
    }

    std::string GetSuggestedFilename(const AreaFilePtr& area)
    {
        return area ? ExtractAreaName(area->getPath()) : "terrain";
    }
}