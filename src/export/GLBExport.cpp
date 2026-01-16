#include "GLBExport.h"
#include "../Area/AreaFile.h"
#include "../Area/TerrainTexture.h"
#include "../Archive.h"
#include "../tex/tex.h"
#include <glm/glm.hpp>
#include <fstream>
#include <filesystem>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <cstring>

namespace GLBExport
{
    static std::string WideToNarrow(const std::wstring& wide)
    {
        std::string result;
        for (wchar_t c : wide) result += (c < 128) ? static_cast<char>(c) : '_';
        return result;
    }

    static std::string SanitizeName(const std::string& name)
    {
        std::string result;
        for (char c : name)
            result += ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') ? c : '_';
        return result;
    }

    static std::string ExtractAreaName(const std::wstring& path)
    {
        if (path.empty()) return "terrain";
        size_t slash = path.rfind(L'/');
        if (slash == std::wstring::npos) slash = path.rfind(L'\\');
        std::wstring name = (slash != std::wstring::npos) ? path.substr(slash + 1) : path;
        size_t ext = name.rfind(L".area");
        if (ext != std::wstring::npos) name = name.substr(0, ext);
        size_t dot = name.rfind(L'.');
        if (dot != std::wstring::npos && name.size() - dot == 5) name = name.substr(0, dot);
        return SanitizeName(WideToNarrow(name));
    }

    struct CachedTex
    {
        std::vector<uint8_t> diffuse, normal;
        int w = 0, h = 0;
        float scale = 4.0f;
        bool valid = false;
    };

    static std::unordered_map<uint32_t, CachedTex> gTexCache;

    static void PreloadTextures(const std::vector<AreaFilePtr>& areas, ProgressCallback progress)
    {
        gTexCache.clear();

        ArchivePtr archive;
        for (const auto& a : areas) if (a && a->getArchive()) { archive = a->getArchive(); break; }
        if (!archive) return;

        std::set<uint32_t> ids;
        for (const auto& a : areas)
        {
            if (!a) continue;
            for (const auto& c : a->getChunks())
            {
                if (!c || !c->isFullyInitialized()) continue;
                const uint32_t* l = c->getWorldLayerIDs();
                for (int i = 0; i < 4; i++) if (l[i]) ids.insert(l[i]);
            }
        }

        auto& mgr = TerrainTexture::Manager::Instance();
        mgr.LoadWorldLayerTable(archive);

        int total = static_cast<int>(ids.size()), cur = 0;
        for (uint32_t id : ids)
        {
            if (progress) progress(cur++, total, "Loading textures...");
            const auto* e = mgr.GetLayerEntry(id);
            if (!e) continue;

            CachedTex t;
            t.scale = (e->scaleU > 0) ? e->scaleU : 4.0f;
            TerrainTexture::RawTextureData raw;

            if (!e->diffusePath.empty() && mgr.LoadRawTextureFromPath(archive, e->diffusePath, raw))
            {
                t.diffuse = std::move(raw.rgba);
                t.w = raw.width; t.h = raw.height;
                t.valid = true;
            }
            if (!e->normalPath.empty() && mgr.LoadRawTextureFromPath(archive, e->normalPath, raw))
                t.normal = std::move(raw.rgba);

            gTexCache[id] = std::move(t);
        }
    }

    static glm::vec4 Sample(const std::vector<uint8_t>& rgba, int w, int h, float u, float v)
    {
        if (rgba.empty()) return glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        u -= std::floor(u); v -= std::floor(v);
        int x = static_cast<int>(u * (w - 1)), y = static_cast<int>(v * (h - 1));
        int i = (y * w + x) * 4;
        return glm::vec4(rgba[i] / 255.0f, rgba[i+1] / 255.0f, rgba[i+2] / 255.0f, rgba[i+3] / 255.0f);
    }

    static glm::vec4 SampleBlend(const std::vector<uint8_t>& rgba, int sz, float u, float v)
    {
        int x = std::clamp(int(u * (sz-1)), 0, sz-1), y = std::clamp(int(v * (sz-1)), 0, sz-1);
        int i = (y * sz + x) * 4;
        return glm::vec4(rgba[i] / 255.0f, rgba[i+1] / 255.0f, rgba[i+2] / 255.0f, rgba[i+3] / 255.0f);
    }

    static std::vector<uint8_t> BakeChunk(const AreaChunkRenderPtr& chunk, int texSz, bool isNormal)
    {
        std::vector<uint8_t> out(texSz * texSz * 4);
        if (!chunk) return out;

        const uint32_t* layerIds = chunk->getWorldLayerIDs();
        const float* layerScales = chunk->getLayerScales();

        std::vector<uint8_t> blend;
        constexpr int bSz = 65;
        const auto& bm = chunk->getBlendMap();
        const auto& bmDXT = chunk->getBlendMapDXT();

        if (!bm.empty()) blend = bm;
        else if (!bmDXT.empty() && Tex::File::decodeDXT1(bmDXT.data(), bSz, bSz, blend))
        {
            for (size_t i = 0; i < blend.size(); i += 4)
                blend[i+3] = static_cast<uint8_t>(std::max(0, 255 - blend[i] - blend[i+1] - blend[i+2]));
        }
        if (blend.empty()) { blend.resize(bSz * bSz * 4, 0); for (int i = 0; i < bSz * bSz; i++) blend[i*4] = 255; }

        std::vector<uint8_t> color;
        bool hasColor = false;
        const auto& cm = chunk->getColorMap();
        const auto& cmDXT = chunk->getColorMapDXT();
        if (!cm.empty()) { color = cm; hasColor = true; }
        else if (!cmDXT.empty() && Tex::File::decodeDXT5(cmDXT.data(), bSz, bSz, color)) hasColor = true;

        const CachedTex* layers[4];
        float scales[4];
        for (int i = 0; i < 4; i++)
        {
            layers[i] = layerIds[i] ? &gTexCache[layerIds[i]] : nullptr;
            scales[i] = (layers[i] && layers[i]->valid) ? (32.0f / layers[i]->scale) :
                        ((layerScales[i] > 0) ? (32.0f / layerScales[i]) : 8.0f);
        }

        float inv = 1.0f / (texSz - 1);
        for (int py = 0; py < texSz; py++)
        {
            float v = py * inv;
            for (int px = 0; px < texSz; px++)
            {
                float u = px * inv;

                glm::vec4 b = SampleBlend(blend, bSz, u, v);
                float sum = b.r + b.g + b.b + b.a;
                if (sum > 0.001f) b /= sum; else b = glm::vec4(1,0,0,0);

                glm::vec4 result(0);
                for (int i = 0; i < 4; i++)
                {
                    float w = b[i];
                    if (w < 0.001f) continue;
                    float lu = u * scales[i], lv = v * scales[i];

                    if (layers[i] && layers[i]->valid)
                    {
                        if (isNormal && !layers[i]->normal.empty())
                        {
                            glm::vec4 n = Sample(layers[i]->normal, layers[i]->w, layers[i]->h, lu, lv);
                            result += glm::vec4(n.r * 2 - 1, n.g * 2 - 1, n.b * 2 - 1, 0) * w;
                        }
                        else if (!isNormal)
                            result += Sample(layers[i]->diffuse, layers[i]->w, layers[i]->h, lu, lv) * w;
                        else
                            result += glm::vec4(0, 0, 1, 0) * w;
                    }
                    else
                    {
                        result += isNormal ? glm::vec4(0, 0, 1, 0) * w : glm::vec4(0.5f, 0.5f, 0.5f, 1) * w;
                    }
                }

                if (!isNormal && hasColor)
                {
                    glm::vec4 tint = SampleBlend(color, bSz, u, v);
                    result.r *= tint.r * 2; result.g *= tint.g * 2; result.b *= tint.b * 2;
                }

                if (isNormal)
                {
                    glm::vec3 n = glm::normalize(glm::vec3(result)) * 0.5f + 0.5f;
                    result = glm::vec4(n, 1);
                }

                int idx = (py * texSz + px) * 4;
                out[idx]   = uint8_t(std::clamp(result.r * 255.0f, 0.0f, 255.0f));
                out[idx+1] = uint8_t(std::clamp(result.g * 255.0f, 0.0f, 255.0f));
                out[idx+2] = uint8_t(std::clamp(result.b * 255.0f, 0.0f, 255.0f));
                out[idx+3] = 255;
            }
        }
        return out;
    }

    static std::vector<uint8_t> EncodePNG(const std::vector<uint8_t>& rgba, int w, int h)
    {
        std::vector<uint8_t> png;
        auto write32 = [&](uint32_t v) { png.push_back(v>>24); png.push_back(v>>16); png.push_back(v>>8); png.push_back(v); };

        uint32_t crcTab[256];
        for (uint32_t n = 0; n < 256; n++) { uint32_t c = n; for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320 ^ (c >> 1) : c >> 1; crcTab[n] = c; }
        auto crc32 = [&](const uint8_t* d, size_t len) { uint32_t c = 0xFFFFFFFF; for (size_t i = 0; i < len; i++) c = crcTab[(c ^ d[i]) & 0xFF] ^ (c >> 8); return c ^ 0xFFFFFFFF; };

        const uint8_t sig[] = {0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A};
        png.insert(png.end(), sig, sig + 8);

        uint8_t ihdr[13] = {uint8_t(w>>24),uint8_t(w>>16),uint8_t(w>>8),uint8_t(w),uint8_t(h>>24),uint8_t(h>>16),uint8_t(h>>8),uint8_t(h),8,6,0,0,0};
        write32(13); png.push_back('I'); png.push_back('H'); png.push_back('D'); png.push_back('R');
        png.insert(png.end(), ihdr, ihdr + 13);
        uint8_t ihdrCrc[17] = {'I','H','D','R'}; memcpy(ihdrCrc + 4, ihdr, 13);
        write32(crc32(ihdrCrc, 17));

        std::vector<uint8_t> raw;
        for (int y = 0; y < h; y++) { raw.push_back(0); for (int x = 0; x < w; x++) { int i = (y*w+x)*4; raw.insert(raw.end(), {rgba[i], rgba[i+1], rgba[i+2], rgba[i+3]}); } }

        std::vector<uint8_t> zlib = {0x78, 0x01};
        for (size_t p = 0; p < raw.size(); )
        {
            size_t bs = std::min(raw.size() - p, size_t(65535));
            bool last = (p + bs >= raw.size());
            zlib.push_back(last ? 1 : 0);
            zlib.push_back(bs & 0xFF); zlib.push_back((bs >> 8) & 0xFF);
            zlib.push_back(~bs & 0xFF); zlib.push_back((~bs >> 8) & 0xFF);
            zlib.insert(zlib.end(), raw.begin() + p, raw.begin() + p + bs);
            p += bs;
        }
        uint32_t a = 1, b = 0; for (uint8_t c : raw) { a = (a + c) % 65521; b = (b + a) % 65521; }
        uint32_t adler = (b << 16) | a;
        zlib.push_back(adler >> 24); zlib.push_back(adler >> 16); zlib.push_back(adler >> 8); zlib.push_back(adler);

        write32(static_cast<uint32_t>(zlib.size()));
        png.push_back('I'); png.push_back('D'); png.push_back('A'); png.push_back('T');
        png.insert(png.end(), zlib.begin(), zlib.end());
        std::vector<uint8_t> idatCrc(4 + zlib.size()); idatCrc[0]='I'; idatCrc[1]='D'; idatCrc[2]='A'; idatCrc[3]='T';
        memcpy(idatCrc.data() + 4, zlib.data(), zlib.size());
        write32(crc32(idatCrc.data(), idatCrc.size()));

        write32(0); png.push_back('I'); png.push_back('E'); png.push_back('N'); png.push_back('D');
        uint8_t iend[4] = {'I','E','N','D'}; write32(crc32(iend, 4));

        return png;
    }

    struct ChunkMesh
    {
        std::vector<float> pos, norm, uv;
        std::vector<uint8_t> vertexColors;
        std::vector<uint16_t> idx;
        glm::vec3 minP, maxP;
        std::vector<uint8_t> diffusePNG, normalPNG;
    };

    static void CollectChunks(const std::vector<AreaFilePtr>& areas, float scale, std::vector<ChunkMesh>& meshes,
                              bool exportTex, int texSz, ProgressCallback progress)
    {
        static std::vector<uint16_t> sharedIdx;
        if (sharedIdx.empty())
        {
            for (int y = 0; y < 16; y++)
            {
                for (int x = 0; x < 16; x++)
                {
                    uint16_t tl = y * 17 + x;
                    uint16_t tr = y * 17 + x + 1;
                    uint16_t bl = (y + 1) * 17 + x;
                    uint16_t br = (y + 1) * 17 + x + 1;
                    sharedIdx.insert(sharedIdx.end(), {tl, bl, tr, tr, bl, br});
                }
            }
        }

        int total = 0, cur = 0;
        for (const auto& a : areas) if (a) for (const auto& c : a->getChunks()) if (c && c->isFullyInitialized()) total++;

        for (const auto& area : areas)
        {
            if (!area) continue;
            glm::vec3 offset = area->getWorldOffset();

            for (const auto& chunk : area->getChunks())
            {
                if (!chunk || !chunk->isFullyInitialized()) continue;
                const auto& verts = chunk->getVertices();
                if (verts.empty()) continue;

                if (progress) progress(cur++, total, "Processing chunks...");

                ChunkMesh m;
                m.minP = glm::vec3(FLT_MAX); m.maxP = glm::vec3(-FLT_MAX);

                std::vector<uint8_t> blendRGBA;
                constexpr int blendSz = 65;
                const auto& bm = chunk->getBlendMap();
                const auto& bmDXT = chunk->getBlendMapDXT();

                if (!bm.empty()) blendRGBA = bm;
                else if (!bmDXT.empty() && Tex::File::decodeDXT1(bmDXT.data(), blendSz, blendSz, blendRGBA))
                {
                    for (size_t i = 0; i < blendRGBA.size(); i += 4)
                        blendRGBA[i+3] = static_cast<uint8_t>(std::max(0, 255 - blendRGBA[i] - blendRGBA[i+1] - blendRGBA[i+2]));
                }
                if (blendRGBA.empty())
                {
                    blendRGBA.resize(blendSz * blendSz * 4, 0);
                    for (int i = 0; i < blendSz * blendSz; i++) blendRGBA[i * 4] = 255;
                }

                for (size_t vi = 0; vi < verts.size(); vi++)
                {
                    const auto& v = verts[vi];

                    float px = (v.x + offset.x) * scale;
                    float py = v.y * scale;
                    float pz = (v.z + offset.z) * scale;

                    m.pos.insert(m.pos.end(), {px, py, pz});
                    m.norm.insert(m.norm.end(), {v.nx, v.ny, v.nz});

                    m.uv.insert(m.uv.end(), {v.u, v.v});

                    m.minP = glm::min(m.minP, glm::vec3(px, py, pz));
                    m.maxP = glm::max(m.maxP, glm::vec3(px, py, pz));

                    int bx = std::clamp(int(v.u * (blendSz - 1)), 0, blendSz - 1);
                    int by = std::clamp(int(v.v * (blendSz - 1)), 0, blendSz - 1);
                    int bi = (by * blendSz + bx) * 4;
                    m.vertexColors.insert(m.vertexColors.end(), {blendRGBA[bi], blendRGBA[bi+1], blendRGBA[bi+2], blendRGBA[bi+3]});
                }

                m.idx = sharedIdx;

                if (exportTex)
                {
                    m.diffusePNG = EncodePNG(BakeChunk(chunk, texSz, false), texSz, texSz);
                    m.normalPNG = EncodePNG(BakeChunk(chunk, texSz, true), texSz, texSz);
                }

                meshes.push_back(std::move(m));
            }
        }
    }

    static void WriteU32(std::vector<uint8_t>& buf, uint32_t v)
    {
        buf.push_back(v & 0xFF); buf.push_back((v >> 8) & 0xFF);
        buf.push_back((v >> 16) & 0xFF); buf.push_back((v >> 24) & 0xFF);
    }

    static void WriteF32(std::vector<uint8_t>& buf, float f)
    {
        uint32_t u; memcpy(&u, &f, 4); WriteU32(buf, u);
    }

    static void Pad(std::vector<uint8_t>& buf, size_t align)
    {
        while (buf.size() % align) buf.push_back(0);
    }

    ExportResult ExportAreasToGLB(const std::vector<AreaFilePtr>& areas, const ExportSettings& settings, ProgressCallback progress)
    {
        ExportResult result;
        if (areas.empty()) { result.errorMessage = "No areas"; return result; }

        std::string outDir = settings.outputPath;
        if (outDir.empty()) { result.errorMessage = "No output path"; return result; }
        std::filesystem::create_directories(outDir);

        std::string baseName = areas[0] ? ExtractAreaName(areas[0]->getPath()) : "terrain";
        std::string glbPath = outDir + "/" + baseName + ".glb";

        if (settings.exportTextures) PreloadTextures(areas, progress);

        std::vector<ChunkMesh> meshes;
        CollectChunks(areas, settings.scale, meshes, settings.exportTextures, settings.textureSize, progress);
        gTexCache.clear();

        if (meshes.empty()) { result.errorMessage = "No chunks"; return result; }

        if (progress) progress(0, 1, "Building GLB...");

        std::vector<uint8_t> bin;
        struct BufView { size_t offset, length; int target; };
        std::vector<BufView> views;
        struct Accessor { int view, type, compType, count; glm::vec3 minV, maxV; bool hasMinMax; };
        std::vector<Accessor> accessors;

        for (size_t i = 0; i < meshes.size(); i++)
        {
            auto& m = meshes[i];

            size_t posOff = bin.size();
            for (float f : m.pos) WriteF32(bin, f);
            Pad(bin, 4);
            views.push_back({posOff, m.pos.size() * 4, 34962});
            accessors.push_back({(int)views.size()-1, 3, 5126, (int)m.pos.size()/3, m.minP, m.maxP, true});

            size_t normOff = bin.size();
            for (float f : m.norm) WriteF32(bin, f);
            Pad(bin, 4);
            views.push_back({normOff, m.norm.size() * 4, 34962});
            accessors.push_back({(int)views.size()-1, 3, 5126, (int)m.norm.size()/3, {}, {}, false});

            size_t uvOff = bin.size();
            for (float f : m.uv) WriteF32(bin, f);
            Pad(bin, 4);
            views.push_back({uvOff, m.uv.size() * 4, 34962});
            accessors.push_back({(int)views.size()-1, 2, 5126, (int)m.uv.size()/2, {}, {}, false});

            size_t colorOff = bin.size();
            bin.insert(bin.end(), m.vertexColors.begin(), m.vertexColors.end());
            Pad(bin, 4);
            views.push_back({colorOff, m.vertexColors.size(), 34962});
            accessors.push_back({(int)views.size()-1, 4, 5121, (int)m.vertexColors.size()/4, {}, {}, false});

            size_t idxOff = bin.size();
            for (uint16_t idx : m.idx) { bin.push_back(idx & 0xFF); bin.push_back(idx >> 8); }
            Pad(bin, 4);
            views.push_back({idxOff, m.idx.size() * 2, 34963});
            accessors.push_back({(int)views.size()-1, 0, 5123, (int)m.idx.size(), {}, {}, false});
        }

        std::vector<std::pair<size_t, size_t>> imgRanges;
        if (settings.exportTextures)
        {
            for (auto& m : meshes)
            {
                size_t off = bin.size();
                bin.insert(bin.end(), m.diffusePNG.begin(), m.diffusePNG.end());
                Pad(bin, 4);
                imgRanges.push_back({off, m.diffusePNG.size()});

                off = bin.size();
                bin.insert(bin.end(), m.normalPNG.begin(), m.normalPNG.end());
                Pad(bin, 4);
                imgRanges.push_back({off, m.normalPNG.size()});
            }
        }

        std::string json = "{\"asset\":{\"version\":\"2.0\",\"generator\":\"WildStar\"},";
        json += "\"scene\":0,\"scenes\":[{\"nodes\":[";
        for (size_t i = 0; i < meshes.size(); i++) json += (i ? "," : "") + std::to_string(i);
        json += "]}],\"nodes\":[";
        for (size_t i = 0; i < meshes.size(); i++)
            json += std::string(i ? "," : "") + "{\"mesh\":" + std::to_string(i) + ",\"name\":\"Chunk_" + std::to_string(i) + "\"}";
        json += "],\"meshes\":[";

        for (size_t i = 0; i < meshes.size(); i++)
        {
            int base = static_cast<int>(i) * 5;
            json += std::string(i ? "," : "") + "{\"primitives\":[{\"attributes\":{\"POSITION\":" + std::to_string(base) +
                    ",\"NORMAL\":" + std::to_string(base+1) +
                    ",\"TEXCOORD_0\":" + std::to_string(base+2) +
                    ",\"COLOR_0\":" + std::to_string(base+3) +
                    "},\"indices\":" + std::to_string(base+4);
            if (settings.exportTextures) json += ",\"material\":" + std::to_string(i);
            json += "}]}";
        }
        json += "],";

        if (settings.exportTextures)
        {
            json += "\"materials\":[";
            for (size_t i = 0; i < meshes.size(); i++)
            {
                json += std::string(i ? "," : "") + "{\"pbrMetallicRoughness\":{\"baseColorTexture\":{\"index\":" +
                        std::to_string(i*2) + "},\"metallicFactor\":0,\"roughnessFactor\":1},\"normalTexture\":{\"index\":" +
                        std::to_string(i*2+1) + "}}";
            }
            json += "],\"textures\":[";
            for (size_t i = 0; i < meshes.size() * 2; i++)
                json += std::string(i ? "," : "") + "{\"sampler\":0,\"source\":" + std::to_string(i) + "}";
            json += "],\"samplers\":[{\"magFilter\":9729,\"minFilter\":9987,\"wrapS\":10497,\"wrapT\":10497}],\"images\":[";

            int imgViewBase = static_cast<int>(views.size());
            for (size_t i = 0; i < imgRanges.size(); i++)
                json += std::string(i ? "," : "") + "{\"bufferView\":" + std::to_string(imgViewBase + i) + ",\"mimeType\":\"image/png\"}";
            json += "],";

            for (auto& r : imgRanges)
                views.push_back({r.first, r.second, 0});
        }

        json += "\"accessors\":[";
        for (size_t i = 0; i < accessors.size(); i++)
        {
            auto& a = accessors[i];
            const char* types[] = {"SCALAR", "", "VEC2", "VEC3", "VEC4"};
            json += std::string(i ? "," : "") + "{\"bufferView\":" + std::to_string(a.view) +
                    ",\"componentType\":" + std::to_string(a.compType) +
                    ",\"count\":" + std::to_string(a.count) +
                    ",\"type\":\"" + types[a.type] + "\"";

            if (a.compType == 5121) json += ",\"normalized\":true";

            if (a.hasMinMax)
            {
                json += ",\"min\":[" + std::to_string(a.minV.x) + "," + std::to_string(a.minV.y) + "," + std::to_string(a.minV.z) + "]";
                json += ",\"max\":[" + std::to_string(a.maxV.x) + "," + std::to_string(a.maxV.y) + "," + std::to_string(a.maxV.z) + "]";
            }
            json += "}";
        }
        json += "],\"bufferViews\":[";
        for (size_t i = 0; i < views.size(); i++)
        {
            json += std::string(i ? "," : "") + "{\"buffer\":0,\"byteOffset\":" + std::to_string(views[i].offset) +
                    ",\"byteLength\":" + std::to_string(views[i].length);
            if (views[i].target) json += ",\"target\":" + std::to_string(views[i].target);
            json += "}";
        }
        json += "],\"buffers\":[{\"byteLength\":" + std::to_string(bin.size()) + "}]}";

        while (json.size() % 4) json += ' ';

        std::ofstream out(glbPath, std::ios::binary);
        if (!out) { result.errorMessage = "Can't write file"; return result; }

        uint32_t totalLen = 12 + 8 + static_cast<uint32_t>(json.size()) + 8 + static_cast<uint32_t>(bin.size());

        std::vector<uint8_t> header;
        WriteU32(header, 0x46546C67);
        WriteU32(header, 2);
        WriteU32(header, totalLen);
        out.write(reinterpret_cast<char*>(header.data()), header.size());

        std::vector<uint8_t> jsonChunk;
        WriteU32(jsonChunk, static_cast<uint32_t>(json.size()));
        WriteU32(jsonChunk, 0x4E4F534A);
        out.write(reinterpret_cast<char*>(jsonChunk.data()), jsonChunk.size());
        out.write(json.data(), json.size());

        std::vector<uint8_t> binChunk;
        WriteU32(binChunk, static_cast<uint32_t>(bin.size()));
        WriteU32(binChunk, 0x004E4942);
        out.write(reinterpret_cast<char*>(binChunk.data()), binChunk.size());
        out.write(reinterpret_cast<char*>(bin.data()), bin.size());

        result.success = true;
        result.outputFile = glbPath;
        result.chunkCount = static_cast<int>(meshes.size());
        result.textureCount = settings.exportTextures ? static_cast<int>(meshes.size()) * 2 : 0;
        for (const auto& m : meshes) { result.vertexCount += static_cast<int>(m.pos.size()) / 3; result.triangleCount += static_cast<int>(m.idx.size()) / 3; }

        return result;
    }

    ExportResult ExportAreaToGLB(const AreaFilePtr& area, const ExportSettings& settings, ProgressCallback progress)
    {
        return ExportAreasToGLB({area}, settings, progress);
    }

    std::string GetSuggestedFilename(const AreaFilePtr& area)
    {
        return area ? ExtractAreaName(area->getPath()) : "terrain";
    }
}