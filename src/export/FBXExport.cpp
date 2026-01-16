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

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#include <stb_image_write.h>

namespace FBXExport
{
    static int64_t gUIDCounter = 1000000000;

    static int64_t GenerateUID()
    {
        return gUIDCounter++;
    }

    static std::string SanitizeFilename(const std::string& name)
    {
        std::string result;
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

    static void WriteHeader(std::ofstream& out)
    {
        out << "; FBX 7.4.0 project file\n";
        out << "; Exported by WildStar Tools\n\n";

        out << "FBXHeaderExtension:  {\n";
        out << "\tFBXHeaderVersion: 1003\n";
        out << "\tFBXVersion: 7400\n";
        out << "\tCreationTimeStamp:  {\n";
        out << "\t\tVersion: 1000\n";
        std::time_t now = std::time(nullptr);
        std::tm* tm = std::localtime(&now);
        out << "\t\tYear: " << (tm->tm_year + 1900) << "\n";
        out << "\t\tMonth: " << (tm->tm_mon + 1) << "\n";
        out << "\t\tDay: " << tm->tm_mday << "\n";
        out << "\t\tHour: " << tm->tm_hour << "\n";
        out << "\t\tMinute: " << tm->tm_min << "\n";
        out << "\t\tSecond: " << tm->tm_sec << "\n";
        out << "\t\tMillisecond: 0\n";
        out << "\t}\n";
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
        out << "\t\tP: \"OriginalUpAxis\", \"int\", \"Integer\", \"\",1\n";
        out << "\t\tP: \"OriginalUpAxisSign\", \"int\", \"Integer\", \"\",1\n";
        out << "\t\tP: \"UnitScaleFactor\", \"double\", \"Number\", \"\",1\n";
        out << "\t\tP: \"OriginalUnitScaleFactor\", \"double\", \"Number\", \"\",1\n";
        out << "\t}\n";
        out << "}\n\n";
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
    };

    struct TextureData
    {
        int64_t textureUID;
        int64_t videoUID;
        std::string name;
        std::string filename;
        std::string type;
    };

    struct MaterialData
    {
        int64_t uid;
        std::string name;
        std::vector<TextureData> textures;
    };

    static bool SaveBlendMapAsPNG(const AreaChunkRenderPtr& chunk, const std::string& outputDir, const std::string& baseName, std::string& outPath)
    {
        const auto& blendMap = chunk->getBlendMap();
        const auto& blendMapDXT = chunk->getBlendMapDXT();

        std::vector<uint8_t> rgba;
        int width = 65;
        int height = 65;

        if (!blendMap.empty())
        {
            rgba = blendMap;
        }
        else if (!blendMapDXT.empty())
        {
            if (!Tex::File::decodeDXT1(blendMapDXT.data(), width, height, rgba))
                return false;

            for (size_t i = 0; i < rgba.size(); i += 4)
            {
                int r = rgba[i];
                int g = rgba[i + 1];
                int b = rgba[i + 2];
                int sum = r + g + b;
                rgba[i + 3] = static_cast<uint8_t>(std::max(0, 255 - sum));
            }
        }
        else
        {
            return false;
        }

        std::string filename = baseName + "_blend.png";
        outPath = outputDir + "/" + filename;

        return stbi_write_png(outPath.c_str(), width, height, 4, rgba.data(), width * 4) != 0;
    }

    static bool SaveColorMapAsPNG(const AreaChunkRenderPtr& chunk, const std::string& outputDir, const std::string& baseName, std::string& outPath)
    {
        const auto& colorMap = chunk->getColorMap();
        const auto& colorMapDXT = chunk->getColorMapDXT();

        std::vector<uint8_t> rgba;
        int width = 65;
        int height = 65;

        if (!colorMap.empty())
        {
            rgba = colorMap;
        }
        else if (!colorMapDXT.empty())
        {
            if (!Tex::File::decodeDXT5(colorMapDXT.data(), width, height, rgba))
                return false;
        }
        else
        {
            return false;
        }

        std::string filename = baseName + "_color.png";
        outPath = outputDir + "/" + filename;

        return stbi_write_png(outPath.c_str(), width, height, 4, rgba.data(), width * 4) != 0;
    }

    static bool ExportLayerTexture(const ArchivePtr& archive, uint32_t layerId, const std::string& outputDir, std::string& outDiffusePath, std::string& outNormalPath)
    {
        auto& texMgr = TerrainTexture::Manager::Instance();
        const auto* entry = texMgr.GetLayerEntry(layerId);
        if (!entry) return false;

        auto exportTex = [&](const std::string& srcPath, std::string& outPath, const std::string& suffix) -> bool {
            if (srcPath.empty()) return false;

            auto fileEntry = archive->findFileCached(srcPath);
            if (!fileEntry)
            {
                std::wstring wpath(srcPath.begin(), srcPath.end());
                auto root = archive->getRoot();
                if (!root) return false;

                std::function<FileEntryPtr(const IFileSystemEntryPtr&, const std::wstring&)> findFile;
                findFile = [&](const IFileSystemEntryPtr& e, const std::wstring& path) -> FileEntryPtr {
                    if (!e) return nullptr;
                    if (!e->isDirectory())
                    {
                        std::wstring n = e->getEntryName();
                        for (auto& c : n) c = std::towlower(c);
                        std::wstring p = path;
                        for (auto& c : p) c = std::towlower(c);
                        if (n.find(p) != std::wstring::npos)
                            return std::dynamic_pointer_cast<FileEntry>(e);
                        return nullptr;
                    }
                    for (const auto& child : e->getChildren())
                    {
                        auto r = findFile(child, path);
                        if (r) return r;
                    }
                    return nullptr;
                };

                size_t lastSlash = srcPath.rfind('/');
                if (lastSlash == std::string::npos) lastSlash = srcPath.rfind('\\');
                std::string filename = (lastSlash != std::string::npos) ? srcPath.substr(lastSlash + 1) : srcPath;
                std::wstring wfn(filename.begin(), filename.end());
                fileEntry = findFile(root, wfn);
            }

            if (!fileEntry) return false;

            std::vector<uint8_t> bytes;
            if (!archive->getFileData(fileEntry, bytes)) return false;

            Tex::File tf;
            if (!tf.readFromMemory(bytes.data(), bytes.size())) return false;

            Tex::ImageRGBA img;
            if (!tf.decodeLargestMipToRGBA(img)) return false;

            std::string safeName = SanitizeFilename(srcPath);
            std::string outputFile = outputDir + "/" + safeName + suffix + ".png";

            if (stbi_write_png(outputFile.c_str(), img.width, img.height, 4, img.rgba.data(), img.width * 4))
            {
                outPath = outputFile;
                return true;
            }
            return false;
        };

        bool hasDiffuse = exportTex(entry->diffusePath, outDiffusePath, "_diffuse");
        bool hasNormal = exportTex(entry->normalPath, outNormalPath, "_normal");

        return hasDiffuse || hasNormal;
    }

    static void CollectMeshData(const AreaFilePtr& area, const ExportSettings& settings,
                                std::vector<MeshData>& meshes, int& totalVerts, int& totalTris)
    {
        const auto& chunks = area->getChunks();
        glm::vec3 worldOffset = area->getWorldOffset();

        int chunkIdx = 0;
        for (const auto& chunk : chunks)
        {
            if (!chunk || !chunk->isFullyInitialized())
            {
                chunkIdx++;
                continue;
            }

            const auto& verts = chunk->getVertices();
            if (verts.empty())
            {
                chunkIdx++;
                continue;
            }

            MeshData mesh;
            mesh.name = "Chunk_" + std::to_string(chunkIdx);
            mesh.geometryUID = GenerateUID();
            mesh.modelUID = GenerateUID();
            mesh.materialUID = GenerateUID();
            mesh.offset = worldOffset;

            for (const auto& v : verts)
            {
                float posX = (v.x + worldOffset.x) * settings.scale;
                float posY = v.y * settings.scale;
                float posZ = (v.z + worldOffset.z) * settings.scale;

                mesh.vertices.push_back(posX);
                mesh.vertices.push_back(posZ);
                mesh.vertices.push_back(posY);

                if (settings.exportNormals)
                {
                    mesh.normals.push_back(v.nx);
                    mesh.normals.push_back(v.nz);
                    mesh.normals.push_back(v.ny);
                }

                if (settings.exportUVs)
                {
                    mesh.uvs.push_back(v.u);
                    mesh.uvs.push_back(1.0f - v.v);
                }
            }

            const auto& srcIndices = AreaChunkRender::getIndices();
            if (!srcIndices.empty())
            {
                for (size_t i = 0; i < srcIndices.size(); i++)
                {
                    mesh.indices.push_back(static_cast<int>(srcIndices[i]));
                }
            }
            else
            {
                for (int y = 0; y < 16; y++)
                {
                    for (int x = 0; x < 16; x++)
                    {
                        int tl = y * 17 + x;
                        int tr = y * 17 + x + 1;
                        int bl = (y + 1) * 17 + x;
                        int br = (y + 1) * 17 + x + 1;

                        mesh.indices.push_back(tl);
                        mesh.indices.push_back(bl);
                        mesh.indices.push_back(tr);

                        mesh.indices.push_back(tr);
                        mesh.indices.push_back(bl);
                        mesh.indices.push_back(br);
                    }
                }
            }

            totalVerts += static_cast<int>(verts.size());
            totalTris += static_cast<int>(mesh.indices.size() / 3);

            meshes.push_back(std::move(mesh));
            chunkIdx++;
        }
    }

    static void WriteDefinitions(std::ofstream& out, int modelCount, int geometryCount, int materialCount, int textureCount)
    {
        out << "Definitions:  {\n";
        out << "\tVersion: 100\n";
        int total = 1 + modelCount + geometryCount + materialCount + textureCount * 2;
        out << "\tCount: " << total << "\n";

        out << "\tObjectType: \"GlobalSettings\" {\n";
        out << "\t\tCount: 1\n";
        out << "\t}\n";

        if (geometryCount > 0)
        {
            out << "\tObjectType: \"Geometry\" {\n";
            out << "\t\tCount: " << geometryCount << "\n";
            out << "\t}\n";
        }

        if (modelCount > 0)
        {
            out << "\tObjectType: \"Model\" {\n";
            out << "\t\tCount: " << modelCount << "\n";
            out << "\t}\n";
        }

        if (materialCount > 0)
        {
            out << "\tObjectType: \"Material\" {\n";
            out << "\t\tCount: " << materialCount << "\n";
            out << "\t}\n";
        }

        if (textureCount > 0)
        {
            out << "\tObjectType: \"Texture\" {\n";
            out << "\t\tCount: " << textureCount << "\n";
            out << "\t}\n";
            out << "\tObjectType: \"Video\" {\n";
            out << "\t\tCount: " << textureCount << "\n";
            out << "\t}\n";
        }

        out << "}\n\n";
    }

    static void WriteGeometry(std::ofstream& out, const MeshData& mesh)
    {
        out << "\tGeometry: " << mesh.geometryUID << ", \"Geometry::" << mesh.name << "\", \"Mesh\" {\n";

        out << "\t\tVertices: *" << mesh.vertices.size() << " {\n";
        out << "\t\t\ta: ";
        for (size_t i = 0; i < mesh.vertices.size(); i++)
        {
            if (i > 0) out << ",";
            out << std::fixed << std::setprecision(6) << mesh.vertices[i];
        }
        out << "\n\t\t}\n";

        out << "\t\tPolygonVertexIndex: *" << mesh.indices.size() << " {\n";
        out << "\t\t\ta: ";
        for (size_t i = 0; i < mesh.indices.size(); i += 3)
        {
            if (i > 0) out << ",";
            out << mesh.indices[i] << "," << mesh.indices[i + 1] << "," << -(mesh.indices[i + 2] + 1);
        }
        out << "\n\t\t}\n";

        out << "\t\tGeometryVersion: 124\n";

        out << "\t\tLayerElementNormal: 0 {\n";
        out << "\t\t\tVersion: 102\n";
        out << "\t\t\tName: \"\"\n";
        out << "\t\t\tMappingInformationType: \"ByVertice\"\n";
        out << "\t\t\tReferenceInformationType: \"Direct\"\n";
        out << "\t\t\tNormals: *" << mesh.normals.size() << " {\n";
        out << "\t\t\t\ta: ";
        for (size_t i = 0; i < mesh.normals.size(); i++)
        {
            if (i > 0) out << ",";
            out << std::fixed << std::setprecision(6) << mesh.normals[i];
        }
        out << "\n\t\t\t}\n";
        out << "\t\t}\n";

        out << "\t\tLayerElementUV: 0 {\n";
        out << "\t\t\tVersion: 101\n";
        out << "\t\t\tName: \"UVMap\"\n";
        out << "\t\t\tMappingInformationType: \"ByVertice\"\n";
        out << "\t\t\tReferenceInformationType: \"Direct\"\n";
        out << "\t\t\tUV: *" << mesh.uvs.size() << " {\n";
        out << "\t\t\t\ta: ";
        for (size_t i = 0; i < mesh.uvs.size(); i++)
        {
            if (i > 0) out << ",";
            out << std::fixed << std::setprecision(6) << mesh.uvs[i];
        }
        out << "\n\t\t\t}\n";
        out << "\t\t}\n";

        out << "\t\tLayerElementMaterial: 0 {\n";
        out << "\t\t\tVersion: 101\n";
        out << "\t\t\tName: \"\"\n";
        out << "\t\t\tMappingInformationType: \"AllSame\"\n";
        out << "\t\t\tReferenceInformationType: \"IndexToDirect\"\n";
        out << "\t\t\tMaterials: *1 {\n";
        out << "\t\t\t\ta: 0\n";
        out << "\t\t\t}\n";
        out << "\t\t}\n";

        out << "\t\tLayer: 0 {\n";
        out << "\t\t\tVersion: 100\n";
        out << "\t\t\tLayerElement:  {\n";
        out << "\t\t\t\tType: \"LayerElementNormal\"\n";
        out << "\t\t\t\tTypedIndex: 0\n";
        out << "\t\t\t}\n";
        out << "\t\t\tLayerElement:  {\n";
        out << "\t\t\t\tType: \"LayerElementUV\"\n";
        out << "\t\t\t\tTypedIndex: 0\n";
        out << "\t\t\t}\n";
        out << "\t\t\tLayerElement:  {\n";
        out << "\t\t\t\tType: \"LayerElementMaterial\"\n";
        out << "\t\t\t\tTypedIndex: 0\n";
        out << "\t\t\t}\n";
        out << "\t\t}\n";

        out << "\t}\n";
    }

    static void WriteModel(std::ofstream& out, const MeshData& mesh)
    {
        out << "\tModel: " << mesh.modelUID << ", \"Model::" << mesh.name << "\", \"Mesh\" {\n";
        out << "\t\tVersion: 232\n";
        out << "\t\tProperties70:  {\n";
        out << "\t\t\tP: \"DefaultAttributeIndex\", \"int\", \"Integer\", \"\",0\n";
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
        out << "\t\tMultiLayer: 0\n";
        out << "\t\tProperties70:  {\n";
        out << "\t\t\tP: \"DiffuseColor\", \"Color\", \"\", \"A\",0.8,0.8,0.8\n";
        out << "\t\t\tP: \"AmbientColor\", \"Color\", \"\", \"A\",0.2,0.2,0.2\n";
        out << "\t\t\tP: \"Emissive\", \"Vector3D\", \"Vector\", \"\",0,0,0\n";
        out << "\t\t\tP: \"Ambient\", \"Vector3D\", \"Vector\", \"\",0.2,0.2,0.2\n";
        out << "\t\t\tP: \"Diffuse\", \"Vector3D\", \"Vector\", \"\",0.8,0.8,0.8\n";
        out << "\t\t\tP: \"Specular\", \"Vector3D\", \"Vector\", \"\",0.2,0.2,0.2\n";
        out << "\t\t\tP: \"Shininess\", \"double\", \"Number\", \"\",20\n";
        out << "\t\t\tP: \"Opacity\", \"double\", \"Number\", \"\",1\n";
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
        out << "\t\tRelativeFilename: \"" << tex.filename << "\"\n";
        out << "\t\tModelUVTranslation: 0,0\n";
        out << "\t\tModelUVScaling: 1,1\n";
        out << "\t\tTexture_Alpha_Source: \"None\"\n";
        out << "\t\tCropping: 0,0,0,0\n";
        out << "\t}\n";
    }

    static void WriteVideo(std::ofstream& out, const TextureData& tex)
    {
        out << "\tVideo: " << tex.videoUID << ", \"Video::" << tex.name << "\", \"Clip\" {\n";
        out << "\t\tType: \"Clip\"\n";
        out << "\t\tProperties70:  {\n";
        out << "\t\t\tP: \"Path\", \"KString\", \"XRefUrl\", \"\", \"" << tex.filename << "\"\n";
        out << "\t\t}\n";
        out << "\t\tFileName: \"" << tex.filename << "\"\n";
        out << "\t\tRelativeFilename: \"" << tex.filename << "\"\n";
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
        {
            out << "\tC: \"OO\"," << tex.videoUID << "," << tex.textureUID << "\n";
        }

        out << "}\n";
    }

    ExportResult ExportAreaToFBX(const AreaFilePtr& area, const ArchivePtr& archive, const ExportSettings& settings, ProgressCallback progress)
    {
        std::vector<AreaFilePtr> areas = {area};
        return ExportAreasToFBX(areas, archive, settings, progress);
    }

    ExportResult ExportAreasToFBX(const std::vector<AreaFilePtr>& areas, const ArchivePtr& archive, const ExportSettings& settings, ProgressCallback progress)
    {
        ExportResult result;

        if (areas.empty())
        {
            result.errorMessage = "No areas to export";
            return result;
        }

        std::string outputDir = settings.outputPath;
        if (outputDir.empty())
        {
            result.errorMessage = "No output path specified";
            return result;
        }

        std::filesystem::create_directories(outputDir);

        std::string baseName = GetSuggestedFilename(areas[0]);
        if (areas.size() > 1)
            baseName = "terrain_combined";

        std::string fbxPath = outputDir + "/" + baseName + ".fbx";

        std::vector<MeshData> meshes;
        std::vector<MaterialData> materials;
        std::vector<TextureData> textures;

        int totalVerts = 0;
        int totalTris = 0;

        int totalChunks = 0;
        for (const auto& area : areas)
        {
            if (!area) continue;
            for (const auto& chunk : area->getChunks())
            {
                if (chunk && chunk->isFullyInitialized())
                    totalChunks++;
            }
        }

        int currentChunk = 0;

        if (progress) progress(0, totalChunks, "Collecting mesh data...");

        for (const auto& area : areas)
        {
            if (!area) continue;
            CollectMeshData(area, settings, meshes, totalVerts, totalTris);
        }

        if (meshes.empty())
        {
            result.errorMessage = "No valid chunks found";
            return result;
        }

        if (settings.exportTextures && archive)
        {
            if (progress) progress(currentChunk, totalChunks, "Exporting textures...");

            std::set<uint32_t> exportedLayers;
            std::string texturesDir = outputDir + "/textures";
            std::filesystem::create_directories(texturesDir);

            int chunkIdx = 0;
            for (const auto& area : areas)
            {
                if (!area) continue;
                const auto& chunks = area->getChunks();

                for (const auto& chunk : chunks)
                {
                    if (!chunk || !chunk->isFullyInitialized()) continue;

                    if (progress)
                    {
                        std::string chunkName = "Chunk " + std::to_string(chunkIdx + 1);
                        progress(currentChunk, totalChunks, "Exporting " + chunkName + "...");
                    }

                    std::string chunkBaseName = "chunk_" + std::to_string(chunkIdx);

                    std::string blendPath;
                    if (SaveBlendMapAsPNG(chunk, texturesDir, chunkBaseName, blendPath))
                    {
                        TextureData tex;
                        tex.textureUID = GenerateUID();
                        tex.videoUID = GenerateUID();
                        tex.name = chunkBaseName + "_blend";
                        tex.filename = "textures/" + chunkBaseName + "_blend.png";
                        tex.type = "blend";
                        textures.push_back(tex);
                        result.textureCount++;
                    }

                    std::string colorPath;
                    if (SaveColorMapAsPNG(chunk, texturesDir, chunkBaseName, colorPath))
                    {
                        TextureData tex;
                        tex.textureUID = GenerateUID();
                        tex.videoUID = GenerateUID();
                        tex.name = chunkBaseName + "_color";
                        tex.filename = "textures/" + chunkBaseName + "_color.png";
                        tex.type = "color";
                        textures.push_back(tex);
                        result.textureCount++;
                    }

                    const uint32_t* layerIds = chunk->getWorldLayerIDs();
                    for (int i = 0; i < 4; i++)
                    {
                        uint32_t layerId = layerIds[i];
                        if (layerId == 0 || exportedLayers.count(layerId)) continue;

                        std::string diffusePath, normalPath;
                        if (ExportLayerTexture(archive, layerId, texturesDir, diffusePath, normalPath))
                        {
                            exportedLayers.insert(layerId);

                            if (!diffusePath.empty())
                            {
                                TextureData tex;
                                tex.textureUID = GenerateUID();
                                tex.videoUID = GenerateUID();
                                tex.name = "layer_" + std::to_string(layerId) + "_diffuse";
                                std::filesystem::path p(diffusePath);
                                tex.filename = "textures/" + p.filename().string();
                                tex.type = "diffuse";
                                textures.push_back(tex);
                                result.textureCount++;
                            }

                            if (!normalPath.empty())
                            {
                                TextureData tex;
                                tex.textureUID = GenerateUID();
                                tex.videoUID = GenerateUID();
                                tex.name = "layer_" + std::to_string(layerId) + "_normal";
                                std::filesystem::path p(normalPath);
                                tex.filename = "textures/" + p.filename().string();
                                tex.type = "normal";
                                textures.push_back(tex);
                                result.textureCount++;
                            }
                        }
                    }

                    chunkIdx++;
                    currentChunk++;
                }
            }
        }

        if (progress) progress(totalChunks, totalChunks, "Writing FBX file...");

        for (auto& mesh : meshes)
        {
            MaterialData mat;
            mat.uid = mesh.materialUID;
            mat.name = mesh.name + "_Material";
            materials.push_back(mat);
        }

        std::ofstream out(fbxPath);
        if (!out.is_open())
        {
            result.errorMessage = "Failed to create output file: " + fbxPath;
            return result;
        }

        WriteHeader(out);
        WriteGlobalSettings(out);
        WriteDefinitions(out, static_cast<int>(meshes.size()), static_cast<int>(meshes.size()),
                        static_cast<int>(materials.size()), static_cast<int>(textures.size()));

        out << "Objects:  {\n";

        for (const auto& mesh : meshes)
        {
            WriteGeometry(out, mesh);
        }

        for (const auto& mesh : meshes)
        {
            WriteModel(out, mesh);
        }

        for (const auto& mat : materials)
        {
            WriteMaterial(out, mat);
        }

        for (const auto& tex : textures)
        {
            WriteTexture(out, tex);
            WriteVideo(out, tex);
        }

        out << "}\n\n";

        WriteConnections(out, meshes, materials, textures);

        out.close();

        if (progress) progress(totalChunks, totalChunks, "Export complete!");

        result.success = true;
        result.outputFile = fbxPath;
        result.vertexCount = totalVerts;
        result.triangleCount = totalTris;
        result.chunkCount = static_cast<int>(meshes.size());

        return result;
    }

    std::string GetSuggestedFilename(const AreaFilePtr& area)
    {
        if (!area) return "terrain";
        
        std::string name = "terrain_" + std::to_string(area->getTileX()) + "_" + std::to_string(area->getTileY());
        return name;
    }
}