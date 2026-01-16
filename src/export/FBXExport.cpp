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

    // Convert wstring to string
    static std::string WideToNarrow(const std::wstring& wide)
    {
        std::string result;
        result.reserve(wide.size());
        for (wchar_t c : wide)
        {
            if (c < 128)
                result += static_cast<char>(c);
            else
                result += '_';
        }
        return result;
    }

    // Extract area name from path like "Map/Zone/AreaName.XXYY.area"
    static std::string ExtractAreaName(const std::wstring& path)
    {
        if (path.empty()) return "terrain";

        // Find the filename part (after last slash)
        size_t lastSlash = path.rfind(L'/');
        if (lastSlash == std::wstring::npos)
            lastSlash = path.rfind(L'\\');

        std::wstring filename = (lastSlash != std::wstring::npos) ? path.substr(lastSlash + 1) : path;

        // Remove .area extension
        size_t areaExt = filename.rfind(L".area");
        if (areaExt != std::wstring::npos)
            filename = filename.substr(0, areaExt);

        // Remove hex tile coordinates (last .XXYY part)
        // Pattern: name.XXYY where XX and YY are hex digits
        if (filename.size() >= 5)
        {
            size_t lastDot = filename.rfind(L'.');
            if (lastDot != std::wstring::npos && filename.size() - lastDot == 5)
            {
                // Check if last 4 chars are hex
                bool isHex = true;
                for (size_t i = lastDot + 1; i < filename.size() && isHex; i++)
                {
                    wchar_t c = filename[i];
                    isHex = (c >= L'0' && c <= L'9') ||
                            (c >= L'a' && c <= L'f') ||
                            (c >= L'A' && c <= L'F');
                }
                if (isHex)
                {
                    filename = filename.substr(0, lastDot);
                }
            }
        }

        std::string result = WideToNarrow(filename);
        if (result.empty())
            result = "terrain";

        return SanitizeFilename(result);
    }

    static void AppendFloat(std::string& out, float value)
    {
        char buf[32];
        int len = snprintf(buf, sizeof(buf), "%.6f", value);
        out.append(buf, len);
    }

    static void AppendInt(std::string& out, int value)
    {
        char buf[16];
        auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), value);
        out.append(buf, ptr - buf);
    }

    static void AppendInt64(std::string& out, int64_t value)
    {
        char buf[24];
        auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), value);
        out.append(buf, ptr - buf);
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
        std::string textureFilename;
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
    };

    struct LayerTextureData
    {
        std::vector<uint8_t> rgba;
        int width = 0;
        int height = 0;
        bool valid = false;
    };

    static std::wstring ToLowerW(const std::wstring& s)
    {
        std::wstring result;
        result.reserve(s.size());
        for (wchar_t c : s)
            result += static_cast<wchar_t>(std::towlower(c));
        return result;
    }

    static FileEntryPtr FindFileByPath(const IFileSystemEntryPtr& root, const std::wstring& wpath)
    {
        if (!root || wpath.empty()) return nullptr;

        std::wstring remaining = wpath;
        IFileSystemEntryPtr current = root;

        while (!remaining.empty() && current && current->isDirectory())
        {
            size_t sep = remaining.find_first_of(L"\\/");
            std::wstring component = (sep != std::wstring::npos) ? remaining.substr(0, sep) : remaining;
            remaining = (sep != std::wstring::npos) ? remaining.substr(sep + 1) : L"";

            std::wstring componentLower = ToLowerW(component);

            IFileSystemEntryPtr found = nullptr;
            for (const auto& child : current->getChildren())
            {
                if (!child) continue;
                std::wstring childLower = ToLowerW(child->getEntryName());
                if (childLower == componentLower)
                {
                    found = child;
                    break;
                }
            }

            if (!found) return nullptr;

            if (remaining.empty())
            {
                return std::dynamic_pointer_cast<FileEntry>(found);
            }

            current = found;
        }

        return nullptr;
    }

    static FileEntryPtr FindFileRecursive(const IFileSystemEntryPtr& entry, const std::wstring& targetLower)
    {
        if (!entry) return nullptr;

        if (!entry->isDirectory())
        {
            std::wstring name = entry->getEntryName();
            std::wstring nameLower = ToLowerW(name);
            if (nameLower.find(targetLower) != std::wstring::npos)
            {
                return std::dynamic_pointer_cast<FileEntry>(entry);
            }
            return nullptr;
        }

        for (const auto& child : entry->getChildren())
        {
            auto result = FindFileRecursive(child, targetLower);
            if (result) return result;
        }
        return nullptr;
    }

    static bool LoadLayerTextureData(const ArchivePtr& archive, uint32_t layerId, LayerTextureData& outData)
    {
        if (!archive || layerId == 0) return false;

        auto& texMgr = TerrainTexture::Manager::Instance();

        // Use TerrainTexture::Manager's method which shares code with runtime loading
        TerrainTexture::RawTextureData rawData;
        if (!texMgr.GetLayerTextureData(archive, layerId, rawData))
            return false;

        outData.rgba = std::move(rawData.rgba);
        outData.width = rawData.width;
        outData.height = rawData.height;
        outData.valid = rawData.valid;

        return true;
    }

    static glm::vec4 SampleTexture(const LayerTextureData& tex, float u, float v)
    {
        if (!tex.valid || tex.rgba.empty())
            return glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);

        u = u - std::floor(u);
        v = v - std::floor(v);

        float fx = u * (tex.width - 1);
        float fy = v * (tex.height - 1);

        int x0 = static_cast<int>(fx);
        int y0 = static_cast<int>(fy);
        int x1 = (x0 + 1) % tex.width;
        int y1 = (y0 + 1) % tex.height;

        float xFrac = fx - x0;
        float yFrac = fy - y0;

        auto getPixel = [&](int x, int y) -> glm::vec4 {
            int idx = (y * tex.width + x) * 4;
            return glm::vec4(
                tex.rgba[idx + 0] / 255.0f,
                tex.rgba[idx + 1] / 255.0f,
                tex.rgba[idx + 2] / 255.0f,
                tex.rgba[idx + 3] / 255.0f
            );
        };

        glm::vec4 c00 = getPixel(x0, y0);
        glm::vec4 c10 = getPixel(x1, y0);
        glm::vec4 c01 = getPixel(x0, y1);
        glm::vec4 c11 = getPixel(x1, y1);

        glm::vec4 c0 = glm::mix(c00, c10, xFrac);
        glm::vec4 c1 = glm::mix(c01, c11, xFrac);

        return glm::mix(c0, c1, yFrac);
    }

    static glm::vec4 SampleBlendMap(const std::vector<uint8_t>& blendRGBA, int blendSize, float u, float v)
    {
        float fx = u * (blendSize - 1);
        float fy = v * (blendSize - 1);

        int x0 = std::clamp(static_cast<int>(fx), 0, blendSize - 1);
        int y0 = std::clamp(static_cast<int>(fy), 0, blendSize - 1);
        int x1 = std::min(x0 + 1, blendSize - 1);
        int y1 = std::min(y0 + 1, blendSize - 1);

        float xFrac = fx - x0;
        float yFrac = fy - y0;

        auto getBlend = [&](int x, int y) -> glm::vec4 {
            int idx = (y * blendSize + x) * 4;
            return glm::vec4(
                blendRGBA[idx + 0] / 255.0f,
                blendRGBA[idx + 1] / 255.0f,
                blendRGBA[idx + 2] / 255.0f,
                blendRGBA[idx + 3] / 255.0f
            );
        };

        glm::vec4 c00 = getBlend(x0, y0);
        glm::vec4 c10 = getBlend(x1, y0);
        glm::vec4 c01 = getBlend(x0, y1);
        glm::vec4 c11 = getBlend(x1, y1);

        glm::vec4 c0 = glm::mix(c00, c10, xFrac);
        glm::vec4 c1 = glm::mix(c01, c11, xFrac);

        return glm::mix(c0, c1, yFrac);
    }

    static bool BakeChunkTexture(
        const AreaChunkRenderPtr& chunk,
        const ArchivePtr& archive,
        const std::string& outputPath,
        int textureSize = 512)
    {
        if (!chunk) return false;

        auto& texMgr = TerrainTexture::Manager::Instance();
        texMgr.LoadWorldLayerTable(archive);

        std::vector<uint8_t> blendRGBA;
        const int blendSize = 65;

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
                for (int i = 0; i < blendSize * blendSize; i++)
                {
                    blendRGBA[i * 4 + 0] = 255;
                    blendRGBA[i * 4 + 1] = 0;
                    blendRGBA[i * 4 + 2] = 0;
                    blendRGBA[i * 4 + 3] = 0;
                }
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
            blendRGBA.resize(blendSize * blendSize * 4);
            for (int i = 0; i < blendSize * blendSize; i++)
            {
                blendRGBA[i * 4 + 0] = 255;
                blendRGBA[i * 4 + 1] = 0;
                blendRGBA[i * 4 + 2] = 0;
                blendRGBA[i * 4 + 3] = 0;
            }
        }

        std::vector<uint8_t> colorRGBA;
        const auto& colorMap = chunk->getColorMap();
        const auto& colorMapDXT = chunk->getColorMapDXT();
        bool hasColorMap = false;

        if (!colorMap.empty())
        {
            colorRGBA = colorMap;
            hasColorMap = true;
        }
        else if (!colorMapDXT.empty())
        {
            if (Tex::File::decodeDXT5(colorMapDXT.data(), blendSize, blendSize, colorRGBA))
            {
                hasColorMap = true;
            }
        }

        const uint32_t* layerIds = chunk->getWorldLayerIDs();
        const float* layerScales = chunk->getLayerScales();

        LayerTextureData layerTextures[4];
        float texScales[4];

        for (int i = 0; i < 4; i++)
        {
            if (layerIds[i] != 0)
            {
                LoadLayerTextureData(archive, layerIds[i], layerTextures[i]);
            }

            float scale = layerScales[i];
            texScales[i] = (scale > 0.0f) ? (32.0f / scale) : 8.0f;
        }

        std::vector<uint8_t> outputRGBA(textureSize * textureSize * 4);

        for (int py = 0; py < textureSize; py++)
        {
            for (int px = 0; px < textureSize; px++)
            {
                float u = static_cast<float>(px) / static_cast<float>(textureSize - 1);
                float v = static_cast<float>(py) / static_cast<float>(textureSize - 1);

                glm::vec4 blend = SampleBlendMap(blendRGBA, blendSize, u, v);

                float blendSum = blend.r + blend.g + blend.b + blend.a;
                if (blendSum > 0.001f)
                    blend /= blendSum;
                else
                    blend = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);

                glm::vec4 diffuse(0.0f);
                for (int i = 0; i < 4; i++)
                {
                    float weight = blend[i];
                    if (weight < 0.001f) continue;

                    float layerU = u * texScales[i];
                    float layerV = v * texScales[i];

                    glm::vec4 layerColor;
                    if (layerTextures[i].valid)
                    {
                        layerColor = SampleTexture(layerTextures[i], layerU, layerV);
                    }
                    else
                    {
                        layerColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
                    }

                    diffuse += layerColor * weight;
                }

                if (hasColorMap)
                {
                    glm::vec4 tint = SampleBlendMap(colorRGBA, blendSize, u, v);
                    diffuse.r *= tint.r * 2.0f;
                    diffuse.g *= tint.g * 2.0f;
                    diffuse.b *= tint.b * 2.0f;
                }

                int outIdx = (py * textureSize + px) * 4;
                outputRGBA[outIdx + 0] = static_cast<uint8_t>(std::clamp(diffuse.r * 255.0f, 0.0f, 255.0f));
                outputRGBA[outIdx + 1] = static_cast<uint8_t>(std::clamp(diffuse.g * 255.0f, 0.0f, 255.0f));
                outputRGBA[outIdx + 2] = static_cast<uint8_t>(std::clamp(diffuse.b * 255.0f, 0.0f, 255.0f));
                outputRGBA[outIdx + 3] = 255;
            }
        }

        return stbi_write_png(outputPath.c_str(), textureSize, textureSize, 4, outputRGBA.data(), textureSize * 4) != 0;
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

            size_t vertCount = verts.size();
            mesh.vertices.reserve(vertCount * 3);
            if (settings.exportNormals)
                mesh.normals.reserve(vertCount * 3);
            if (settings.exportUVs)
                mesh.uvs.reserve(vertCount * 2);

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
                mesh.indices.reserve(srcIndices.size());
                for (size_t i = 0; i < srcIndices.size(); i++)
                {
                    mesh.indices.push_back(static_cast<int>(srcIndices[i]));
                }
            }
            else
            {
                mesh.indices = sharedIndices;
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
        size_t estimatedSize = mesh.vertices.size() * 20 + mesh.normals.size() * 20 +
                               mesh.uvs.size() * 20 + mesh.indices.size() * 12 + 2048;

        std::string buf;
        buf.reserve(estimatedSize);

        buf += "\tGeometry: ";
        AppendInt64(buf, mesh.geometryUID);
        buf += ", \"Geometry::";
        buf += mesh.name;
        buf += "\", \"Mesh\" {\n";

        buf += "\t\tVertices: *";
        AppendInt(buf, static_cast<int>(mesh.vertices.size()));
        buf += " {\n\t\t\ta: ";

        for (size_t i = 0; i < mesh.vertices.size(); i++)
        {
            if (i > 0) buf += ',';
            AppendFloat(buf, mesh.vertices[i]);
        }
        buf += "\n\t\t}\n";

        buf += "\t\tPolygonVertexIndex: *";
        AppendInt(buf, static_cast<int>(mesh.indices.size()));
        buf += " {\n\t\t\ta: ";

        for (size_t i = 0; i < mesh.indices.size(); i += 3)
        {
            if (i > 0) buf += ',';
            AppendInt(buf, mesh.indices[i]);
            buf += ',';
            AppendInt(buf, mesh.indices[i + 1]);
            buf += ',';
            AppendInt(buf, -(mesh.indices[i + 2] + 1));
        }
        buf += "\n\t\t}\n";

        buf += "\t\tGeometryVersion: 124\n";

        buf += "\t\tLayerElementNormal: 0 {\n";
        buf += "\t\t\tVersion: 102\n";
        buf += "\t\t\tName: \"\"\n";
        buf += "\t\t\tMappingInformationType: \"ByVertice\"\n";
        buf += "\t\t\tReferenceInformationType: \"Direct\"\n";
        buf += "\t\t\tNormals: *";
        AppendInt(buf, static_cast<int>(mesh.normals.size()));
        buf += " {\n\t\t\t\ta: ";

        for (size_t i = 0; i < mesh.normals.size(); i++)
        {
            if (i > 0) buf += ',';
            AppendFloat(buf, mesh.normals[i]);
        }
        buf += "\n\t\t\t}\n\t\t}\n";

        buf += "\t\tLayerElementUV: 0 {\n";
        buf += "\t\t\tVersion: 101\n";
        buf += "\t\t\tName: \"UVMap\"\n";
        buf += "\t\t\tMappingInformationType: \"ByVertice\"\n";
        buf += "\t\t\tReferenceInformationType: \"Direct\"\n";
        buf += "\t\t\tUV: *";
        AppendInt(buf, static_cast<int>(mesh.uvs.size()));
        buf += " {\n\t\t\t\ta: ";

        for (size_t i = 0; i < mesh.uvs.size(); i++)
        {
            if (i > 0) buf += ',';
            AppendFloat(buf, mesh.uvs[i]);
        }
        buf += "\n\t\t\t}\n\t\t}\n";

        buf += "\t\tLayerElementMaterial: 0 {\n";
        buf += "\t\t\tVersion: 101\n";
        buf += "\t\t\tName: \"\"\n";
        buf += "\t\t\tMappingInformationType: \"AllSame\"\n";
        buf += "\t\t\tReferenceInformationType: \"IndexToDirect\"\n";
        buf += "\t\t\tMaterials: *1 {\n\t\t\t\ta: 0\n\t\t\t}\n\t\t}\n";

        buf += "\t\tLayer: 0 {\n";
        buf += "\t\t\tVersion: 100\n";
        buf += "\t\t\tLayerElement:  {\n";
        buf += "\t\t\t\tType: \"LayerElementNormal\"\n";
        buf += "\t\t\t\tTypedIndex: 0\n";
        buf += "\t\t\t}\n";
        buf += "\t\t\tLayerElement:  {\n";
        buf += "\t\t\t\tType: \"LayerElementUV\"\n";
        buf += "\t\t\t\tTypedIndex: 0\n";
        buf += "\t\t\t}\n";
        buf += "\t\t\tLayerElement:  {\n";
        buf += "\t\t\t\tType: \"LayerElementMaterial\"\n";
        buf += "\t\t\t\tTypedIndex: 0\n";
        buf += "\t\t\t}\n";
        buf += "\t\t}\n";

        buf += "\t}\n";

        out.write(buf.data(), buf.size());
    }

    static void WriteModel(std::ofstream& out, const MeshData& mesh)
    {
        out << "\tModel: " << mesh.modelUID << ", \"Model::" << mesh.name << "\", \"Mesh\" {\n";
        out << "\t\tVersion: 232\n";
        out << "\t\tProperties70:  {\n";
        out << "\t\t\tP: \"ScalingMax\", \"Vector3D\", \"Vector\", \"\",0,0,0\n";
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
        out << "\t\tShadingModel: \"lambert\"\n";
        out << "\t\tMultiLayer: 0\n";
        out << "\t\tProperties70:  {\n";
        out << "\t\t\tP: \"AmbientColor\", \"Color\", \"\", \"A\",0.2,0.2,0.2\n";
        out << "\t\t\tP: \"DiffuseColor\", \"Color\", \"\", \"A\",0.8,0.8,0.8\n";
        out << "\t\t}\n";
        out << "\t}\n";
    }

    static void WriteTexture(std::ofstream& out, const TextureData& tex)
    {
        out << "\tTexture: " << tex.textureUID << ", \"Texture::" << tex.name << "\", \"\" {\n";
        out << "\t\tType: \"TextureVideoClip\"\n";
        out << "\t\tVersion: 202\n";
        out << "\t\tTextureName: \"Texture::" << tex.name << "\"\n";
        out << "\t\tProperties70:  {\n";
        out << "\t\t\tP: \"UVSet\", \"KString\", \"\", \"\", \"UVMap\"\n";
        out << "\t\t}\n";
        out << "\t\tMedia: \"Video::" << tex.name << "\"\n";
        out << "\t\tFileName: \"" << tex.filename << "\"\n";
        out << "\t\tRelativeFilename: \"" << tex.relativeFilename << "\"\n";
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

        for (size_t i = 0; i < materials.size() && i < textures.size(); i++)
        {
            out << "\tC: \"OP\"," << textures[i].textureUID << "," << materials[i].uid << ", \"DiffuseColor\"\n";
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

        std::string baseName;
        if (areas.size() == 1 && areas[0])
        {
            baseName = ExtractAreaName(areas[0]->getPath());
        }
        else if (!areas.empty() && areas[0])
        {
            baseName = ExtractAreaName(areas[0]->getPath());
            if (areas.size() > 1)
            {
                baseName += "_combined";
            }
        }
        else
        {
            baseName = "terrain";
        }

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
            std::string texturesDir = outputDir + "/textures";
            std::filesystem::create_directories(texturesDir);

            int meshIdx = 0;
            for (const auto& area : areas)
            {
                if (!area) continue;
                const auto& chunks = area->getChunks();

                for (size_t i = 0; i < chunks.size(); i++)
                {
                    const auto& chunk = chunks[i];
                    if (!chunk || !chunk->isFullyInitialized()) continue;

                    if (meshIdx >= static_cast<int>(meshes.size())) break;

                    if (progress)
                    {
                        progress(currentChunk, totalChunks, "Baking texture for chunk " + std::to_string(currentChunk + 1) + "...");
                    }

                    std::string texFilename = "chunk_" + std::to_string(meshIdx) + "_diffuse.png";
                    std::string texPath = texturesDir + "/" + texFilename;

                    if (BakeChunkTexture(chunk, archive, texPath, 512))
                    {
                        meshes[meshIdx].textureFilename = "textures/" + texFilename;
                        result.textureCount++;
                    }

                    meshIdx++;
                    currentChunk++;
                }
            }
        }

        if (progress) progress(totalChunks, totalChunks, "Writing FBX file...");

        for (size_t i = 0; i < meshes.size(); i++)
        {
            MaterialData mat;
            mat.uid = meshes[i].materialUID;
            mat.name = meshes[i].name + "_Material";

            if (!meshes[i].textureFilename.empty())
            {
                TextureData tex;
                tex.textureUID = GenerateUID();
                tex.videoUID = GenerateUID();
                tex.name = meshes[i].name + "_Diffuse";
                tex.filename = meshes[i].textureFilename;
                tex.relativeFilename = meshes[i].textureFilename;
                mat.diffuseTextureUID = tex.textureUID;
                textures.push_back(tex);
            }

            materials.push_back(mat);
        }

        std::ofstream out(fbxPath, std::ios::binary);
        if (!out.is_open())
        {
            result.errorMessage = "Failed to create output file: " + fbxPath;
            return result;
        }

        constexpr size_t BUFFER_SIZE = 1024 * 1024;
        std::vector<char> streamBuffer(BUFFER_SIZE);
        out.rdbuf()->pubsetbuf(streamBuffer.data(), BUFFER_SIZE);

        WriteHeader(out);
        WriteGlobalSettings(out);
        WriteDefinitions(out, static_cast<int>(meshes.size()), static_cast<int>(meshes.size()),
                        static_cast<int>(materials.size()), static_cast<int>(textures.size()));

        out << "Objects:  {\n";

        int meshIdx = 0;
        for (const auto& mesh : meshes)
        {
            if (progress && meshIdx % 10 == 0)
            {
                progress(totalChunks, totalChunks, "Writing geometry " + std::to_string(meshIdx + 1) + "/" + std::to_string(meshes.size()) + "...");
            }
            WriteGeometry(out, mesh);
            meshIdx++;
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
        return ExtractAreaName(area->getPath());
    }
}