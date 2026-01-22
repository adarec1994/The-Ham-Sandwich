#include "FBXExport.h"
#include "../Area/AreaFile.h"
#include "../Area/TerrainTexture.h"
#include "../Archive.h"
#include "../tex/tex.h"
#include <glm/glm.hpp>
#include <fstream>
#include <filesystem>
#include <set>
#include <unordered_map>
#include <cwctype>
#include <charconv>
#include <algorithm>
#include <mutex>

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#include <stb_image_write.h>

static inline glm::vec3 ToGlm(const DirectX::XMFLOAT3& v)
{
    return glm::vec3(v.x, v.y, v.z);
}

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
        uint32_t layerIds[4] = {0, 0, 0, 0};
        float layerScales[4] = {4.0f, 4.0f, 4.0f, 4.0f};
        std::string blendMapFile;
        std::string colorMapFile;
        bool hasColorMap = false;
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
        std::vector<int64_t> textureUIDs;
    };

    struct LayerTextureInfo
    {
        std::string diffuseFile;
        std::string normalFile;
        float scale = 4.0f;
        bool exported = false;
    };

    static bool ExportBlendMap(const AreaChunkRenderPtr& chunk, const std::string& path)
    {
        constexpr int blendSize = 65;
        std::vector<uint8_t> blendRGBA;

        const auto& blendMap = chunk->getBlendMap();
        const auto& blendMapDXT = chunk->getBlendMapDXT();

        if (!blendMap.empty())
        {
            blendRGBA = blendMap;
        }
        else if (!blendMapDXT.empty())
        {
            if (!Tex::File::decodeDXT1(blendMapDXT.data(), blendSize, blendSize, blendRGBA))
                return false;
            for (size_t i = 0; i < blendRGBA.size(); i += 4)
            {
                int sum = blendRGBA[i] + blendRGBA[i + 1] + blendRGBA[i + 2];
                blendRGBA[i + 3] = static_cast<uint8_t>(std::max(0, 255 - sum));
            }
        }
        else
        {
            blendRGBA.resize(blendSize * blendSize * 4, 0);
            for (int i = 0; i < blendSize * blendSize; i++) blendRGBA[i * 4] = 255;
        }

        return stbi_write_png(path.c_str(), blendSize, blendSize, 4, blendRGBA.data(), blendSize * 4) != 0;
    }

    static bool ExportColorMap(const AreaChunkRenderPtr& chunk, const std::string& path)
    {
        constexpr int mapSize = 65;
        std::vector<uint8_t> colorRGBA;

        const auto& colorMap = chunk->getColorMap();
        const auto& colorMapDXT = chunk->getColorMapDXT();

        if (!colorMap.empty())
        {
            colorRGBA = colorMap;
        }
        else if (!colorMapDXT.empty())
        {
            if (!Tex::File::decodeDXT5(colorMapDXT.data(), mapSize, mapSize, colorRGBA))
                return false;
        }
        else
        {
            return false;
        }

        return stbi_write_png(path.c_str(), mapSize, mapSize, 4, colorRGBA.data(), mapSize * 4) != 0;
    }

    static bool ExportLayerTexture(const ArchivePtr& archive, uint32_t layerId,
                                    const std::string& diffusePath, const std::string& normalPath,
                                    float& outScale)
    {
        auto& texMgr = TerrainTexture::Manager::Instance();
        texMgr.LoadWorldLayerTable(archive);

        const auto* entry = texMgr.GetLayerEntry(layerId);
        if (!entry) return false;

        outScale = (entry->scaleU > 0.0f) ? entry->scaleU : 4.0f;

        bool success = false;
        TerrainTexture::RawTextureData rawData;

        if (!entry->diffusePath.empty() && texMgr.LoadRawTextureFromPath(archive, entry->diffusePath, rawData))
        {
            stbi_write_png(diffusePath.c_str(), rawData.width, rawData.height, 4, rawData.rgba.data(), rawData.width * 4);
            success = true;
        }

        if (!entry->normalPath.empty() && texMgr.LoadRawTextureFromPath(archive, entry->normalPath, rawData))
        {
            stbi_write_png(normalPath.c_str(), rawData.width, rawData.height, 4, rawData.rgba.data(), rawData.width * 4);
        }

        return success;
    }

    static void CollectMeshData(const AreaFilePtr& area, const ExportSettings& settings,
                                std::vector<MeshData>& meshes, int& totalVerts, int& totalTris)
    {
        const auto& chunks = area->getChunks();
        glm::vec3 worldOffset = ToGlm(area->getWorldOffset());

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

            const uint32_t* ids = chunk->getWorldLayerIDs();
            const float* scales = chunk->getLayerScales();
            for (int i = 0; i < 4; i++)
            {
                mesh.layerIds[i] = ids[i];
                mesh.layerScales[i] = (scales[i] > 0.0f) ? scales[i] : 4.0f;
            }

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
        out << "; WildStar Terrain - Layer-based export for Blender\n";
        out << "; Material names encode: Chunk_N_L{layer0}_{layer1}_{layer2}_{layer3}\n";
        out << "; Scales in material properties, blend/color maps per chunk\n\n";
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

    static void WriteMaterial(std::ofstream& out, const MaterialData& mat, const MeshData& mesh)
    {
        out << "\tMaterial: " << mat.uid << ", \"Material::" << mat.name << "\", \"\" {\n";
        out << "\t\tVersion: 102\n";
        out << "\t\tShadingModel: \"phong\"\n";
        out << "\t\tProperties70:  {\n";
        out << "\t\t\tP: \"DiffuseColor\", \"Color\", \"\", \"A\",1,1,1\n";
        out << "\t\t\tP: \"Layer0Scale\", \"double\", \"Number\", \"\"," << mesh.layerScales[0] << "\n";
        out << "\t\t\tP: \"Layer1Scale\", \"double\", \"Number\", \"\"," << mesh.layerScales[1] << "\n";
        out << "\t\t\tP: \"Layer2Scale\", \"double\", \"Number\", \"\"," << mesh.layerScales[2] << "\n";
        out << "\t\t\tP: \"Layer3Scale\", \"double\", \"Number\", \"\"," << mesh.layerScales[3] << "\n";
        out << "\t\t\tP: \"Layer0ID\", \"int\", \"Integer\", \"\"," << mesh.layerIds[0] << "\n";
        out << "\t\t\tP: \"Layer1ID\", \"int\", \"Integer\", \"\"," << mesh.layerIds[1] << "\n";
        out << "\t\t\tP: \"Layer2ID\", \"int\", \"Integer\", \"\"," << mesh.layerIds[2] << "\n";
        out << "\t\t\tP: \"Layer3ID\", \"int\", \"Integer\", \"\"," << mesh.layerIds[3] << "\n";
        out << "\t\t\tP: \"HasColorMap\", \"bool\", \"Bool\", \"\"," << (mesh.hasColorMap ? 1 : 0) << "\n";
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

        for (const auto& tex : textures)
            out << "\tC: \"OO\"," << tex.videoUID << "," << tex.textureUID << "\n";

        out << "}\n";
    }

    static void WriteLayerInfo(const std::string& path,
                               const std::unordered_map<uint32_t, LayerTextureInfo>& layers)
    {
        std::ofstream out(path);
        out << "# WildStar Terrain Layer Info\n";
        out << "# LayerID,DiffuseFile,NormalFile,Scale\n";
        for (const auto& [id, info] : layers)
        {
            if (info.exported)
            {
                out << id << "," << info.diffuseFile << "," << info.normalFile << "," << info.scale << "\n";
            }
        }
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
        std::string texturesDir = outputDir + "/textures";
        std::filesystem::create_directories(texturesDir);

        std::string baseName = (!areas.empty() && areas[0]) ? ExtractAreaName(areas[0]->getPath()) : "terrain";
        if (areas.size() > 1) baseName += "_combined";

        std::string fbxPath = outputDir + "/" + baseName + ".fbx";

        std::vector<MeshData> meshes;
        std::vector<MaterialData> materials;
        std::vector<TextureData> textures;
        std::unordered_map<uint32_t, LayerTextureInfo> layerTextures;

        int totalVerts = 0, totalTris = 0;

        if (progress) progress(0, 1, "Collecting mesh data...");
        for (const auto& area : areas)
        {
            if (area) CollectMeshData(area, settings, meshes, totalVerts, totalTris);
        }

        if (meshes.empty()) { result.errorMessage = "No valid chunks found"; return result; }

        std::set<uint32_t> uniqueLayerIds;
        for (const auto& mesh : meshes)
        {
            for (int i = 0; i < 4; i++)
                if (mesh.layerIds[i] != 0) uniqueLayerIds.insert(mesh.layerIds[i]);
        }

        if (settings.exportTextures)
        {
            ArchivePtr effectiveArchive;
            for (const auto& area : areas)
            {
                if (area && area->getArchive()) { effectiveArchive = area->getArchive(); break; }
            }

            if (effectiveArchive)
            {
                int total = static_cast<int>(uniqueLayerIds.size());
                int current = 0;

                for (uint32_t layerId : uniqueLayerIds)
                {
                    if (progress) progress(current, total, "Exporting layer textures...");

                    std::string diffuseFile = "layer_" + std::to_string(layerId) + "_diffuse.png";
                    std::string normalFile = "layer_" + std::to_string(layerId) + "_normal.png";

                    LayerTextureInfo info;
                    info.diffuseFile = diffuseFile;
                    info.normalFile = normalFile;

                    if (ExportLayerTexture(effectiveArchive, layerId,
                                           texturesDir + "/" + diffuseFile,
                                           texturesDir + "/" + normalFile,
                                           info.scale))
                    {
                        info.exported = true;
                        result.textureCount++;

                        TextureData tex;
                        tex.textureUID = GenerateUID();
                        tex.videoUID = GenerateUID();
                        tex.name = "Layer_" + std::to_string(layerId) + "_Diffuse";
                        tex.filename = tex.relativeFilename = "textures/" + diffuseFile;
                        textures.push_back(tex);

                        tex.textureUID = GenerateUID();
                        tex.videoUID = GenerateUID();
                        tex.name = "Layer_" + std::to_string(layerId) + "_Normal";
                        tex.filename = tex.relativeFilename = "textures/" + normalFile;
                        textures.push_back(tex);
                    }

                    layerTextures[layerId] = info;
                    current++;
                }

                int meshIdx = 0;
                int totalMeshes = static_cast<int>(meshes.size());

                for (const auto& area : areas)
                {
                    if (!area) continue;
                    const auto& chunks = area->getChunks();

                    for (const auto& chunk : chunks)
                    {
                        if (!chunk || !chunk->isFullyInitialized()) continue;
                        if (meshIdx >= static_cast<int>(meshes.size())) break;

                        if (progress) progress(meshIdx, totalMeshes, "Exporting blend maps...");

                        std::string blendFile = "chunk_" + std::to_string(meshIdx) + "_blend.png";
                        if (ExportBlendMap(chunk, texturesDir + "/" + blendFile))
                        {
                            meshes[meshIdx].blendMapFile = "textures/" + blendFile;

                            TextureData tex;
                            tex.textureUID = GenerateUID();
                            tex.videoUID = GenerateUID();
                            tex.name = "Chunk_" + std::to_string(meshIdx) + "_Blend";
                            tex.filename = tex.relativeFilename = "textures/" + blendFile;
                            textures.push_back(tex);
                        }

                        std::string colorFile = "chunk_" + std::to_string(meshIdx) + "_color.png";
                        if (ExportColorMap(chunk, texturesDir + "/" + colorFile))
                        {
                            meshes[meshIdx].colorMapFile = "textures/" + colorFile;
                            meshes[meshIdx].hasColorMap = true;

                            TextureData tex;
                            tex.textureUID = GenerateUID();
                            tex.videoUID = GenerateUID();
                            tex.name = "Chunk_" + std::to_string(meshIdx) + "_Color";
                            tex.filename = tex.relativeFilename = "textures/" + colorFile;
                            textures.push_back(tex);
                        }

                        meshIdx++;
                    }
                }

                WriteLayerInfo(outputDir + "/layers.csv", layerTextures);
            }
        }

        if (progress) progress(0, 1, "Writing FBX...");

        for (size_t i = 0; i < meshes.size(); i++)
        {
            MaterialData mat;
            mat.uid = meshes[i].materialUID;
            mat.name = meshes[i].name + "_L" +
                       std::to_string(meshes[i].layerIds[0]) + "_" +
                       std::to_string(meshes[i].layerIds[1]) + "_" +
                       std::to_string(meshes[i].layerIds[2]) + "_" +
                       std::to_string(meshes[i].layerIds[3]);
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
        for (size_t i = 0; i < materials.size(); i++) WriteMaterial(out, materials[i], meshes[i]);
        for (const auto& tex : textures) { WriteTexture(out, tex); WriteVideo(out, tex); }
        out << "}\n\n";

        WriteConnections(out, meshes, materials, textures);
        out.close();

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