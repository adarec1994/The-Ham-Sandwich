#include "TerrainExport.h"
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
#include <sstream>
#include <iomanip>

namespace TerrainExport
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

    static std::string Base64Encode(const uint8_t* data, size_t len)
    {
        static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string result;
        result.reserve(((len + 2) / 3) * 4);
        for (size_t i = 0; i < len; i += 3)
        {
            uint32_t n = static_cast<uint32_t>(data[i]) << 16;
            if (i + 1 < len) n |= static_cast<uint32_t>(data[i + 1]) << 8;
            if (i + 2 < len) n |= static_cast<uint32_t>(data[i + 2]);
            result += chars[(n >> 18) & 0x3F];
            result += chars[(n >> 12) & 0x3F];
            result += (i + 1 < len) ? chars[(n >> 6) & 0x3F] : '=';
            result += (i + 2 < len) ? chars[n & 0x3F] : '=';
        }
        return result;
    }

    static void FlipImageVertically(std::vector<uint8_t>& rgba, int w, int h)
    {
        std::vector<uint8_t> row(w * 4);
        for (int y = 0; y < h / 2; y++)
        {
            int topIdx = y * w * 4;
            int botIdx = (h - 1 - y) * w * 4;
            memcpy(row.data(), &rgba[topIdx], w * 4);
            memcpy(&rgba[topIdx], &rgba[botIdx], w * 4);
            memcpy(&rgba[botIdx], row.data(), w * 4);
        }
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

        write32(uint32_t(zlib.size()));
        png.push_back('I'); png.push_back('D'); png.push_back('A'); png.push_back('T');
        png.insert(png.end(), zlib.begin(), zlib.end());
        std::vector<uint8_t> idatCrc(4 + zlib.size()); idatCrc[0]='I'; idatCrc[1]='D'; idatCrc[2]='A'; idatCrc[3]='T';
        memcpy(idatCrc.data() + 4, zlib.data(), zlib.size());
        write32(crc32(idatCrc.data(), idatCrc.size()));

        write32(0); png.push_back('I'); png.push_back('E'); png.push_back('N'); png.push_back('D');
        uint8_t iend[4] = {'I','E','N','D'}; write32(crc32(iend, 4));
        return png;
    }

    static std::string EncodeImageBase64(std::vector<uint8_t> rgba, int w, int h, bool flip = false)
    {
        if (flip) FlipImageVertically(rgba, w, h);
        auto png = EncodePNG(rgba, w, h);
        return Base64Encode(png.data(), png.size());
    }

    static bool DecodeBlendMap(const AreaChunkRenderPtr& chunk, std::vector<uint8_t>& out)
    {
        constexpr int sz = 65;
        const auto& bm = chunk->getBlendMap();
        const auto& bmDXT = chunk->getBlendMapDXT();

        if (!bm.empty()) { out = bm; return true; }

        if (!bmDXT.empty() && Tex::File::decodeDXT1(bmDXT.data(), sz, sz, out))
        {
            for (size_t i = 0; i < out.size(); i += 4)
            {
                int sum = out[i] + out[i+1] + out[i+2];
                out[i+3] = static_cast<uint8_t>(std::max(0, 255 - sum));
            }
            return true;
        }

        out.resize(sz * sz * 4, 0);
        for (int i = 0; i < sz * sz; i++) out[i * 4] = 255;
        return true;
    }

    static bool DecodeColorMap(const AreaChunkRenderPtr& chunk, std::vector<uint8_t>& out)
    {
        constexpr int sz = 65;
        const auto& cm = chunk->getColorMap();
        const auto& cmDXT = chunk->getColorMapDXT();

        if (!cm.empty()) { out = cm; return true; }
        if (!cmDXT.empty() && Tex::File::decodeDXT5(cmDXT.data(), sz, sz, out)) return true;
        return false;
    }

    ExportResult ExportAreasToTerrain(const std::vector<AreaFilePtr>& areas, const ExportSettings& settings, ProgressCallback progress)
    {
        ExportResult result;
        if (areas.empty()) { result.errorMessage = "No areas"; return result; }

        std::string outDir = settings.outputPath;
        if (outDir.empty()) { result.errorMessage = "No output path"; return result; }
        std::filesystem::create_directories(outDir);

        std::string baseName = areas[0] ? ExtractAreaName(areas[0]->getPath()) : "terrain";
        std::string jsonPath = outDir + "/" + baseName + ".wsterrain";

        ArchivePtr archive;
        for (const auto& a : areas) if (a && a->getArchive()) { archive = a->getArchive(); break; }

        std::set<uint32_t> uniqueLayerIds;
        for (const auto& area : areas)
        {
            if (!area) continue;
            for (const auto& chunk : area->getChunks())
            {
                if (!chunk) continue;
                const uint32_t* ids = chunk->getWorldLayerIDs();
                for (int i = 0; i < 4; i++) if (ids[i]) uniqueLayerIds.insert(ids[i]);
            }
        }

        auto& texMgr = TerrainTexture::Manager::Instance();
        if (archive) texMgr.LoadWorldLayerTable(archive);

        std::ofstream out(jsonPath);
        if (!out) { result.errorMessage = "Can't write file"; return result; }

        out << std::fixed << std::setprecision(6);
        out << "{\n";
        out << "\"version\": 2,\n";
        out << "\"name\": \"" << baseName << "\",\n";

        out << "\"layers\": {\n";
        int layerIdx = 0;
        int totalLayers = static_cast<int>(uniqueLayerIds.size());
        for (uint32_t layerId : uniqueLayerIds)
        {
            if (progress) progress(layerIdx, totalLayers, "Exporting layer textures...");

            const auto* entry = texMgr.GetLayerEntry(layerId);
            float scale = (entry && entry->scaleU > 0) ? entry->scaleU : 4.0f;

            out << (layerIdx > 0 ? ",\n" : "") << "\"" << layerId << "\": {\n";
            out << "  \"scale\": " << scale;

            if (archive && entry)
            {
                TerrainTexture::RawTextureData raw;
                if (!entry->diffusePath.empty() && texMgr.LoadRawTextureFromPath(archive, entry->diffusePath, raw))
                {
                    out << ",\n  \"diffuse\": {\"width\": " << raw.width << ", \"height\": " << raw.height
                        << ", \"data\": \"" << EncodeImageBase64(raw.rgba, raw.width, raw.height, true) << "\"}";
                }
                if (!entry->normalPath.empty() && texMgr.LoadRawTextureFromPath(archive, entry->normalPath, raw))
                {
                    out << ",\n  \"normal\": {\"width\": " << raw.width << ", \"height\": " << raw.height
                        << ", \"data\": \"" << EncodeImageBase64(raw.rgba, raw.width, raw.height, true) << "\"}";
                }
            }
            out << "\n}";
            layerIdx++;
        }
        out << "\n},\n";

        out << "\"chunks\": [\n";

        int chunkIdx = 0;
        int totalChunks = 0;
        for (const auto& area : areas)
            if (area) for (const auto& c : area->getChunks())
                if (c && c->isFullyInitialized()) totalChunks++;

        bool firstChunk = true;
        for (const auto& area : areas)
        {
            if (!area) continue;
            glm::vec3 offset = area->getWorldOffset();

            for (const auto& chunk : area->getChunks())
            {
                if (!chunk || !chunk->isFullyInitialized()) continue;
                const auto& verts = chunk->getVertices();
                if (verts.empty()) continue;

                if (progress) progress(chunkIdx, totalChunks, "Exporting chunks...");

                if (!firstChunk) out << ",\n";
                firstChunk = false;

                out << "{\n";
                out << "  \"index\": " << chunkIdx << ",\n";

                const uint32_t* layerIds = chunk->getWorldLayerIDs();
                out << "  \"layerIds\": [" << layerIds[0] << "," << layerIds[1] << "," << layerIds[2] << "," << layerIds[3] << "],\n";

                out << "  \"positions\": [";
                for (size_t i = 0; i < verts.size(); i++)
                {
                    const auto& v = verts[i];
                    if (i > 0) out << ",";
                    out << (v.x + offset.x) * settings.scale << "," << v.y * settings.scale << "," << (v.z + offset.z) * settings.scale;
                }
                out << "],\n";

                out << "  \"normals\": [";
                for (size_t i = 0; i < verts.size(); i++)
                {
                    const auto& v = verts[i];
                    if (i > 0) out << ",";
                    out << v.nx << "," << v.ny << "," << v.nz;
                }
                out << "],\n";

                out << "  \"uvs\": [";
                for (size_t i = 0; i < verts.size(); i++)
                {
                    const auto& v = verts[i];
                    if (i > 0) out << ",";
                    out << v.u << "," << v.v;
                }
                out << "],\n";

                out << "  \"indices\": [";
                bool firstIdx = true;
                for (int y = 0; y < 16; y++)
                {
                    for (int x = 0; x < 16; x++)
                    {
                        uint32_t tl = y * 17 + x;
                        uint32_t tr = y * 17 + x + 1;
                        uint32_t bl = (y + 1) * 17 + x;
                        uint32_t br = (y + 1) * 17 + x + 1;
                        if (!firstIdx) out << ",";
                        out << tl << "," << bl << "," << tr << "," << tr << "," << bl << "," << br;
                        firstIdx = false;
                    }
                }
                out << "],\n";

                std::vector<uint8_t> blendRGBA;
                DecodeBlendMap(chunk, blendRGBA);
                out << "  \"blendMap\": {\"width\": 65, \"height\": 65, \"data\": \"" << EncodeImageBase64(blendRGBA, 65, 65, true) << "\"}";

                std::vector<uint8_t> colorRGBA;
                if (DecodeColorMap(chunk, colorRGBA))
                {
                    out << ",\n  \"colorMap\": {\"width\": 65, \"height\": 65, \"data\": \"" << EncodeImageBase64(colorRGBA, 65, 65, false) << "\"}";
                }

                out << "\n}";
                chunkIdx++;
                result.chunkCount++;
            }
        }

        out << "\n]\n";
        out << "}\n";
        out.close();

        result.success = true;
        result.outputFile = jsonPath;
        result.textureCount = static_cast<int>(uniqueLayerIds.size());
        return result;
    }

    ExportResult ExportAreaToTerrain(const AreaFilePtr& area, const ExportSettings& settings, ProgressCallback progress)
    {
        return ExportAreasToTerrain({area}, settings, progress);
    }

    std::string GetSuggestedFilename(const AreaFilePtr& area)
    {
        return area ? ExtractAreaName(area->getPath()) : "terrain";
    }
}