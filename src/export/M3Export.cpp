#include "M3Export.h"
#include "../models/M3Render.h"
#include "../Archive.h"
#include "../tex/tex.h"
#include <glm/glm.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <codecvt>
#include <locale>
#include <sstream>
#include <iomanip>

namespace M3Export
{
    static uint32_t Crc32Table[256];
    static bool Crc32Init = false;

    static void InitCrc32()
    {
        if (Crc32Init) return;
        for (uint32_t i = 0; i < 256; i++)
        {
            uint32_t c = i;
            for (int j = 0; j < 8; j++)
                c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
            Crc32Table[i] = c;
        }
        Crc32Init = true;
    }

    static uint32_t Crc32(const uint8_t* data, size_t len, uint32_t crc = 0)
    {
        InitCrc32();
        crc = ~crc;
        for (size_t i = 0; i < len; i++)
            crc = Crc32Table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
        return ~crc;
    }

    static uint32_t Adler32(const uint8_t* data, size_t len)
    {
        uint32_t a = 1, b = 0;
        for (size_t i = 0; i < len; i++)
        {
            a = (a + data[i]) % 65521;
            b = (b + a) % 65521;
        }
        return (b << 16) | a;
    }

    static void WritePngU32BE(std::vector<uint8_t>& out, uint32_t v)
    {
        out.push_back((v >> 24) & 0xFF);
        out.push_back((v >> 16) & 0xFF);
        out.push_back((v >> 8) & 0xFF);
        out.push_back(v & 0xFF);
    }

    static void WritePngChunk(std::vector<uint8_t>& out, const char* type, const std::vector<uint8_t>& data)
    {
        WritePngU32BE(out, static_cast<uint32_t>(data.size()));
        size_t typeStart = out.size();
        for (int i = 0; i < 4; i++) out.push_back(type[i]);
        out.insert(out.end(), data.begin(), data.end());
        uint32_t crc = Crc32(out.data() + typeStart, 4 + data.size());
        WritePngU32BE(out, crc);
    }

    static std::vector<uint8_t> CompressDeflateStore(const uint8_t* data, size_t len)
    {
        std::vector<uint8_t> out;
        out.push_back(0x78);
        out.push_back(0x01);
        size_t pos = 0;
        while (pos < len)
        {
            size_t blockSize = std::min(len - pos, (size_t)65535);
            bool last = (pos + blockSize >= len);
            out.push_back(last ? 0x01 : 0x00);
            out.push_back(blockSize & 0xFF);
            out.push_back((blockSize >> 8) & 0xFF);
            out.push_back(~blockSize & 0xFF);
            out.push_back((~blockSize >> 8) & 0xFF);
            out.insert(out.end(), data + pos, data + pos + blockSize);
            pos += blockSize;
        }
        uint32_t adler = Adler32(data, len);
        WritePngU32BE(out, adler);
        return out;
    }

    static std::vector<uint8_t> EncodePNG(const uint8_t* rgba, int width, int height)
    {
        std::vector<uint8_t> png;
        const uint8_t sig[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
        png.insert(png.end(), sig, sig + 8);
        std::vector<uint8_t> ihdr;
        WritePngU32BE(ihdr, width);
        WritePngU32BE(ihdr, height);
        ihdr.push_back(8);
        ihdr.push_back(6);
        ihdr.push_back(0);
        ihdr.push_back(0);
        ihdr.push_back(0);
        WritePngChunk(png, "IHDR", ihdr);
        std::vector<uint8_t> rawData;
        int stride = width * 4;
        for (int y = 0; y < height; y++)
        {
            rawData.push_back(0);
            rawData.insert(rawData.end(), rgba + y * stride, rgba + y * stride + stride);
        }
        auto compressed = CompressDeflateStore(rawData.data(), rawData.size());
        WritePngChunk(png, "IDAT", compressed);
        std::vector<uint8_t> iend;
        WritePngChunk(png, "IEND", iend);
        return png;
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
        return result.empty() ? "model" : result;
    }

    static std::string ExtractModelName(const std::string& path)
    {
        if (path.empty()) return "model";
        size_t lastSlash = path.rfind('/');
        if (lastSlash == std::string::npos) lastSlash = path.rfind('\\');
        std::string filename = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
        size_t ext = filename.rfind(".m3");
        if (ext != std::string::npos) filename = filename.substr(0, ext);
        return SanitizeFilename(filename);
    }

    static std::string EscapeJsonString(const std::string& s)
    {
        std::string result;
        for (char c : s)
        {
            switch (c)
            {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c; break;
            }
        }
        return result;
    }

    static std::string FloatStr(float v)
    {
        if (std::isnan(v) || std::isinf(v)) return "0";
        std::ostringstream oss;
        oss << std::setprecision(7) << v;
        return oss.str();
    }

    static void WriteU32(std::vector<uint8_t>& buf, uint32_t v)
    {
        buf.push_back(v & 0xFF);
        buf.push_back((v >> 8) & 0xFF);
        buf.push_back((v >> 16) & 0xFF);
        buf.push_back((v >> 24) & 0xFF);
    }

    static void WriteF32(std::vector<uint8_t>& buf, float v)
    {
        uint32_t u;
        std::memcpy(&u, &v, 4);
        WriteU32(buf, u);
    }

    static void Pad(std::vector<uint8_t>& buf, size_t a)
    {
        while (buf.size() % a) buf.push_back(0);
    }

    struct BufView { size_t off, len; int target; };
    struct Acc { int view, comp, count; std::string type; glm::vec3 minV, maxV; bool hasMinMax; };

    static std::vector<uint8_t> LoadTextureAsPNG(const ArchivePtr& arc, const std::string& path)
    {
        if (!arc || path.empty()) return {};

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
        std::wstring wp = conv.from_bytes(path);
        if (wp.find(L".tex") == std::wstring::npos) wp += L".tex";

        auto entry = arc->getByPath(wp);
        if (!entry)
        {
            std::replace(wp.begin(), wp.end(), L'/', L'\\');
            entry = arc->getByPath(wp);
        }
        if (!entry) return {};

        std::vector<uint8_t> buffer;
        arc->getFileData(std::dynamic_pointer_cast<FileEntry>(entry), buffer);
        if (buffer.empty()) return {};

        Tex::File tf;
        if (!tf.readFromMemory(buffer.data(), buffer.size())) return {};

        Tex::ImageRGBA img;
        if (!tf.decodeLargestMipToRGBA(img)) return {};

        return EncodePNG(img.rgba.data(), img.width, img.height);
    }

    ExportResult ExportToFBX(M3Render* render, const ArchivePtr& archive, const ExportSettings& settings, ProgressCallback progress)
    {
        ExportResult result;
        result.errorMessage = "FBX export not implemented - use GLB";
        return result;
    }

    ExportResult ExportToGLB(M3Render* render, const ArchivePtr& archive, const ExportSettings& settings, ProgressCallback progress)
    {
        ExportResult result;
        if (!render) { result.errorMessage = "No model"; return result; }

        std::string outputDir = settings.outputPath;
        if (outputDir.empty()) { result.errorMessage = "No output path"; return result; }

        std::filesystem::create_directories(outputDir);
        std::string baseName = ExtractModelName(render->getModelName());
        std::string glbPath = outputDir + "/" + baseName + ".glb";

        if (progress) progress(0, 100, "Collecting geometry...");

        const auto& allVertices = render->getVertices();
        const auto& allIndices = render->getIndices();
        const auto& submeshes = render->getAllSubmeshes();
        const auto& materials = render->getAllMaterials();
        const auto& textures = render->getAllTextures();

        struct SubmeshExport
        {
            std::vector<glm::vec3> positions;
            std::vector<glm::vec3> normals;
            std::vector<glm::vec2> uvs;
            std::vector<uint32_t> indices;
            uint16_t materialId;
            size_t originalIndex;
            std::string name;
        };
        std::vector<SubmeshExport> exportList;

        size_t totalVerts = 0;
        size_t totalTris = 0;

        for (size_t si = 0; si < submeshes.size(); ++si)
        {
            if (!render->getSubmeshVisible(si))
                continue;

            const auto& sm = submeshes[si];

            SubmeshExport se;
            se.materialId = sm.materialID;
            se.originalIndex = si;
            se.name = "Submesh_" + std::to_string(si);

            std::unordered_map<uint32_t, uint32_t> remap;

            for (uint32_t i = 0; i < sm.indexCount; ++i)
            {
                uint32_t localIdx = allIndices[sm.startIndex + i];
                uint32_t globalIdx = localIdx + sm.startVertex;

                if (globalIdx >= allVertices.size()) continue;

                auto it = remap.find(globalIdx);
                if (it != remap.end())
                {
                    se.indices.push_back(it->second);
                }
                else
                {
                    uint32_t newIdx = static_cast<uint32_t>(se.positions.size());
                    remap[globalIdx] = newIdx;

                    const auto& v = allVertices[globalIdx];
                    se.positions.push_back(v.position);
                    se.normals.push_back(v.normal);
                    se.uvs.push_back(glm::vec2(v.uv1.x, 1.0f - v.uv1.y));
                    se.indices.push_back(newIdx);
                }
            }

            if (!se.positions.empty() && !se.indices.empty())
            {
                totalVerts += se.positions.size();
                totalTris += se.indices.size() / 3;
                exportList.push_back(std::move(se));
            }
        }

        if (exportList.empty()) { result.errorMessage = "No geometry"; return result; }

        result.vertexCount = static_cast<int>(totalVerts);
        result.triangleCount = static_cast<int>(totalTris);

        if (progress) progress(10, 100, "Loading textures...");

        struct TexData { std::vector<uint8_t> png; size_t bufOff, bufLen; std::string name; };
        std::vector<TexData> loadedTex;
        std::unordered_map<int, int> texIdxToGltfMat;

        if (settings.exportTextures && archive)
        {
            for (size_t ti = 0; ti < textures.size(); ++ti)
            {
                auto png = LoadTextureAsPNG(archive, textures[ti].path);
                if (!png.empty())
                {
                    texIdxToGltfMat[static_cast<int>(ti)] = static_cast<int>(loadedTex.size());
                    std::string texName = textures[ti].path;
                    size_t slash = texName.rfind('/');
                    if (slash == std::string::npos) slash = texName.rfind('\\');
                    if (slash != std::string::npos) texName = texName.substr(slash + 1);
                    loadedTex.push_back({std::move(png), 0, 0, texName});
                }
            }
        }
        result.textureCount = static_cast<int>(loadedTex.size());

        std::unordered_map<uint16_t, int> matIdToGltfMat;
        for (const auto& se : exportList)
        {
            if (matIdToGltfMat.count(se.materialId)) continue;

            int gltfMat = -1;
            if (se.materialId < materials.size() && !materials[se.materialId].variants.empty())
            {
                int texIdx = materials[se.materialId].variants[0].textureIndexA;
                auto it = texIdxToGltfMat.find(texIdx);
                if (it != texIdxToGltfMat.end())
                    gltfMat = it->second;
            }
            matIdToGltfMat[se.materialId] = gltfMat;
        }

        if (progress) progress(30, 100, "Building binary buffer...");

        std::vector<uint8_t> bin;
        std::vector<BufView> views;
        std::vector<Acc> accessors;

        struct MeshData { int posAcc, normAcc, uvAcc, idxAcc; int matIdx; std::string name; };
        std::vector<MeshData> meshes;

        for (const auto& se : exportList)
        {
            MeshData md;
            md.name = se.name;
            md.matIdx = matIdToGltfMat.count(se.materialId) ? matIdToGltfMat[se.materialId] : -1;

            glm::vec3 minP(FLT_MAX), maxP(-FLT_MAX);
            size_t posOff = bin.size();
            for (const auto& p : se.positions)
            {
                WriteF32(bin, p.x); WriteF32(bin, p.y); WriteF32(bin, p.z);
                minP = glm::min(minP, p);
                maxP = glm::max(maxP, p);
            }
            Pad(bin, 4);
            views.push_back({posOff, se.positions.size() * 12, 34962});
            md.posAcc = static_cast<int>(accessors.size());
            accessors.push_back({(int)views.size() - 1, 5126, (int)se.positions.size(), "VEC3", minP, maxP, true});

            size_t normOff = bin.size();
            for (const auto& n : se.normals)
            {
                WriteF32(bin, n.x); WriteF32(bin, n.y); WriteF32(bin, n.z);
            }
            Pad(bin, 4);
            views.push_back({normOff, se.normals.size() * 12, 34962});
            md.normAcc = static_cast<int>(accessors.size());
            accessors.push_back({(int)views.size() - 1, 5126, (int)se.normals.size(), "VEC3", {}, {}, false});

            size_t uvOff = bin.size();
            for (const auto& uv : se.uvs)
            {
                WriteF32(bin, uv.x); WriteF32(bin, uv.y);
            }
            Pad(bin, 4);
            views.push_back({uvOff, se.uvs.size() * 8, 34962});
            md.uvAcc = static_cast<int>(accessors.size());
            accessors.push_back({(int)views.size() - 1, 5126, (int)se.uvs.size(), "VEC2", {}, {}, false});

            size_t idxOff = bin.size();
            for (uint32_t idx : se.indices) WriteU32(bin, idx);
            Pad(bin, 4);
            views.push_back({idxOff, se.indices.size() * 4, 34963});
            md.idxAcc = static_cast<int>(accessors.size());
            accessors.push_back({(int)views.size() - 1, 5125, (int)se.indices.size(), "SCALAR", {}, {}, false});

            meshes.push_back(md);
        }

        for (auto& t : loadedTex)
        {
            t.bufOff = bin.size();
            bin.insert(bin.end(), t.png.begin(), t.png.end());
            t.bufLen = t.png.size();
            Pad(bin, 4);
        }

        if (progress) progress(60, 100, "Building JSON...");

        std::string json = "{\"asset\":{\"version\":\"2.0\",\"generator\":\"WildStar M3 Exporter\"},";
        json += "\"scene\":0,";

        json += "\"scenes\":[{\"name\":\"Scene\",\"nodes\":[0]}],";

        json += "\"nodes\":[{\"name\":\"" + EscapeJsonString(baseName) + "\",\"children\":[";
        for (size_t i = 0; i < meshes.size(); ++i)
        {
            if (i > 0) json += ",";
            json += std::to_string(i + 1);
        }
        json += "]}";
        for (size_t i = 0; i < meshes.size(); ++i)
        {
            json += ",{\"name\":\"" + EscapeJsonString(meshes[i].name) + "\",\"mesh\":" + std::to_string(i) + "}";
        }
        json += "],";

        json += "\"meshes\":[";
        for (size_t i = 0; i < meshes.size(); ++i)
        {
            const auto& m = meshes[i];
            if (i > 0) json += ",";
            json += "{\"name\":\"" + EscapeJsonString(m.name) + "\",\"primitives\":[{";
            json += "\"attributes\":{\"POSITION\":" + std::to_string(m.posAcc);
            json += ",\"NORMAL\":" + std::to_string(m.normAcc);
            json += ",\"TEXCOORD_0\":" + std::to_string(m.uvAcc) + "}";
            json += ",\"indices\":" + std::to_string(m.idxAcc);
            if (m.matIdx >= 0)
                json += ",\"material\":" + std::to_string(m.matIdx);
            json += "}]}";
        }
        json += "],";

        if (!loadedTex.empty())
        {
            json += "\"materials\":[";
            for (size_t i = 0; i < loadedTex.size(); ++i)
            {
                if (i > 0) json += ",";
                json += "{\"name\":\"" + EscapeJsonString(loadedTex[i].name) + "\",\"pbrMetallicRoughness\":{";
                json += "\"baseColorTexture\":{\"index\":" + std::to_string(i) + "},";
                json += "\"metallicFactor\":0,\"roughnessFactor\":1}}";
            }
            json += "],";

            json += "\"textures\":[";
            for (size_t i = 0; i < loadedTex.size(); ++i)
            {
                if (i > 0) json += ",";
                json += "{\"source\":" + std::to_string(i) + "}";
            }
            json += "],";

            json += "\"images\":[";
            for (size_t i = 0; i < loadedTex.size(); ++i)
            {
                if (i > 0) json += ",";
                int vi = static_cast<int>(views.size());
                views.push_back({loadedTex[i].bufOff, loadedTex[i].bufLen, 0});
                json += "{\"bufferView\":" + std::to_string(vi) + ",\"mimeType\":\"image/png\"}";
            }
            json += "],";
        }

        json += "\"accessors\":[";
        for (size_t i = 0; i < accessors.size(); ++i)
        {
            auto& a = accessors[i];
            if (i > 0) json += ",";
            json += "{\"bufferView\":" + std::to_string(a.view);
            json += ",\"componentType\":" + std::to_string(a.comp);
            json += ",\"count\":" + std::to_string(a.count);
            json += ",\"type\":\"" + a.type + "\"";
            if (a.hasMinMax)
            {
                json += ",\"min\":[" + FloatStr(a.minV.x) + "," + FloatStr(a.minV.y) + "," + FloatStr(a.minV.z) + "]";
                json += ",\"max\":[" + FloatStr(a.maxV.x) + "," + FloatStr(a.maxV.y) + "," + FloatStr(a.maxV.z) + "]";
            }
            json += "}";
        }
        json += "],";

        json += "\"bufferViews\":[";
        for (size_t i = 0; i < views.size(); ++i)
        {
            if (i > 0) json += ",";
            json += "{\"buffer\":0,\"byteOffset\":" + std::to_string(views[i].off);
            json += ",\"byteLength\":" + std::to_string(views[i].len);
            if (views[i].target) json += ",\"target\":" + std::to_string(views[i].target);
            json += "}";
        }
        json += "],";

        json += "\"buffers\":[{\"byteLength\":" + std::to_string(bin.size()) + "}]}";

        while (json.size() % 4) json += ' ';

        if (progress) progress(90, 100, "Writing file...");

        std::ofstream out(glbPath, std::ios::binary);
        if (!out) { result.errorMessage = "Can't write file"; return result; }

        uint32_t totalLen = 12 + 8 + static_cast<uint32_t>(json.size()) + 8 + static_cast<uint32_t>(bin.size());

        std::vector<uint8_t> header;
        WriteU32(header, 0x46546C67);
        WriteU32(header, 2);
        WriteU32(header, totalLen);
        out.write(reinterpret_cast<char*>(header.data()), header.size());

        std::vector<uint8_t> jc;
        WriteU32(jc, static_cast<uint32_t>(json.size()));
        WriteU32(jc, 0x4E4F534A);
        out.write(reinterpret_cast<char*>(jc.data()), jc.size());
        out.write(json.data(), json.size());

        std::vector<uint8_t> bc;
        WriteU32(bc, static_cast<uint32_t>(bin.size()));
        WriteU32(bc, 0x004E4942);
        out.write(reinterpret_cast<char*>(bc.data()), bc.size());
        out.write(reinterpret_cast<char*>(bin.data()), bin.size());

        out.close();

        if (progress) progress(100, 100, "Done!");

        result.success = true;
        result.outputFile = glbPath;
        return result;
    }

    std::string GetSuggestedFilename(M3Render* render)
    {
        if (!render) return "model";
        return ExtractModelName(render->getModelName());
    }
}