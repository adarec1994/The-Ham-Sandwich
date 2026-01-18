#include "M3Export.h"
#include "../models/M3Render.h"
#include "../Archive.h"
#include "../tex/tex.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <cfloat>
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
    static std::vector<uint8_t> EncodePNG_RGB(const uint8_t* rgba, int width, int height)
    {
        std::vector<uint8_t> png;
        const uint8_t sig[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
        png.insert(png.end(), sig, sig + 8);
        std::vector<uint8_t> ihdr;
        WritePngU32BE(ihdr, width);
        WritePngU32BE(ihdr, height);
        ihdr.push_back(8);
        ihdr.push_back(2);
        ihdr.push_back(0);
        ihdr.push_back(0);
        ihdr.push_back(0);
        WritePngChunk(png, "IHDR", ihdr);
        std::vector<uint8_t> rawData;
        for (int y = 0; y < height; y++)
        {
            rawData.push_back(0);
            for (int x = 0; x < width; x++)
            {
                int srcIdx = (y * width + x) * 4;
                rawData.push_back(rgba[srcIdx + 0]);
                rawData.push_back(rgba[srcIdx + 1]);
                rawData.push_back(rgba[srcIdx + 2]);
            }
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
        for (unsigned char c : s)
        {
            if (c == '"') result += "\\\"";
            else if (c == '\\') result += "\\\\";
            else if (c == '\n') result += "\\n";
            else if (c == '\r') result += "\\r";
            else if (c == '\t') result += "\\t";
            else if (c < 32)
            {
                char buf[8];
                snprintf(buf, sizeof(buf), "\\u%04x", c);
                result += buf;
            }
            else result += c;
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
    static void WriteU16(std::vector<uint8_t>& buf, uint16_t v)
    {
        buf.push_back(v & 0xFF);
        buf.push_back((v >> 8) & 0xFF);
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
    static std::string Base64Encode(const uint8_t* data, size_t len)
    {
        static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string result;
        result.reserve((len + 2) / 3 * 4);
        for (size_t i = 0; i < len; i += 3) {
            uint32_t n = (uint32_t)data[i] << 16;
            if (i + 1 < len) n |= (uint32_t)data[i + 1] << 8;
            if (i + 2 < len) n |= (uint32_t)data[i + 2];
            result += chars[(n >> 18) & 0x3F];
            result += chars[(n >> 12) & 0x3F];
            result += (i + 1 < len) ? chars[(n >> 6) & 0x3F] : '=';
            result += (i + 2 < len) ? chars[n & 0x3F] : '=';
        }
        return result;
    }
    static std::vector<uint8_t> LoadTextureAsPNG(const ArchivePtr& arc, const std::string& path)
    {
        if (!arc || path.empty()) return {};
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
        std::wstring wp = conv.from_bytes(path);
        if (wp.find(L".tex") == std::wstring::npos) wp += L".tex";
        auto entry = arc->getByPath(wp);
        if (!entry) {
            std::wstring wp2 = wp;
            std::replace(wp2.begin(), wp2.end(), L'/', L'\\');
            entry = arc->getByPath(wp2);
            if (!entry) {
                std::replace(wp2.begin(), wp2.end(), L'\\', L'/');
                entry = arc->getByPath(wp2);
            }
        }
        if (!entry) return {};
        std::vector<uint8_t> buffer;
        arc->getFileData(std::dynamic_pointer_cast<FileEntry>(entry), buffer);
        if (buffer.empty()) return {};
        Tex::File tf;
        if (!tf.readFromMemory(buffer.data(), buffer.size())) return {};
        Tex::ImageRGBA img;
        if (!tf.decodeLargestMipToRGBA(img)) return {};
        return EncodePNG_RGB(img.rgba.data(), img.width, img.height);
    }
    static int64_t gFbxIdCounter = 1000000000;
    static int64_t GenFbxId() { return gFbxIdCounter++; }
    static std::string FbxF(double v) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(6) << v;
        return oss.str();
    }
    ExportResult ExportToFBX(M3Render* render, const ArchivePtr& archive, const ExportSettings& settings, ProgressCallback progress)
    {
        ExportResult result;
        if (!render) { result.errorMessage = "No model"; return result; }
        std::string outputDir = settings.outputPath;
        if (outputDir.empty()) { result.errorMessage = "No output path"; return result; }
        std::filesystem::create_directories(outputDir);
        std::string baseName = settings.customName.empty() ? ExtractModelName(render->getModelName()) : SanitizeFilename(settings.customName);
        std::string fbxPath = outputDir + "/" + baseName + ".fbx";
        const float SCALE = 100.0f;
        if (progress) progress(0, 100, "Collecting geometry...");
        const auto& allVertices = render->getVertices();
        const auto& allIndices = render->getIndices();
        const auto& submeshes = render->getAllSubmeshes();
        const auto& bones = render->getAllBones();
        const auto& materials = render->getAllMaterials();
        const auto& textures = render->getAllTextures();
        const auto& animations = render->getAllAnimations();
        std::vector<glm::mat4> effectiveBindGlobal(bones.size());
        for (size_t i = 0; i < bones.size(); ++i) {
            const auto& bone = bones[i];
            bool isRootBone = (bone.parentId < 0 || bone.parentId >= (int)bones.size());
            bool boneAtOrigin = glm::length(glm::vec3(bone.globalMatrix[3])) < 0.001f;
            if (boneAtOrigin && !bone.tracks[6].keyframes.empty()) {
                glm::vec3 track6Pos = bone.tracks[6].keyframes[0].translation;
                glm::mat4 localT = glm::translate(glm::mat4(1.0f), track6Pos);
                if (!isRootBone) {
                    effectiveBindGlobal[i] = effectiveBindGlobal[bone.parentId] * localT;
                } else {
                    effectiveBindGlobal[i] = localT;
                }
            } else {
                effectiveBindGlobal[i] = bone.globalMatrix;
            }
        }
        struct SubmeshData {
            std::vector<glm::vec3> positions;
            std::vector<glm::vec3> normals;
            std::vector<glm::vec2> uvs;
            std::vector<glm::uvec4> joints;
            std::vector<glm::vec4> weights;
            std::vector<uint32_t> indices;
            uint16_t materialId;
            std::string name;
        };
        std::vector<SubmeshData> meshList;
        size_t totalVerts = 0, totalTris = 0;
        for (size_t si = 0; si < submeshes.size(); ++si) {
            if (!render->getSubmeshVisible(si)) continue;
            const auto& sm = submeshes[si];
            SubmeshData sd;
            sd.materialId = sm.materialID;
            sd.name = "Mesh_" + std::to_string(si);
            std::unordered_map<uint32_t, uint32_t> remap;
            for (uint32_t i = 0; i < sm.indexCount; ++i) {
                uint32_t idx = allIndices[sm.startIndex + i] + sm.startVertex;
                if (idx >= allVertices.size()) continue;
                auto it = remap.find(idx);
                if (it != remap.end()) {
                    sd.indices.push_back(it->second);
                } else {
                    uint32_t ni = static_cast<uint32_t>(sd.positions.size());
                    remap[idx] = ni;
                    const auto& v = allVertices[idx];
                    sd.positions.push_back(v.position);
                    sd.normals.push_back(v.normal);
                    sd.uvs.push_back(v.uv1);
                    sd.joints.push_back(v.boneIndices);
                    sd.weights.push_back(v.boneWeights);
                    sd.indices.push_back(ni);
                }
            }
            if (!sd.positions.empty() && !sd.indices.empty()) {
                totalVerts += sd.positions.size();
                totalTris += sd.indices.size() / 3;
                meshList.push_back(std::move(sd));
            }
        }
        if (meshList.empty()) { result.errorMessage = "No visible submeshes"; return result; }
        result.vertexCount = static_cast<int>(totalVerts);
        result.triangleCount = static_cast<int>(totalTris);
        result.boneCount = static_cast<int>(bones.size());
        result.animationCount = settings.exportAnimations ? static_cast<int>(animations.size()) : 0;
        bool hasSkeleton = !bones.empty() && settings.exportSkeleton;
        bool hasAnimations = hasSkeleton && settings.exportAnimations && !animations.empty();
        if (progress) progress(10, 100, "Loading textures...");
        struct FbxTexture { std::string name; std::vector<uint8_t> data; int64_t texId; int64_t vidId; };
        std::vector<FbxTexture> fbxTextures;
        std::unordered_map<uint16_t, int> matIdToDiffuseIdx;
        std::unordered_map<uint16_t, int> matIdToNormalIdx;
        std::unordered_map<std::string, int> pathToTexIdx;
        auto LoadOrGetTexture = [&](const std::string& texPath) -> int {
            if (texPath.empty()) return -1;
            auto it = pathToTexIdx.find(texPath);
            if (it != pathToTexIdx.end()) return it->second;
            auto png = LoadTextureAsPNG(archive, texPath);
            if (png.empty()) return -1;
            std::string texName = texPath;
            size_t slash = texName.rfind('/');
            if (slash == std::string::npos) slash = texName.rfind('\\');
            if (slash != std::string::npos) texName = texName.substr(slash + 1);
            size_t dot = texName.rfind('.');
            if (dot != std::string::npos) texName = texName.substr(0, dot);
            texName = SanitizeFilename(texName);
            if (texName.empty()) texName = "texture_" + std::to_string(fbxTextures.size());
            int idx = static_cast<int>(fbxTextures.size());
            fbxTextures.push_back({texName, std::move(png), 0, 0});
            pathToTexIdx[texPath] = idx;
            return idx;
        };
        if (settings.exportTextures && archive) {
            for (const auto& mesh : meshList) {
                if (matIdToDiffuseIdx.count(mesh.materialId)) continue;
                if (mesh.materialId >= materials.size()) continue;
                const auto& mat = materials[mesh.materialId];
                if (mat.variants.empty()) continue;
                int variantIdx = render->getMaterialSelectedVariant(mesh.materialId);
                if (variantIdx < 0 || variantIdx >= (int)mat.variants.size()) variantIdx = 0;
                const auto& variant = mat.variants[variantIdx];
                std::string diffusePath = variant.textureColorPath;
                if (diffusePath.empty() && variant.textureIndexA >= 0 && variant.textureIndexA < (int)textures.size())
                    diffusePath = textures[variant.textureIndexA].path;
                std::string normalPath = variant.textureNormalPath;
                if (normalPath.empty() && variant.textureIndexB >= 0 && variant.textureIndexB < (int)textures.size())
                    normalPath = textures[variant.textureIndexB].path;
                int diffuseIdx = LoadOrGetTexture(diffusePath);
                int normalIdx = LoadOrGetTexture(normalPath);
                matIdToDiffuseIdx[mesh.materialId] = diffuseIdx;
                matIdToNormalIdx[mesh.materialId] = normalIdx;
            }
            result.textureCount = static_cast<int>(fbxTextures.size());
        }
        if (progress) progress(20, 100, "Building FBX...");
        gFbxIdCounter = 1000000000;
        std::vector<int64_t> meshIds, meshGeoIds, materialIds, boneIds, boneAttrIds, clusterIds, skinIds;
        int64_t rootId = GenFbxId();
        for (size_t i = 0; i < meshList.size(); ++i) { meshIds.push_back(GenFbxId()); meshGeoIds.push_back(GenFbxId()); }
        for (size_t i = 0; i < meshList.size(); ++i) materialIds.push_back(GenFbxId());
        for (auto& tex : fbxTextures) { tex.texId = GenFbxId(); tex.vidId = GenFbxId(); }
        if (hasSkeleton) {
            for (size_t i = 0; i < bones.size(); ++i) { boneIds.push_back(GenFbxId()); boneAttrIds.push_back(GenFbxId()); }
            for (size_t i = 0; i < meshList.size(); ++i) skinIds.push_back(GenFbxId());
            for (size_t m = 0; m < meshList.size(); ++m)
                for (size_t b = 0; b < bones.size(); ++b) clusterIds.push_back(GenFbxId());
        }
        struct FbxAnimCurve { int64_t id; std::vector<int64_t> times; std::vector<float> values; };
        struct FbxAnimCurveNode { int64_t id; std::string prop; int64_t curveX, curveY, curveZ; size_t boneIdx; };
        struct FbxAnimLayer { int64_t id; std::vector<FbxAnimCurveNode> curveNodes; std::vector<FbxAnimCurve> curves; };
        struct FbxAnimStack { int64_t id; std::string name; int64_t layerId; int64_t startTime; int64_t endTime; FbxAnimLayer layer; };
        std::vector<FbxAnimStack> animStacks;
        auto QuatToEulerXYZ = [](const glm::quat& q) -> glm::vec3 {
            glm::mat3 m = glm::mat3_cast(q);
            float rx = atan2(m[1][2], m[2][2]) * 57.2957795f;
            float ry = atan2(-m[0][2], sqrt(m[1][2]*m[1][2] + m[2][2]*m[2][2])) * 57.2957795f;
            float rz = atan2(m[0][1], m[0][0]) * 57.2957795f;
            return glm::vec3(rx, ry, rz);
        };
        if (hasAnimations) {
            for (size_t ai = 0; ai < animations.size(); ++ai) {
                const auto& anim = animations[ai];
                FbxAnimStack stack;
                stack.id = GenFbxId();
                stack.name = "Animation_" + std::to_string(anim.sequenceId);
                stack.layerId = GenFbxId();
                float startMs = (float)anim.timestampStart;
                float endMs = (float)anim.timestampEnd;
                float durationMs = endMs - startMs;
                stack.startTime = 0;
                stack.endTime = (int64_t)((durationMs / 1000.0f) * 46186158000.0);
                stack.layer.id = stack.layerId;
                for (size_t bi = 0; bi < bones.size(); ++bi) {
                    const auto& bone = bones[bi];
                    bool boneAtOrigin = glm::length(glm::vec3(bone.globalMatrix[3])) < 0.001f;
                    const M3AnimationTrack* transTrack = nullptr;
                    const M3AnimationTrack* rotTrack = nullptr;
                    const M3AnimationTrack* scaleTrack = nullptr;
                    if (!bone.tracks[6].keyframes.empty() && boneAtOrigin) {
                        transTrack = &bone.tracks[6];
                    }
                    for (int t = 4; t <= 5; ++t) if (!bone.tracks[t].keyframes.empty()) { rotTrack = &bone.tracks[t]; break; }
                    for (int t = 0; t <= 2; ++t) if (!bone.tracks[t].keyframes.empty()) { scaleTrack = &bone.tracks[t]; break; }
                    if (transTrack && !transTrack->keyframes.empty()) {
                        FbxAnimCurveNode node;
                        node.id = GenFbxId();
                        node.prop = "Lcl Translation";
                        node.boneIdx = bi;
                        FbxAnimCurve cx, cy, cz;
                        cx.id = GenFbxId(); cy.id = GenFbxId(); cz.id = GenFbxId();
                        for (const auto& kf : transTrack->keyframes) {
                            float ktMs = (float)kf.timestamp;
                            if (ktMs >= startMs && ktMs <= endMs) {
                                float relativeMs = ktMs - startMs;
                                int64_t fbxTime = (int64_t)((relativeMs / 1000.0f) * 46186158000.0);
                                cx.times.push_back(fbxTime); cx.values.push_back(kf.translation.x);
                                cy.times.push_back(fbxTime); cy.values.push_back(kf.translation.y);
                                cz.times.push_back(fbxTime); cz.values.push_back(kf.translation.z);
                            }
                        }
                        if (cx.times.size() >= 2) {
                            node.curveX = cx.id; node.curveY = cy.id; node.curveZ = cz.id;
                            stack.layer.curveNodes.push_back(node);
                            stack.layer.curves.push_back(std::move(cx));
                            stack.layer.curves.push_back(std::move(cy));
                            stack.layer.curves.push_back(std::move(cz));
                        }
                    }
                    if (rotTrack && !rotTrack->keyframes.empty()) {
                        FbxAnimCurveNode node;
                        node.id = GenFbxId();
                        node.prop = "Lcl Rotation";
                        node.boneIdx = bi;
                        FbxAnimCurve cx, cy, cz;
                        cx.id = GenFbxId(); cy.id = GenFbxId(); cz.id = GenFbxId();
                        for (const auto& kf : rotTrack->keyframes) {
                            float ktMs = (float)kf.timestamp;
                            if (ktMs >= startMs && ktMs <= endMs) {
                                float relativeMs = ktMs - startMs;
                                int64_t fbxTime = (int64_t)((relativeMs / 1000.0f) * 46186158000.0);
                                glm::vec3 euler = QuatToEulerXYZ(kf.rotation);
                                cx.times.push_back(fbxTime); cx.values.push_back(euler.x);
                                cy.times.push_back(fbxTime); cy.values.push_back(euler.y);
                                cz.times.push_back(fbxTime); cz.values.push_back(euler.z);
                            }
                        }
                        if (cx.times.size() >= 2) {
                            node.curveX = cx.id; node.curveY = cy.id; node.curveZ = cz.id;
                            stack.layer.curveNodes.push_back(node);
                            stack.layer.curves.push_back(std::move(cx));
                            stack.layer.curves.push_back(std::move(cy));
                            stack.layer.curves.push_back(std::move(cz));
                        }
                    }
                    if (scaleTrack && !scaleTrack->keyframes.empty()) {
                        FbxAnimCurveNode node;
                        node.id = GenFbxId();
                        node.prop = "Lcl Scaling";
                        node.boneIdx = bi;
                        FbxAnimCurve cx, cy, cz;
                        cx.id = GenFbxId(); cy.id = GenFbxId(); cz.id = GenFbxId();
                        for (const auto& kf : scaleTrack->keyframes) {
                            float ktMs = (float)kf.timestamp;
                            if (ktMs >= startMs && ktMs <= endMs) {
                                float relativeMs = ktMs - startMs;
                                int64_t fbxTime = (int64_t)((relativeMs / 1000.0f) * 46186158000.0);
                                cx.times.push_back(fbxTime); cx.values.push_back(kf.scale.x);
                                cy.times.push_back(fbxTime); cy.values.push_back(kf.scale.y);
                                cz.times.push_back(fbxTime); cz.values.push_back(kf.scale.z);
                            }
                        }
                        if (cx.times.size() >= 2) {
                            node.curveX = cx.id; node.curveY = cy.id; node.curveZ = cz.id;
                            stack.layer.curveNodes.push_back(node);
                            stack.layer.curves.push_back(std::move(cx));
                            stack.layer.curves.push_back(std::move(cy));
                            stack.layer.curves.push_back(std::move(cz));
                        }
                    }
                }
                if (!stack.layer.curveNodes.empty()) animStacks.push_back(std::move(stack));
            }
        }
        int animStackCount = (int)animStacks.size();
        int animLayerCount = animStackCount;
        int animCurveNodeCount = 0, animCurveCount = 0;
        for (const auto& stack : animStacks) {
            animCurveNodeCount += (int)stack.layer.curveNodes.size();
            animCurveCount += (int)stack.layer.curves.size();
        }
        std::ostringstream fbx;
        fbx << std::fixed << std::setprecision(6);
        fbx << "; FBX 7.5.0 project file\n; Created by WildStar M3 Exporter\n";
        fbx << "FBXHeaderExtension:  {\n\tFBXHeaderVersion: 1003\n\tFBXVersion: 7500\n";
        fbx << "\tCreationTimeStamp:  {\n\t\tVersion: 1000\n\t\tYear: 2025\n\t\tMonth: 1\n\t\tDay: 1\n";
        fbx << "\t\tHour: 0\n\t\tMinute: 0\n\t\tSecond: 0\n\t\tMillisecond: 0\n\t}\n";
        fbx << "\tCreator: \"WildStar M3 Exporter\"\n}\n\n";
        fbx << "GlobalSettings:  {\n\tVersion: 1000\n\tProperties70:  {\n";
        fbx << "\t\tP: \"UpAxis\", \"int\", \"Integer\", \"\", 1\n";
        fbx << "\t\tP: \"UpAxisSign\", \"int\", \"Integer\", \"\", 1\n";
        fbx << "\t\tP: \"FrontAxis\", \"int\", \"Integer\", \"\", 2\n";
        fbx << "\t\tP: \"FrontAxisSign\", \"int\", \"Integer\", \"\", 1\n";
        fbx << "\t\tP: \"CoordAxis\", \"int\", \"Integer\", \"\", 0\n";
        fbx << "\t\tP: \"CoordAxisSign\", \"int\", \"Integer\", \"\", 1\n";
        fbx << "\t\tP: \"OriginalUpAxis\", \"int\", \"Integer\", \"\", 1\n";
        fbx << "\t\tP: \"OriginalUpAxisSign\", \"int\", \"Integer\", \"\", 1\n";
        fbx << "\t\tP: \"UnitScaleFactor\", \"double\", \"Number\", \"\", 1\n";
        fbx << "\t\tP: \"OriginalUnitScaleFactor\", \"double\", \"Number\", \"\", 1\n";
        fbx << "\t}\n}\n\n";
        fbx << "Documents:  {\n\tCount: 1\n\tDocument: 1000000000, \"\", \"Scene\" {\n";
        fbx << "\t\tProperties70:  {\n\t\t\tP: \"SourceObject\", \"object\", \"\", \"\"\n";
        std::string activeAnimName = animStacks.empty() ? "" : animStacks[0].name;
        fbx << "\t\t\tP: \"ActiveAnimStackName\", \"KString\", \"\", \"\", \"" << activeAnimName << "\"\n\t\t}\n";
        fbx << "\t\tRootNode: 0\n\t}\n}\n\n";
        fbx << "References:  {\n}\n\n";
        int defCount = 1 + (int)meshList.size() * 2 + (int)meshList.size();
        if (hasSkeleton) defCount += (int)bones.size() * 2 + (int)meshList.size() + (int)meshList.size() * (int)bones.size();
        if (!fbxTextures.empty()) defCount += (int)fbxTextures.size() * 2;
        defCount += animStackCount + animLayerCount + animCurveNodeCount + animCurveCount;
        fbx << "Definitions:  {\n\tVersion: 100\n\tCount: " << defCount << "\n";
        fbx << "\tObjectType: \"GlobalSettings\" {\n\t\tCount: 1\n\t}\n";
        fbx << "\tObjectType: \"Model\" {\n\t\tCount: " << (1 + meshList.size() + (hasSkeleton ? bones.size() : 0)) << "\n";
        fbx << "\t\tPropertyTemplate: \"FbxNode\" {\n\t\t\tProperties70:  {\n";
        fbx << "\t\t\t\tP: \"Lcl Translation\", \"Lcl Translation\", \"\", \"A\", 0,0,0\n";
        fbx << "\t\t\t\tP: \"Lcl Rotation\", \"Lcl Rotation\", \"\", \"A\", 0,0,0\n";
        fbx << "\t\t\t\tP: \"Lcl Scaling\", \"Lcl Scaling\", \"\", \"A\", 1,1,1\n";
        fbx << "\t\t\t}\n\t\t}\n\t}\n";
        fbx << "\tObjectType: \"Geometry\" {\n\t\tCount: " << meshList.size() << "\n\t}\n";
        fbx << "\tObjectType: \"Material\" {\n\t\tCount: " << meshList.size() << "\n\t}\n";
        if (!fbxTextures.empty()) {
            fbx << "\tObjectType: \"Texture\" {\n\t\tCount: " << fbxTextures.size() << "\n\t}\n";
            fbx << "\tObjectType: \"Video\" {\n\t\tCount: " << fbxTextures.size() << "\n\t}\n";
        }
        if (hasSkeleton) {
            fbx << "\tObjectType: \"Deformer\" {\n\t\tCount: " << (meshList.size() + meshList.size() * bones.size()) << "\n\t}\n";
            fbx << "\tObjectType: \"NodeAttribute\" {\n\t\tCount: " << bones.size() << "\n\t}\n";
        }
        if (animStackCount > 0) {
            fbx << "\tObjectType: \"AnimationStack\" {\n\t\tCount: " << animStackCount << "\n\t}\n";
            fbx << "\tObjectType: \"AnimationLayer\" {\n\t\tCount: " << animLayerCount << "\n\t}\n";
            fbx << "\tObjectType: \"AnimationCurveNode\" {\n\t\tCount: " << animCurveNodeCount << "\n\t}\n";
            fbx << "\tObjectType: \"AnimationCurve\" {\n\t\tCount: " << animCurveCount << "\n\t}\n";
        }
        fbx << "}\n\n";
        if (progress) progress(40, 100, "Writing objects...");
        fbx << "Objects:  {\n";
        fbx << "\tModel: " << rootId << ", \"Model::" << baseName << "\", \"Null\" {\n";
        fbx << "\t\tVersion: 232\n\t\tProperties70:  {\n";
        fbx << "\t\t\tP: \"Lcl Translation\", \"Lcl Translation\", \"\", \"A\", 0,0,0\n";
        fbx << "\t\t}\n\t\tShading: Y\n\t\tCulling: \"CullingOff\"\n\t}\n";
        for (size_t mi = 0; mi < meshList.size(); ++mi) {
            const auto& mesh = meshList[mi];
            fbx << "\tModel: " << meshIds[mi] << ", \"Model::" << mesh.name << "\", \"Mesh\" {\n";
            fbx << "\t\tVersion: 232\n\t\tProperties70:  {\n";
            fbx << "\t\t\tP: \"Lcl Translation\", \"Lcl Translation\", \"\", \"A\", 0,0,0\n";
            fbx << "\t\t}\n\t\tShading: Y\n\t\tCulling: \"CullingOff\"\n\t}\n";
        }
        if (hasSkeleton) {
            for (size_t bi = 0; bi < bones.size(); ++bi) {
                const auto& bone = bones[bi];
                std::string boneName = bone.name.empty() ? ("Bone_" + std::to_string(bi)) : bone.name;
                fbx << "\tNodeAttribute: " << boneAttrIds[bi] << ", \"NodeAttribute::" << boneName << "\", \"LimbNode\" {\n";
                fbx << "\t\tProperties70:  {\n";
                fbx << "\t\t\tP: \"Size\", \"double\", \"Number\", \"\", 0.001\n";
                fbx << "\t\t}\n";
                fbx << "\t\tTypeFlags: \"Skeleton\"\n\t}\n";
            }
            for (size_t bi = 0; bi < bones.size(); ++bi) {
                const auto& bone = bones[bi];
                std::string boneName = bone.name.empty() ? ("Bone_" + std::to_string(bi)) : bone.name;
                glm::mat4 localMat;
                if (bone.parentId >= 0 && bone.parentId < (int)bones.size()) {
                    glm::mat4 parentEffectiveInv = glm::inverse(effectiveBindGlobal[bone.parentId]);
                    localMat = parentEffectiveInv * effectiveBindGlobal[bi];
                } else {
                    localMat = effectiveBindGlobal[bi];
                }
                glm::vec3 scale, translation, skew;
                glm::quat rotation;
                glm::vec4 perspective;
                glm::decompose(localMat, scale, rotation, translation, skew, perspective);
                glm::mat3 rm = glm::mat3_cast(rotation);
                float rx = atan2(rm[1][2], rm[2][2]) * 57.2957795f;
                float ry = atan2(-rm[0][2], sqrt(rm[1][2]*rm[1][2] + rm[2][2]*rm[2][2])) * 57.2957795f;
                float rz = atan2(rm[0][1], rm[0][0]) * 57.2957795f;
                fbx << "\tModel: " << boneIds[bi] << ", \"Model::" << boneName << "\", \"LimbNode\" {\n";
                fbx << "\t\tVersion: 232\n\t\tProperties70:  {\n";
                fbx << "\t\t\tP: \"Lcl Translation\", \"Lcl Translation\", \"\", \"A\", " << FbxF(translation.x) << "," << FbxF(translation.y) << "," << FbxF(translation.z) << "\n";
                fbx << "\t\t\tP: \"Lcl Rotation\", \"Lcl Rotation\", \"\", \"A\", " << FbxF(rx) << "," << FbxF(ry) << "," << FbxF(rz) << "\n";
                fbx << "\t\t\tP: \"Lcl Scaling\", \"Lcl Scaling\", \"\", \"A\", " << FbxF(scale.x) << "," << FbxF(scale.y) << "," << FbxF(scale.z) << "\n";
                fbx << "\t\t}\n\t\tShading: Y\n\t\tCulling: \"CullingOff\"\n\t}\n";
            }
        }
        for (size_t mi = 0; mi < meshList.size(); ++mi) {
            const auto& mesh = meshList[mi];
            fbx << "\tGeometry: " << meshGeoIds[mi] << ", \"Geometry::" << mesh.name << "\", \"Mesh\" {\n";
            fbx << "\t\tVertices: *" << (mesh.positions.size() * 3) << " {\n\t\t\ta: ";
            for (size_t i = 0; i < mesh.positions.size(); ++i) {
                if (i > 0) fbx << ",";
                fbx << FbxF(mesh.positions[i].x) << "," << FbxF(mesh.positions[i].y) << "," << FbxF(mesh.positions[i].z);
            }
            fbx << "\n\t\t}\n";
            fbx << "\t\tPolygonVertexIndex: *" << mesh.indices.size() << " {\n\t\t\ta: ";
            for (size_t i = 0; i < mesh.indices.size(); i += 3) {
                if (i > 0) fbx << ",";
                fbx << mesh.indices[i] << "," << mesh.indices[i+1] << "," << (-(int)mesh.indices[i+2] - 1);
            }
            fbx << "\n\t\t}\n";
            fbx << "\t\tGeometryVersion: 124\n";
            fbx << "\t\tLayerElementNormal: 0 {\n\t\t\tVersion: 102\n\t\t\tName: \"\"\n";
            fbx << "\t\t\tMappingInformationType: \"ByVertice\"\n\t\t\tReferenceInformationType: \"Direct\"\n";
            fbx << "\t\t\tNormals: *" << (mesh.normals.size() * 3) << " {\n\t\t\t\ta: ";
            for (size_t i = 0; i < mesh.normals.size(); ++i) {
                if (i > 0) fbx << ",";
                fbx << FbxF(mesh.normals[i].x) << "," << FbxF(mesh.normals[i].y) << "," << FbxF(mesh.normals[i].z);
            }
            fbx << "\n\t\t\t}\n\t\t}\n";
            fbx << "\t\tLayerElementUV: 0 {\n\t\t\tVersion: 101\n\t\t\tName: \"UVMap\"\n";
            fbx << "\t\t\tMappingInformationType: \"ByVertice\"\n\t\t\tReferenceInformationType: \"Direct\"\n";
            fbx << "\t\t\tUV: *" << (mesh.uvs.size() * 2) << " {\n\t\t\t\ta: ";
            for (size_t i = 0; i < mesh.uvs.size(); ++i) {
                if (i > 0) fbx << ",";
                fbx << FbxF(mesh.uvs[i].x) << "," << FbxF(1.0f - mesh.uvs[i].y);
            }
            fbx << "\n\t\t\t}\n\t\t}\n";
            fbx << "\t\tLayerElementMaterial: 0 {\n\t\t\tVersion: 101\n\t\t\tName: \"\"\n";
            fbx << "\t\t\tMappingInformationType: \"AllSame\"\n\t\t\tReferenceInformationType: \"IndexToDirect\"\n";
            fbx << "\t\t\tMaterials: *1 {\n\t\t\t\ta: 0\n\t\t\t}\n\t\t}\n";
            fbx << "\t\tLayer: 0 {\n\t\t\tVersion: 100\n";
            fbx << "\t\t\tLayerElement:  {\n\t\t\t\tType: \"LayerElementNormal\"\n\t\t\t\tTypedIndex: 0\n\t\t\t}\n";
            fbx << "\t\t\tLayerElement:  {\n\t\t\t\tType: \"LayerElementUV\"\n\t\t\t\tTypedIndex: 0\n\t\t\t}\n";
            fbx << "\t\t\tLayerElement:  {\n\t\t\t\tType: \"LayerElementMaterial\"\n\t\t\t\tTypedIndex: 0\n\t\t\t}\n";
            fbx << "\t\t}\n\t}\n";
        }
        for (size_t mi = 0; mi < meshList.size(); ++mi) {
            fbx << "\tMaterial: " << materialIds[mi] << ", \"Material::Material_" << mi << "\", \"\" {\n";
            fbx << "\t\tVersion: 102\n\t\tShadingModel: \"phong\"\n\t\tMultiLayer: 0\n";
            fbx << "\t\tProperties70:  {\n";
            fbx << "\t\t\tP: \"DiffuseColor\", \"Color\", \"\", \"A\", 0.8,0.8,0.8\n";
            fbx << "\t\t\tP: \"Emissive\", \"Vector3D\", \"Vector\", \"\", 0,0,0\n";
            fbx << "\t\t\tP: \"Ambient\", \"Vector3D\", \"Vector\", \"\", 0.2,0.2,0.2\n";
            fbx << "\t\t\tP: \"Diffuse\", \"Vector3D\", \"Vector\", \"\", 0.8,0.8,0.8\n";
            fbx << "\t\t\tP: \"Specular\", \"Vector3D\", \"Vector\", \"\", 0.2,0.2,0.2\n";
            fbx << "\t\t\tP: \"Shininess\", \"double\", \"Number\", \"\", 20\n";
            fbx << "\t\t\tP: \"Opacity\", \"double\", \"Number\", \"\", 1\n";
            fbx << "\t\t}\n\t}\n";
        }
        for (const auto& tex : fbxTextures) {
            std::string b64 = Base64Encode(tex.data.data(), tex.data.size());
            std::string filename = tex.name + ".png";
            fbx << "\tVideo: " << tex.vidId << ", \"Video::" << tex.name << "\", \"Clip\" {\n";
            fbx << "\t\tType: \"Clip\"\n";
            fbx << "\t\tProperties70:  {\n";
            fbx << "\t\t\tP: \"Path\", \"KString\", \"XRefUrl\", \"\", \"" << filename << "\"\n";
            fbx << "\t\t}\n";
            fbx << "\t\tUseMipMap: 0\n";
            fbx << "\t\tFilename: \"" << filename << "\"\n";
            fbx << "\t\tRelativeFilename: \"" << filename << "\"\n";
            fbx << "\t\tContent: ,\"" << b64 << "\"\n";
            fbx << "\t}\n";
            fbx << "\tTexture: " << tex.texId << ", \"Texture::" << tex.name << "\", \"\" {\n";
            fbx << "\t\tType: \"TextureVideoClip\"\n";
            fbx << "\t\tVersion: 202\n";
            fbx << "\t\tTextureName: \"Texture::" << tex.name << "\"\n";
            fbx << "\t\tProperties70:  {\n";
            fbx << "\t\t\tP: \"UVSet\", \"KString\", \"\", \"\", \"UVMap\"\n";
            fbx << "\t\t\tP: \"UseMaterial\", \"bool\", \"\", \"\", 1\n";
            fbx << "\t\t}\n";
            fbx << "\t\tMedia: \"Video::" << tex.name << "\"\n";
            fbx << "\t\tFileName: \"" << filename << "\"\n";
            fbx << "\t\tRelativeFilename: \"" << filename << "\"\n";
            fbx << "\t\tModelUVTranslation: 0,0\n";
            fbx << "\t\tModelUVScaling: 1,1\n";
            fbx << "\t\tTexture_Alpha_Source: \"None\"\n";
            fbx << "\t\tCropping: 0,0,0,0\n";
            fbx << "\t}\n";
        }
        if (hasSkeleton) {
            if (progress) progress(50, 100, "Writing skinning...");
            for (size_t mi = 0; mi < meshList.size(); ++mi) {
                fbx << "\tDeformer: " << skinIds[mi] << ", \"Deformer::" << meshList[mi].name << "_Skin\", \"Skin\" {\n";
                fbx << "\t\tVersion: 101\n\t\tLink_DeformAcuracy: 50\n\t}\n";
            }
            for (size_t mi = 0; mi < meshList.size(); ++mi) {
                const auto& mesh = meshList[mi];
                for (size_t bi = 0; bi < bones.size(); ++bi) {
                    const auto& bone = bones[bi];
                    std::string boneName = bone.name.empty() ? ("Bone_" + std::to_string(bi)) : bone.name;
                    std::vector<int> vertIndices;
                    std::vector<double> vertWeights;
                    for (size_t vi = 0; vi < mesh.positions.size(); ++vi) {
                        const auto& j = mesh.joints[vi];
                        const auto& w = mesh.weights[vi];
                        float sum = w.x + w.y + w.z + w.w;
                        if (sum < 0.0001f) continue;
                        float wn[4] = {w.x/sum, w.y/sum, w.z/sum, w.w/sum};
                        uint32_t ji[4] = {j.x, j.y, j.z, j.w};
                        for (int k = 0; k < 4; ++k) {
                            if (ji[k] == bi && wn[k] > 0.0001f) {
                                vertIndices.push_back((int)vi);
                                vertWeights.push_back(wn[k]);
                            }
                        }
                    }
                    int64_t cid = clusterIds[mi * bones.size() + bi];
                    fbx << "\tDeformer: " << cid << ", \"SubDeformer::" << boneName << "\", \"Cluster\" {\n";
                    fbx << "\t\tVersion: 100\n\t\tUserData: \"\", \"\"\n";
                    if (!vertIndices.empty()) {
                        fbx << "\t\tIndexes: *" << vertIndices.size() << " {\n\t\t\ta: ";
                        for (size_t i = 0; i < vertIndices.size(); ++i) {
                            if (i > 0) fbx << ",";
                            fbx << vertIndices[i];
                        }
                        fbx << "\n\t\t}\n";
                        fbx << "\t\tWeights: *" << vertWeights.size() << " {\n\t\t\ta: ";
                        for (size_t i = 0; i < vertWeights.size(); ++i) {
                            if (i > 0) fbx << ",";
                            fbx << FbxF(vertWeights[i]);
                        }
                        fbx << "\n\t\t}\n";
                    }
                    glm::mat4 ibm = glm::inverse(effectiveBindGlobal[bi]);
                    fbx << "\t\tTransform: *16 {\n\t\t\ta: ";
                    for (int c = 0; c < 4; ++c) {
                        for (int r = 0; r < 4; ++r) {
                            if (c > 0 || r > 0) fbx << ",";
                            fbx << FbxF(ibm[c][r]);
                        }
                    }
                    fbx << "\n\t\t}\n";
                    fbx << "\t\tTransformLink: *16 {\n\t\t\ta: ";
                    for (int c = 0; c < 4; ++c) {
                        for (int r = 0; r < 4; ++r) {
                            if (c > 0 || r > 0) fbx << ",";
                            fbx << FbxF(effectiveBindGlobal[bi][c][r]);
                        }
                    }
                    fbx << "\n\t\t}\n\t}\n";
                }
            }
        }
        if (progress) progress(80, 100, "Writing animations...");
        for (const auto& stack : animStacks) {
            fbx << "\tAnimationStack: " << stack.id << ", \"AnimStack::" << stack.name << "\", \"\" {\n";
            fbx << "\t\tProperties70:  {\n";
            fbx << "\t\t\tP: \"LocalStart\", \"KTime\", \"Time\", \"\"," << stack.startTime << "\n";
            fbx << "\t\t\tP: \"LocalStop\", \"KTime\", \"Time\", \"\"," << stack.endTime << "\n";
            fbx << "\t\t\tP: \"ReferenceStart\", \"KTime\", \"Time\", \"\"," << stack.startTime << "\n";
            fbx << "\t\t\tP: \"ReferenceStop\", \"KTime\", \"Time\", \"\"," << stack.endTime << "\n";
            fbx << "\t\t}\n\t}\n";
            fbx << "\tAnimationLayer: " << stack.layer.id << ", \"AnimLayer::" << stack.name << "_Layer\", \"\" {\n\t}\n";
            for (const auto& node : stack.layer.curveNodes) {
                std::string boneName = bones[node.boneIdx].name.empty() ? ("Bone_" + std::to_string(node.boneIdx)) : bones[node.boneIdx].name;
                fbx << "\tAnimationCurveNode: " << node.id << ", \"AnimCurveNode::" << node.prop << "\", \"\" {\n";
                fbx << "\t\tProperties70:  {\n";
                if (node.prop == "Lcl Translation") {
                    fbx << "\t\t\tP: \"d|X\", \"Number\", \"\", \"A\",0\n";
                    fbx << "\t\t\tP: \"d|Y\", \"Number\", \"\", \"A\",0\n";
                    fbx << "\t\t\tP: \"d|Z\", \"Number\", \"\", \"A\",0\n";
                } else if (node.prop == "Lcl Rotation") {
                    fbx << "\t\t\tP: \"d|X\", \"Number\", \"\", \"A\",0\n";
                    fbx << "\t\t\tP: \"d|Y\", \"Number\", \"\", \"A\",0\n";
                    fbx << "\t\t\tP: \"d|Z\", \"Number\", \"\", \"A\",0\n";
                } else {
                    fbx << "\t\t\tP: \"d|X\", \"Number\", \"\", \"A\",1\n";
                    fbx << "\t\t\tP: \"d|Y\", \"Number\", \"\", \"A\",1\n";
                    fbx << "\t\t\tP: \"d|Z\", \"Number\", \"\", \"A\",1\n";
                }
                fbx << "\t\t}\n\t}\n";
            }
            for (const auto& curve : stack.layer.curves) {
                fbx << "\tAnimationCurve: " << curve.id << ", \"AnimCurve::\", \"\" {\n";
                fbx << "\t\tDefault: " << (curve.values.empty() ? 0.0f : curve.values[0]) << "\n";
                fbx << "\t\tKeyVer: 4008\n";
                fbx << "\t\tKeyTime: *" << curve.times.size() << " {\n\t\t\ta: ";
                for (size_t i = 0; i < curve.times.size(); ++i) {
                    if (i > 0) fbx << ",";
                    fbx << curve.times[i];
                }
                fbx << "\n\t\t}\n";
                fbx << "\t\tKeyValueFloat: *" << curve.values.size() << " {\n\t\t\ta: ";
                for (size_t i = 0; i < curve.values.size(); ++i) {
                    if (i > 0) fbx << ",";
                    fbx << FbxF(curve.values[i]);
                }
                fbx << "\n\t\t}\n";
                fbx << "\t\tKeyAttrFlags: *1 {\n\t\t\ta: 24840\n\t\t}\n";
                fbx << "\t\tKeyAttrDataFloat: *4 {\n\t\t\ta: 0,0,0,0\n\t\t}\n";
                fbx << "\t\tKeyAttrRefCount: *1 {\n\t\t\ta: " << curve.times.size() << "\n\t\t}\n";
                fbx << "\t}\n";
            }
        }
        fbx << "}\n\n";
        fbx << "Connections:  {\n";
        fbx << "\tC: \"OO\"," << rootId << ",0\n";
        for (size_t mi = 0; mi < meshList.size(); ++mi) {
            fbx << "\tC: \"OO\"," << meshIds[mi] << "," << rootId << "\n";
            fbx << "\tC: \"OO\"," << meshGeoIds[mi] << "," << meshIds[mi] << "\n";
            fbx << "\tC: \"OO\"," << materialIds[mi] << "," << meshIds[mi] << "\n";
            auto diffIt = matIdToDiffuseIdx.find(meshList[mi].materialId);
            if (diffIt != matIdToDiffuseIdx.end() && diffIt->second >= 0) {
                int texIdx = diffIt->second;
                fbx << "\tC: \"OP\"," << fbxTextures[texIdx].texId << "," << materialIds[mi] << ", \"DiffuseColor\"\n";
                fbx << "\tC: \"OO\"," << fbxTextures[texIdx].vidId << "," << fbxTextures[texIdx].texId << "\n";
            }
            auto normIt = matIdToNormalIdx.find(meshList[mi].materialId);
            if (normIt != matIdToNormalIdx.end() && normIt->second >= 0) {
                int texIdx = normIt->second;
                fbx << "\tC: \"OP\"," << fbxTextures[texIdx].texId << "," << materialIds[mi] << ", \"NormalMap\"\n";
                fbx << "\tC: \"OO\"," << fbxTextures[texIdx].vidId << "," << fbxTextures[texIdx].texId << "\n";
            }
        }
        if (hasSkeleton) {
            for (size_t bi = 0; bi < bones.size(); ++bi) {
                fbx << "\tC: \"OO\"," << boneAttrIds[bi] << "," << boneIds[bi] << "\n";
            }
            for (size_t bi = 0; bi < bones.size(); ++bi) {
                int pid = bones[bi].parentId;
                if (pid >= 0 && pid < (int)bones.size()) {
                    fbx << "\tC: \"OO\"," << boneIds[bi] << "," << boneIds[pid] << "\n";
                } else {
                    fbx << "\tC: \"OO\"," << boneIds[bi] << "," << rootId << "\n";
                }
            }
            for (size_t mi = 0; mi < meshList.size(); ++mi) {
                fbx << "\tC: \"OO\"," << skinIds[mi] << "," << meshGeoIds[mi] << "\n";
                for (size_t bi = 0; bi < bones.size(); ++bi) {
                    int64_t cid = clusterIds[mi * bones.size() + bi];
                    fbx << "\tC: \"OO\"," << cid << "," << skinIds[mi] << "\n";
                    fbx << "\tC: \"OO\"," << boneIds[bi] << "," << cid << "\n";
                }
            }
        }
        for (const auto& stack : animStacks) {
            fbx << "\tC: \"OO\"," << stack.layer.id << "," << stack.id << "\n";
            for (const auto& node : stack.layer.curveNodes) {
                fbx << "\tC: \"OO\"," << node.id << "," << stack.layer.id << "\n";
                fbx << "\tC: \"OP\"," << node.id << "," << boneIds[node.boneIdx] << ", \"" << node.prop << "\"\n";
            }
            size_t curveIdx = 0;
            for (const auto& node : stack.layer.curveNodes) {
                fbx << "\tC: \"OP\"," << stack.layer.curves[curveIdx].id << "," << node.id << ", \"d|X\"\n";
                fbx << "\tC: \"OP\"," << stack.layer.curves[curveIdx+1].id << "," << node.id << ", \"d|Y\"\n";
                fbx << "\tC: \"OP\"," << stack.layer.curves[curveIdx+2].id << "," << node.id << ", \"d|Z\"\n";
                curveIdx += 3;
            }
        }
        fbx << "}\n";
        if (!animStacks.empty()) {
            fbx << "\nTakes:  {\n\tCurrent: \"" << animStacks[0].name << "\"\n";
            for (const auto& stack : animStacks) {
                fbx << "\tTake: \"" << stack.name << "\" {\n";
                fbx << "\t\tFileName: \"" << stack.name << ".tak\"\n";
                fbx << "\t\tLocalTime: " << stack.startTime << "," << stack.endTime << "\n";
                fbx << "\t\tReferenceTime: " << stack.startTime << "," << stack.endTime << "\n";
                fbx << "\t}\n";
            }
            fbx << "}\n";
        }
        if (progress) progress(90, 100, "Writing file...");
        std::ofstream out(fbxPath);
        if (!out) { result.errorMessage = "Can't write file"; return result; }
        out << fbx.str();
        out.close();
        if (progress) progress(100, 100, "Done!");
        result.success = true;
        result.outputFile = fbxPath;
        return result;
    }
    ExportResult ExportToGLB(M3Render* render, const ArchivePtr& archive, const ExportSettings& settings, ProgressCallback progress)
    {
        ExportResult result;
        if (!render) { result.errorMessage = "No model"; return result; }
        std::string outputDir = settings.outputPath;
        if (outputDir.empty()) { result.errorMessage = "No output path"; return result; }
        std::filesystem::create_directories(outputDir);
        std::string baseName = settings.customName.empty()
            ? ExtractModelName(render->getModelName())
            : SanitizeFilename(settings.customName);
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
            std::vector<glm::uvec4> joints;
            std::vector<glm::vec4> weights;
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
            bool visible = render->getSubmeshVisible(si);
            if (!visible)
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
                    se.uvs.push_back(v.uv1);
                    se.joints.push_back(v.boneIndices);
                    se.weights.push_back(v.boneWeights);
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
        if (exportList.empty()) { result.errorMessage = "No visible submeshes"; return result; }
        result.vertexCount = static_cast<int>(totalVerts);
        result.triangleCount = static_cast<int>(totalTris);
        result.boneCount = static_cast<int>(render->getAllBones().size());
        if (progress) progress(10, 100, "Loading textures...");
        struct TexData { std::vector<uint8_t> png; size_t bufOff, bufLen; std::string name; };
        std::vector<TexData> loadedImages;
        std::unordered_map<int, int> texIdxToGltfImage;
        std::unordered_map<std::string, int> pathToGltfImage;
        auto LoadAndRegisterTexture = [&](int texIdx, const std::string& path) -> int {
            if (texIdx < 0 && path.empty()) return -1;
            std::string texPath = path;
            if (texPath.empty() && texIdx >= 0 && texIdx < (int)textures.size())
                texPath = textures[texIdx].path;
            if (texPath.empty()) return -1;
            auto pathIt = pathToGltfImage.find(texPath);
            if (pathIt != pathToGltfImage.end())
                return pathIt->second;
            auto png = LoadTextureAsPNG(archive, texPath);
            if (png.empty()) return -1;
            int imgIdx = static_cast<int>(loadedImages.size());
            std::string texName = texPath;
            size_t slash = texName.rfind('/');
            if (slash == std::string::npos) slash = texName.rfind('\\');
            if (slash != std::string::npos) texName = texName.substr(slash + 1);
            texName = SanitizeFilename(texName);
            if (texName.empty()) texName = "texture_" + std::to_string(imgIdx);
            loadedImages.push_back({std::move(png), 0, 0, texName});
            pathToGltfImage[texPath] = imgIdx;
            if (texIdx >= 0) texIdxToGltfImage[texIdx] = imgIdx;
            return imgIdx;
        };
        struct GltfMaterial {
            std::string name;
            int diffuseImage = -1;
            int normalImage = -1;
        };
        std::vector<GltfMaterial> gltfMaterials;
        std::unordered_map<uint16_t, int> matIdToGltfMat;
        if (settings.exportTextures && archive)
        {
            for (const auto& se : exportList)
            {
                if (matIdToGltfMat.count(se.materialId)) continue;
                int gltfMatIdx = -1;
                if (se.materialId < materials.size())
                {
                    const auto& mat = materials[se.materialId];
                    if (!mat.variants.empty())
                    {
                        int variantIdx = render->getMaterialSelectedVariant(se.materialId);
                        if (variantIdx < 0 || variantIdx >= (int)mat.variants.size())
                            variantIdx = 0;
                        const auto& variant = mat.variants[variantIdx];
                        int diffuseImg = LoadAndRegisterTexture(variant.textureIndexA, variant.textureColorPath);
                        int normalImg = LoadAndRegisterTexture(variant.textureIndexB, variant.textureNormalPath);
                        if (diffuseImg >= 0 || normalImg >= 0)
                        {
                            gltfMatIdx = static_cast<int>(gltfMaterials.size());
                            GltfMaterial gm;
                            gm.name = "Material_" + std::to_string(se.materialId);
                            gm.diffuseImage = diffuseImg;
                            gm.normalImage = normalImg;
                            gltfMaterials.push_back(gm);
                        }
                    }
                }
                matIdToGltfMat[se.materialId] = gltfMatIdx;
            }
        }
        result.textureCount = static_cast<int>(loadedImages.size());
        if (progress) progress(30, 100, "Building binary buffer...");
        std::vector<uint8_t> bin;
        std::vector<BufView> views;
        std::vector<Acc> accessors;
        struct MeshData { int posAcc, normAcc, uvAcc, jointsAcc, weightsAcc, idxAcc; int matIdx; std::string name; };
        std::vector<MeshData> meshes;
        const auto& bones = render->getAllBones();
        bool hasSkeleton = !bones.empty() && settings.exportSkeleton;
        std::vector<glm::mat4> effectiveBindGlobal(bones.size());
        for (size_t i = 0; i < bones.size(); ++i) {
            const auto& bone = bones[i];
            bool isRootBone = (bone.parentId < 0 || bone.parentId >= (int)bones.size());
            bool boneAtOrigin = glm::length(glm::vec3(bone.globalMatrix[3])) < 0.001f;
            if (boneAtOrigin && !bone.tracks[6].keyframes.empty()) {
                glm::vec3 track6Pos = bone.tracks[6].keyframes[0].translation;
                glm::mat4 localT = glm::translate(glm::mat4(1.0f), track6Pos);
                if (!isRootBone) {
                    effectiveBindGlobal[i] = effectiveBindGlobal[bone.parentId] * localT;
                } else {
                    effectiveBindGlobal[i] = localT;
                }
            } else {
                effectiveBindGlobal[i] = bone.globalMatrix;
            }
        }
        for (const auto& se : exportList)
        {
            MeshData md;
            md.name = se.name;
            md.matIdx = matIdToGltfMat.count(se.materialId) ? matIdToGltfMat[se.materialId] : -1;
            md.jointsAcc = -1;
            md.weightsAcc = -1;
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
            if (hasSkeleton && !se.joints.empty())
            {
                size_t jointsOff = bin.size();
                for (const auto& j : se.joints)
                {
                    uint16_t maxBone = static_cast<uint16_t>(bones.size() > 0 ? bones.size() - 1 : 0);
                    WriteU16(bin, std::min(static_cast<uint16_t>(j.x), maxBone));
                    WriteU16(bin, std::min(static_cast<uint16_t>(j.y), maxBone));
                    WriteU16(bin, std::min(static_cast<uint16_t>(j.z), maxBone));
                    WriteU16(bin, std::min(static_cast<uint16_t>(j.w), maxBone));
                }
                Pad(bin, 4);
                views.push_back({jointsOff, se.joints.size() * 8, 34962});
                md.jointsAcc = static_cast<int>(accessors.size());
                accessors.push_back({(int)views.size() - 1, 5123, (int)se.joints.size(), "VEC4", {}, {}, false});
                size_t weightsOff = bin.size();
                for (const auto& w : se.weights)
                {
                    float sum = w.x + w.y + w.z + w.w;
                    if (sum > 0.0001f)
                    {
                        WriteF32(bin, w.x / sum);
                        WriteF32(bin, w.y / sum);
                        WriteF32(bin, w.z / sum);
                        WriteF32(bin, w.w / sum);
                    }
                    else
                    {
                        WriteF32(bin, 1.0f);
                        WriteF32(bin, 0.0f);
                        WriteF32(bin, 0.0f);
                        WriteF32(bin, 0.0f);
                    }
                }
                Pad(bin, 4);
                views.push_back({weightsOff, se.weights.size() * 16, 34962});
                md.weightsAcc = static_cast<int>(accessors.size());
                accessors.push_back({(int)views.size() - 1, 5126, (int)se.weights.size(), "VEC4", {}, {}, false});
            }
            size_t idxOff = bin.size();
            for (uint32_t idx : se.indices) WriteU32(bin, idx);
            Pad(bin, 4);
            views.push_back({idxOff, se.indices.size() * 4, 34963});
            md.idxAcc = static_cast<int>(accessors.size());
            accessors.push_back({(int)views.size() - 1, 5125, (int)se.indices.size(), "SCALAR", {}, {}, false});
            meshes.push_back(md);
        }
        for (auto& t : loadedImages)
        {
            t.bufOff = bin.size();
            bin.insert(bin.end(), t.png.begin(), t.png.end());
            t.bufLen = t.png.size();
            Pad(bin, 4);
        }
        int inverseBindMatricesAcc = -1;
        if (hasSkeleton)
        {
            size_t ibmOff = bin.size();
            for (const auto& bone : bones)
            {
                glm::mat4 ibm = glm::inverse(bone.globalMatrix);
                for (int c = 0; c < 4; ++c)
                {
                    for (int r = 0; r < 4; ++r)
                    {
                        WriteF32(bin, ibm[c][r]);
                    }
                }
            }
            Pad(bin, 4);
            views.push_back({ibmOff, bones.size() * 64, 0});
            inverseBindMatricesAcc = static_cast<int>(accessors.size());
            accessors.push_back({(int)views.size() - 1, 5126, (int)bones.size(), "MAT4", {}, {}, false});
        }
        const auto& animations = render->getAllAnimations();
        bool hasAnimations = hasSkeleton && settings.exportAnimations && !animations.empty();
        struct AnimChannelData { int boneIndex; std::string path; int inputAcc; int outputAcc; };
        struct AnimData {
            std::string name;
            std::vector<AnimChannelData> channels;
        };
        std::vector<AnimData> gltfAnimations;
        if (hasAnimations)
        {
            for (size_t animIdx = 0; animIdx < animations.size(); ++animIdx)
            {
                const auto& anim = animations[animIdx];
                float startTime = anim.timestampStart / 1000.0f;
                float endTime = anim.timestampEnd / 1000.0f;
                float duration = endTime - startTime;
                if (duration <= 0.0f) continue;
                AnimData animData;
                animData.name = "Animation_" + std::to_string(anim.sequenceId);
                for (size_t boneIdx = 0; boneIdx < bones.size(); ++boneIdx)
                {
                    const auto& bone = bones[boneIdx];
                    bool boneAtOrigin = glm::length(glm::vec3(bone.globalMatrix[3])) < 0.001f;
                    const M3AnimationTrack* scaleTrack = nullptr;
                    for (int t = 0; t <= 2; ++t) {
                        if (!bone.tracks[t].keyframes.empty()) { scaleTrack = &bone.tracks[t]; break; }
                    }
                    const M3AnimationTrack* rotTrack = nullptr;
                    for (int t = 4; t <= 5; ++t) {
                        if (!bone.tracks[t].keyframes.empty()) { rotTrack = &bone.tracks[t]; break; }
                    }
                    const M3AnimationTrack* transTrack = nullptr;
                    if (!bone.tracks[6].keyframes.empty() && boneAtOrigin)
                    {
                        transTrack = &bone.tracks[6];
                    }
                    if (transTrack && !transTrack->keyframes.empty())
                    {
                        std::vector<float> times;
                        std::vector<glm::vec3> values;
                        for (const auto& kf : transTrack->keyframes)
                        {
                            float t = kf.timestamp / 1000.0f - startTime;
                            if (t >= 0.0f && t <= duration)
                            {
                                times.push_back(t);
                                values.push_back(kf.translation);
                            }
                        }
                        if (times.size() >= 2)
                        {
                            size_t timeOff = bin.size();
                            float minT = times.front(), maxT = times.back();
                            for (float t : times) WriteF32(bin, t);
                            Pad(bin, 4);
                            views.push_back({timeOff, times.size() * 4, 0});
                            int timeAcc = static_cast<int>(accessors.size());
                            accessors.push_back({(int)views.size() - 1, 5126, (int)times.size(), "SCALAR",
                                glm::vec3(minT), glm::vec3(maxT), true});
                            size_t valOff = bin.size();
                            for (const auto& v : values)
                            {
                                WriteF32(bin, v.x);
                                WriteF32(bin, v.y);
                                WriteF32(bin, v.z);
                            }
                            Pad(bin, 4);
                            views.push_back({valOff, values.size() * 12, 0});
                            int valAcc = static_cast<int>(accessors.size());
                            accessors.push_back({(int)views.size() - 1, 5126, (int)values.size(), "VEC3", {}, {}, false});
                            animData.channels.push_back({(int)boneIdx, "translation", timeAcc, valAcc});
                        }
                    }
                    if (rotTrack && !rotTrack->keyframes.empty())
                    {
                        std::vector<float> times;
                        std::vector<glm::quat> values;
                        for (const auto& kf : rotTrack->keyframes)
                        {
                            float t = kf.timestamp / 1000.0f - startTime;
                            if (t >= 0.0f && t <= duration)
                            {
                                times.push_back(t);
                                values.push_back(kf.rotation);
                            }
                        }
                        if (times.size() >= 2)
                        {
                            size_t timeOff = bin.size();
                            float minT = times.front(), maxT = times.back();
                            for (float t : times) WriteF32(bin, t);
                            Pad(bin, 4);
                            views.push_back({timeOff, times.size() * 4, 0});
                            int timeAcc = static_cast<int>(accessors.size());
                            accessors.push_back({(int)views.size() - 1, 5126, (int)times.size(), "SCALAR",
                                glm::vec3(minT), glm::vec3(maxT), true});
                            size_t valOff = bin.size();
                            for (const auto& q : values)
                            {
                                WriteF32(bin, q.x);
                                WriteF32(bin, q.y);
                                WriteF32(bin, q.z);
                                WriteF32(bin, q.w);
                            }
                            Pad(bin, 4);
                            views.push_back({valOff, values.size() * 16, 0});
                            int valAcc = static_cast<int>(accessors.size());
                            accessors.push_back({(int)views.size() - 1, 5126, (int)values.size(), "VEC4", {}, {}, false});
                            animData.channels.push_back({(int)boneIdx, "rotation", timeAcc, valAcc});
                        }
                    }
                    if (scaleTrack && !scaleTrack->keyframes.empty())
                    {
                        std::vector<float> times;
                        std::vector<glm::vec3> values;
                        for (const auto& kf : scaleTrack->keyframes)
                        {
                            float t = kf.timestamp / 1000.0f - startTime;
                            if (t >= 0.0f && t <= duration)
                            {
                                times.push_back(t);
                                values.push_back(kf.scale);
                            }
                        }
                        if (times.size() >= 2)
                        {
                            size_t timeOff = bin.size();
                            float minT = times.front(), maxT = times.back();
                            for (float t : times) WriteF32(bin, t);
                            Pad(bin, 4);
                            views.push_back({timeOff, times.size() * 4, 0});
                            int timeAcc = static_cast<int>(accessors.size());
                            accessors.push_back({(int)views.size() - 1, 5126, (int)times.size(), "SCALAR",
                                glm::vec3(minT), glm::vec3(maxT), true});
                            size_t valOff = bin.size();
                            for (const auto& v : values)
                            {
                                WriteF32(bin, v.x);
                                WriteF32(bin, v.y);
                                WriteF32(bin, v.z);
                            }
                            Pad(bin, 4);
                            views.push_back({valOff, values.size() * 12, 0});
                            int valAcc = static_cast<int>(accessors.size());
                            accessors.push_back({(int)views.size() - 1, 5126, (int)values.size(), "VEC3", {}, {}, false});
                            animData.channels.push_back({(int)boneIdx, "scale", timeAcc, valAcc});
                        }
                    }
                }
                if (!animData.channels.empty())
                {
                    gltfAnimations.push_back(std::move(animData));
                }
            }
            result.animationCount = static_cast<int>(gltfAnimations.size());
        }
        if (progress) progress(60, 100, "Building JSON...");
        std::vector<std::vector<int>> boneChildren(bones.size());
        std::vector<int> rootBones;
        if (hasSkeleton)
        {
            for (size_t i = 0; i < bones.size(); ++i)
            {
                int parentId = bones[i].parentId;
                if (parentId >= 0 && parentId < (int)bones.size())
                {
                    boneChildren[parentId].push_back(static_cast<int>(i));
                }
                else
                {
                    rootBones.push_back(static_cast<int>(i));
                }
            }
            for (int rb : rootBones)
            {
            }
        }
        int rootNode = 0;
        int firstMeshNode = 1;
        int firstBoneNode = static_cast<int>(1 + meshes.size());
        std::string json = "{\"asset\":{\"version\":\"2.0\",\"generator\":\"WildStar M3 Exporter\"},";
        json += "\"scene\":0,";
        json += "\"scenes\":[{\"name\":\"Scene\",\"nodes\":[" + std::to_string(rootNode) + "]}],";
        json += "\"nodes\":[";
        json += "{\"name\":\"" + EscapeJsonString(baseName) + "\",\"children\":[";
        for (size_t i = 0; i < meshes.size(); ++i)
        {
            if (i > 0) json += ",";
            json += std::to_string(firstMeshNode + i);
        }
        if (hasSkeleton)
        {
            for (int rb : rootBones)
            {
                json += "," + std::to_string(firstBoneNode + rb);
            }
        }
        json += "]}";
        for (size_t i = 0; i < meshes.size(); ++i)
        {
            json += ",{\"name\":\"" + EscapeJsonString(meshes[i].name) + "\",\"mesh\":" + std::to_string(i);
            if (hasSkeleton)
                json += ",\"skin\":0";
            json += "}";
        }
        if (hasSkeleton)
        {
            for (size_t i = 0; i < bones.size(); ++i)
            {
                const auto& bone = bones[i];
                json += ",{\"name\":\"" + EscapeJsonString(bone.name.empty() ? "Bone_" + std::to_string(i) : bone.name) + "\"";
                glm::mat4 localMatrix;
                if (bone.parentId >= 0 && bone.parentId < (int)bones.size())
                {
                    glm::mat4 parentEffectiveInv = glm::inverse(effectiveBindGlobal[bone.parentId]);
                    localMatrix = parentEffectiveInv * effectiveBindGlobal[i];
                }
                else
                {
                    localMatrix = effectiveBindGlobal[i];
                }
                json += ",\"matrix\":[";
                for (int c = 0; c < 4; ++c)
                {
                    for (int r = 0; r < 4; ++r)
                    {
                        if (c > 0 || r > 0) json += ",";
                        json += FloatStr(localMatrix[c][r]);
                    }
                }
                json += "]";
                if (!boneChildren[i].empty())
                {
                    json += ",\"children\":[";
                    for (size_t ci = 0; ci < boneChildren[i].size(); ++ci)
                    {
                        if (ci > 0) json += ",";
                        json += std::to_string(firstBoneNode + boneChildren[i][ci]);
                    }
                    json += "]";
                }
                json += "}";
            }
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
            json += ",\"TEXCOORD_0\":" + std::to_string(m.uvAcc);
            if (m.jointsAcc >= 0)
                json += ",\"JOINTS_0\":" + std::to_string(m.jointsAcc);
            if (m.weightsAcc >= 0)
                json += ",\"WEIGHTS_0\":" + std::to_string(m.weightsAcc);
            json += "}";
            json += ",\"indices\":" + std::to_string(m.idxAcc);
            if (m.matIdx >= 0)
                json += ",\"material\":" + std::to_string(m.matIdx);
            json += "}]}";
        }
        json += "],";
        if (hasSkeleton)
        {
            json += "\"skins\":[{\"name\":\"Armature\",\"inverseBindMatrices\":" + std::to_string(inverseBindMatricesAcc);
            json += ",\"skeleton\":" + std::to_string(rootBones.empty() ? rootNode : (firstBoneNode + rootBones[0]));
            json += ",\"joints\":[";
            for (size_t i = 0; i < bones.size(); ++i)
            {
                if (i > 0) json += ",";
                json += std::to_string(firstBoneNode + i);
            }
            json += "]}],";
        }
        if (!gltfAnimations.empty())
        {
            json += "\"animations\":[";
            for (size_t ai = 0; ai < gltfAnimations.size(); ++ai)
            {
                const auto& anim = gltfAnimations[ai];
                if (ai > 0) json += ",";
                json += "{\"name\":\"" + EscapeJsonString(anim.name) + "\"";
                json += ",\"samplers\":[";
                for (size_t ci = 0; ci < anim.channels.size(); ++ci)
                {
                    const auto& ch = anim.channels[ci];
                    if (ci > 0) json += ",";
                    json += "{\"input\":" + std::to_string(ch.inputAcc);
                    json += ",\"output\":" + std::to_string(ch.outputAcc);
                    json += ",\"interpolation\":\"LINEAR\"}";
                }
                json += "]";
                json += ",\"channels\":[";
                for (size_t ci = 0; ci < anim.channels.size(); ++ci)
                {
                    const auto& ch = anim.channels[ci];
                    if (ci > 0) json += ",";
                    json += "{\"sampler\":" + std::to_string(ci);
                    json += ",\"target\":{\"node\":" + std::to_string(firstBoneNode + ch.boneIndex);
                    json += ",\"path\":\"" + ch.path + "\"}}";
                }
                json += "]";
                json += "}";
            }
            json += "],";
        }
        if (!gltfMaterials.empty())
        {
            json += "\"materials\":[";
            for (size_t i = 0; i < gltfMaterials.size(); ++i)
            {
                const auto& mat = gltfMaterials[i];
                if (i > 0) json += ",";
                json += "{\"name\":\"" + EscapeJsonString(mat.name) + "\",\"pbrMetallicRoughness\":{";
                if (mat.diffuseImage >= 0)
                    json += "\"baseColorTexture\":{\"index\":" + std::to_string(mat.diffuseImage) + "},";
                json += "\"metallicFactor\":0,\"roughnessFactor\":1}";
                if (mat.normalImage >= 0)
                    json += ",\"normalTexture\":{\"index\":" + std::to_string(mat.normalImage) + "}";
                json += "}";
            }
            json += "],";
            json += "\"textures\":[";
            for (size_t i = 0; i < loadedImages.size(); ++i)
            {
                if (i > 0) json += ",";
                json += "{\"source\":" + std::to_string(i) + "}";
            }
            json += "],";
            json += "\"images\":[";
            for (size_t i = 0; i < loadedImages.size(); ++i)
            {
                if (i > 0) json += ",";
                int vi = static_cast<int>(views.size());
                views.push_back({loadedImages[i].bufOff, loadedImages[i].bufLen, 0});
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