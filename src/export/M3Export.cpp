#define NOMINMAX
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
        if (std::isnan(v) || std::isinf(v)) return "0";
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(6) << v;
        return oss.str();
    }

    static void WriteBoneDebugReport(const std::string& path, M3Render* render)
    {
        std::ofstream out(path);
        if (!out) return;
        const auto& bones = render->getAllBones();
        const auto& animations = render->getAllAnimations();
        out << std::fixed << std::setprecision(4);
        out << "=== Bone Debug Report ===\n";
        out << "Total bones: " << bones.size() << "\n\n";
        for (size_t i = 0; i < bones.size(); ++i) {
            const auto& bone = bones[i];
            std::string name = bone.name.empty() ? ("Bone_" + std::to_string(i)) : bone.name;
            glm::mat4 localMatrix = bone.globalMatrix;
            if (bone.parentId >= 0 && bone.parentId < (int)bones.size()) {
                localMatrix = bones[bone.parentId].inverseGlobalMatrix * bone.globalMatrix;
            }
            float localDet = glm::determinant(glm::mat3(localMatrix));
            float globalDet = glm::determinant(glm::mat3(bone.globalMatrix));
            bool mirrored = localDet < 0.0f;
            glm::vec3 globalPos = glm::vec3(bone.globalMatrix[3]);
            glm::vec3 localPos = glm::vec3(localMatrix[3]);
            glm::vec3 lScale, lTrans, lSkew; glm::quat lRot; glm::vec4 lPersp;
            glm::decompose(localMatrix, lScale, lRot, lTrans, lSkew, lPersp);
            out << "[" << i << "] \"" << name << "\" parent=" << bone.parentId
                << " mirror=" << (mirrored ? "YES" : "no ")
                << " localDet=" << localDet
                << " globalDet=" << globalDet << "\n";
            out << "     globalPos=(" << globalPos.x << "," << globalPos.y << "," << globalPos.z << ")"
                << " localPos=(" << localPos.x << "," << localPos.y << "," << localPos.z << ")\n";
            out << "     localScale=(" << lScale.x << "," << lScale.y << "," << lScale.z << ")"
                << " localRot=(w=" << lRot.w << ",x=" << lRot.x << ",y=" << lRot.y << ",z=" << lRot.z << ")\n";
            for (int t = 0; t < 8; ++t) {
                if (bone.tracks[t].keyframes.empty()) continue;
                size_t nkf = bone.tracks[t].keyframes.size();
                const auto& kf0 = bone.tracks[t].keyframes[0];
                const char* role = "?";
                if (t <= 2) role = "scale";
                else if (t == 4 || t == 5) role = "rot  ";
                else if (t == 6) role = "trans";
                out << "     track" << t << " (" << role << "): " << nkf << " kf"
                    << "  t0=" << kf0.timestamp
                    << " s=(" << kf0.scale.x << "," << kf0.scale.y << "," << kf0.scale.z << ")"
                    << " r=(w=" << kf0.rotation.w << ",x=" << kf0.rotation.x << ",y=" << kf0.rotation.y << ",z=" << kf0.rotation.z << ")"
                    << " t=(" << kf0.translation.x << "," << kf0.translation.y << "," << kf0.translation.z << ")"
                    << "\n";
            }
        }
        out << "\n=== Animations ===\n";
        for (size_t i = 0; i < animations.size(); ++i) {
            const auto& anim = animations[i];
            out << "[" << i << "] seq=" << anim.sequenceId
                << " start=" << anim.timestampStart
                << " end=" << anim.timestampEnd
                << " duration=" << (anim.timestampEnd - anim.timestampStart) << "ms\n";
        }
    }

    // =========================================================================
    // Runtime-faithful bone state helpers.
    //
    // The renderer (M3Render) does several things that the export must mirror
    // EXACTLY, otherwise the exported bind pose / animations diverge from what
    // the runtime produces. The two main subtleties are:
    //
    //   1. AT_ORIGIN bones (those whose globalMatrix has translation 0) get
    //      their bind pose reconstructed from track 6 translation + track 4/5
    //      rotation + track 0/1/2 scale, composed as T*R*S. The previous
    //      exporter only used the track 6 TRANSLATION, dropping rotation
    //      and scale - this caused incorrect bind poses for ~45 bones in
    //      typical character models.
    //
    //   2. MIRRORED bones (negative-determinant local matrix, e.g. left-side
    //      bones in a humanoid) are composed at runtime as T*S*R, NOT the
    //      standard T*R*S that glTF/FBX use. To export this correctly we
    //      must bake the runtime composition at every keyframe and decompose
    //      back into TRS that, with T*R*S order, produces the same matrix.
    // =========================================================================

    struct BoneRuntimeState {
        glm::mat4 effectiveBindGlobal{1.0f};
        glm::mat4 inverseEffectiveBindGlobal{1.0f};
        glm::mat4 bindLocalMatrix{1.0f};
        glm::vec3 bindLocalScale{1.0f};
        glm::quat bindLocalRotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 bindLocalTranslation{0.0f};
        bool boneAtOrigin = false;
        bool boneMirrored = false;
    };

    // Forward declaration - defined further down.
    static void DecomposeForExport(const glm::mat4& m, glm::vec3& outT, glm::quat& outR, glm::vec3& outS);

    // Safe matrix inverse - returns identity for singular / non-finite matrices
    // rather than propagating NaN through the rest of the pipeline.
    static glm::mat4 SafeInverse(const glm::mat4& m) {
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                if (!std::isfinite(m[c][r])) return glm::mat4(1.0f);
        float det = glm::determinant(m);
        if (!std::isfinite(det) || std::abs(det) < 1e-12f) return glm::mat4(1.0f);
        glm::mat4 inv = glm::inverse(m);
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                if (!std::isfinite(inv[c][r])) return glm::mat4(1.0f);
        return inv;
    }

    // Mirrors M3Render::precomputeBoneData() exactly. Bones must already be
    // in topological order (parents before children), which the loader guarantees.
    static void PrecomputeBoneStates(const std::vector<M3Bone>& bones,
                                     std::vector<BoneRuntimeState>& states)
    {
        size_t n = bones.size();
        states.assign(n, BoneRuntimeState{});

        // Pass 1: compute effectiveBindGlobal in hierarchy order
        for (size_t i = 0; i < n; ++i) {
            const auto& bone = bones[i];
            bool isRoot = (bone.parentId < 0 || bone.parentId >= (int)n);
            bool atOrigin = glm::length(glm::vec3(bone.globalMatrix[3])) < 0.001f;
            states[i].boneAtOrigin = atOrigin;

            if (atOrigin && !bone.tracks[6].keyframes.empty()) {
                glm::vec3 track6Pos = bone.tracks[6].keyframes[0].translation;

                float origDet = glm::determinant(glm::mat3(bone.globalMatrix));
                bool needNegativeDet = (origDet < 0.0f);

                glm::vec3 bindScale(1.0f);
                bool foundMatchingScale = false;
                for (int t = 0; t <= 2; ++t) {
                    if (bone.tracks[t].keyframes.empty()) continue;
                    glm::vec3 ts = bone.tracks[t].keyframes[0].scale;
                    bool tNeg = (ts.x * ts.y * ts.z) < 0.0f;
                    if (tNeg == needNegativeDet) {
                        bindScale = ts;
                        foundMatchingScale = true;
                        break;
                    }
                }
                if (!foundMatchingScale) {
                    for (int t = 0; t <= 2; ++t) {
                        if (bone.tracks[t].keyframes.empty()) continue;
                        bindScale = bone.tracks[t].keyframes[0].scale;
                        float scaleDet = bindScale.x * bindScale.y * bindScale.z;
                        if ((scaleDet < 0.0f) != needNegativeDet)
                            bindScale.x = -bindScale.x;
                        break;
                    }
                }

                glm::quat bindRot(1.0f, 0.0f, 0.0f, 0.0f);
                for (int t = 4; t <= 5; ++t) {
                    if (!bone.tracks[t].keyframes.empty()) {
                        bindRot = bone.tracks[t].keyframes[0].rotation;
                        // Guard against zero / degenerate quaternions in source data
                        float qL2 = bindRot.x*bindRot.x + bindRot.y*bindRot.y +
                                    bindRot.z*bindRot.z + bindRot.w*bindRot.w;
                        if (!std::isfinite(qL2) || qL2 < 1e-12f)
                            bindRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                        else
                            bindRot = bindRot * (1.0f / std::sqrt(qL2));
                        break;
                    }
                }

                glm::mat4 T = glm::translate(glm::mat4(1.0f), track6Pos);
                glm::mat4 R = glm::mat4_cast(bindRot);
                glm::mat4 S = glm::scale(glm::mat4(1.0f), bindScale);
                glm::mat4 localT = T * R * S;

                if (!isRoot)
                    states[i].effectiveBindGlobal = states[bone.parentId].effectiveBindGlobal * localT;
                else
                    states[i].effectiveBindGlobal = localT;
            } else {
                states[i].effectiveBindGlobal = bone.globalMatrix;
            }

            states[i].inverseEffectiveBindGlobal = SafeInverse(states[i].effectiveBindGlobal);
        }

        // Pass 2: bindLocalMatrix and decomposition (mirrors renderer's pass 2).
        // We use DecomposeForExport instead of glm::decompose because the latter
        // can fail (or assert in MSVC debug builds) on negative-determinant
        // matrices, which our fix actively produces for AT_ORIGIN bones with
        // mirrored scale tracks.
        for (size_t i = 0; i < n; ++i) {
            const auto& bone = bones[i];
            bool isRoot = (bone.parentId < 0 || bone.parentId >= (int)n);

            if (isRoot)
                states[i].bindLocalMatrix = states[i].effectiveBindGlobal;
            else
                states[i].bindLocalMatrix = states[bone.parentId].inverseEffectiveBindGlobal * states[i].effectiveBindGlobal;

            float det = glm::determinant(glm::mat3(states[i].bindLocalMatrix));
            states[i].boneMirrored = (det < 0);

            DecomposeForExport(states[i].bindLocalMatrix,
                               states[i].bindLocalTranslation,
                               states[i].bindLocalRotation,
                               states[i].bindLocalScale);
            // DecomposeForExport already guarantees:
            //  - rotation is a valid normalized quaternion with w >= 0
            //  - for negative-det matrices, scale.x is negated so scale's det
            //    matches the matrix's det (this is what SelectTracksForBone
            //    relies on for its determinant-matching logic)
        }
    }

    // Track interpolation - matches renderer's interpolateScale/Rotation/Translation
    static glm::vec3 InterpScaleAt(const M3AnimationTrack& tr, float tMs) {
        if (tr.keyframes.empty()) return glm::vec3(1.0f);
        if (tr.keyframes.size() == 1) return tr.keyframes[0].scale;
        if (tMs <= tr.keyframes.front().timestamp) return tr.keyframes.front().scale;
        if (tMs >= tr.keyframes.back().timestamp) return tr.keyframes.back().scale;
        size_t idx = 0;
        for (size_t i = 0; i + 1 < tr.keyframes.size(); ++i) {
            if (tr.keyframes[i].timestamp <= tMs && tr.keyframes[i+1].timestamp > tMs) { idx = i; break; }
        }
        const auto& a = tr.keyframes[idx];
        const auto& b = tr.keyframes[idx+1];
        float d = (float)(b.timestamp - a.timestamp);
        if (d <= 0.0f) return a.scale;
        float t = glm::clamp((tMs - (float)a.timestamp) / d, 0.0f, 1.0f);
        return glm::mix(a.scale, b.scale, t);
    }

    // Safe quaternion normalize: returns identity for zero / NaN inputs.
    static glm::quat SafeNormalize(const glm::quat& q) {
        float l2 = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
        if (!std::isfinite(l2) || l2 < 1e-12f)
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        float inv = 1.0f / std::sqrt(l2);
        return glm::quat(q.w * inv, q.x * inv, q.y * inv, q.z * inv);
    }

    static glm::quat InterpRotationAt(const M3AnimationTrack& tr, float tMs) {
        if (tr.keyframes.empty()) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        if (tr.keyframes.size() == 1) return SafeNormalize(tr.keyframes[0].rotation);
        if (tMs <= tr.keyframes.front().timestamp) return SafeNormalize(tr.keyframes.front().rotation);
        if (tMs >= tr.keyframes.back().timestamp) return SafeNormalize(tr.keyframes.back().rotation);
        size_t idx = 0;
        for (size_t i = 0; i + 1 < tr.keyframes.size(); ++i) {
            if (tr.keyframes[i].timestamp <= tMs && tr.keyframes[i+1].timestamp > tMs) { idx = i; break; }
        }
        const auto& a = tr.keyframes[idx];
        const auto& b = tr.keyframes[idx+1];
        float d = (float)(b.timestamp - a.timestamp);
        glm::quat q0 = SafeNormalize(a.rotation), q1 = SafeNormalize(b.rotation);
        if (d <= 0.0f) return q0;
        float t = glm::clamp((tMs - (float)a.timestamp) / d, 0.0f, 1.0f);
        if (glm::dot(q0, q1) < 0.0f) q1 = -q1;
        return SafeNormalize(glm::slerp(q0, q1, t));
    }

    static glm::vec3 InterpTranslationAt(const M3AnimationTrack& tr, float tMs) {
        if (tr.keyframes.empty()) return glm::vec3(0.0f);
        if (tr.keyframes.size() == 1) return tr.keyframes[0].translation;
        if (tMs <= tr.keyframes.front().timestamp) return tr.keyframes.front().translation;
        if (tMs >= tr.keyframes.back().timestamp) return tr.keyframes.back().translation;
        size_t idx = 0;
        for (size_t i = 0; i + 1 < tr.keyframes.size(); ++i) {
            if (tr.keyframes[i].timestamp <= tMs && tr.keyframes[i+1].timestamp > tMs) { idx = i; break; }
        }
        const auto& a = tr.keyframes[idx];
        const auto& b = tr.keyframes[idx+1];
        float d = (float)(b.timestamp - a.timestamp);
        if (d <= 0.0f) return a.translation;
        float t = glm::clamp((tMs - (float)a.timestamp) / d, 0.0f, 1.0f);
        return glm::mix(a.translation, b.translation, t);
    }

    // Per-bone selection of which tracks to use - locked once per (anim, bone)
    // so that the chosen track stays consistent across the entire animation.
    struct BoneTrackSel {
        const M3AnimationTrack* scaleTrack = nullptr;
        const M3AnimationTrack* rotTrack   = nullptr;
        const M3AnimationTrack* transTrack = nullptr;
        bool flipScaleX = false;
    };

    static BoneTrackSel SelectTracksForBone(const M3Bone& bone, const BoneRuntimeState& st)
    {
        BoneTrackSel sel;

        // Scale: prefer track with matching determinant sign vs. bind pose
        float bindScaleDet = st.bindLocalScale.x * st.bindLocalScale.y * st.bindLocalScale.z;
        bool needNegativeDet = (bindScaleDet < 0.0f);
        for (int t = 0; t <= 2; ++t) {
            if (bone.tracks[t].keyframes.empty()) continue;
            const glm::vec3& s0 = bone.tracks[t].keyframes[0].scale;
            bool tNeg = (s0.x * s0.y * s0.z) < 0.0f;
            if (tNeg == needNegativeDet) { sel.scaleTrack = &bone.tracks[t]; break; }
        }
        if (!sel.scaleTrack) {
            for (int t = 0; t <= 2; ++t) {
                if (bone.tracks[t].keyframes.empty()) continue;
                sel.scaleTrack = &bone.tracks[t];
                const glm::vec3& s0 = sel.scaleTrack->keyframes[0].scale;
                bool tNeg = (s0.x * s0.y * s0.z) < 0.0f;
                if (tNeg != needNegativeDet) sel.flipScaleX = true;
                break;
            }
        }
        // Rotation: first non-empty of track 4, 5
        for (int t = 4; t <= 5; ++t) {
            if (!bone.tracks[t].keyframes.empty()) { sel.rotTrack = &bone.tracks[t]; break; }
        }
        // Translation: track 6 only if AT_ORIGIN (matches renderer's runtime)
        if (st.boneAtOrigin && !bone.tracks[6].keyframes.empty())
            sel.transTrack = &bone.tracks[6];

        return sel;
    }

    // Compute the local matrix at time tMs using the EXACT runtime composition rule.
    // This is the heart of the fix - mirrored bones use T*S*R, others use T*R*S.
    static glm::mat4 ComputeRuntimeLocalAt(const M3Bone& bone, const BoneRuntimeState& st,
                                           const BoneTrackSel& sel, float tMs)
    {
        glm::vec3 scale = st.bindLocalScale;
        if (sel.scaleTrack) {
            scale = InterpScaleAt(*sel.scaleTrack, tMs);
            if (sel.flipScaleX) scale.x = -scale.x;
        }
        glm::quat rotation = st.bindLocalRotation;
        if (sel.rotTrack) rotation = InterpRotationAt(*sel.rotTrack, tMs);
        glm::vec3 translation = st.bindLocalTranslation;
        if (sel.transTrack) translation = InterpTranslationAt(*sel.transTrack, tMs);

        glm::mat4 T = glm::translate(glm::mat4(1.0f), translation);
        glm::mat4 R = glm::mat4_cast(SafeNormalize(rotation));
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
        return st.boneMirrored ? (T * S * R) : (T * R * S);
    }

    // Decompose a 4x4 matrix into T, R, S such that T * R * S exactly equals
    // the input. For matrices with negative determinant (mirrors), scale.x
    // is made negative and R remains a proper rotation (positive determinant).
    // This is what glTF/FBX expect for clean re-composition.
    // Robust against NaN / inf / degenerate inputs - returns identity TRS for
    // bad input rather than propagating NaN further.
    static void DecomposeForExport(const glm::mat4& m, glm::vec3& outT, glm::quat& outR, glm::vec3& outS)
    {
        // Sanity check the entire matrix for non-finite values up front.
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                if (!std::isfinite(m[c][r])) {
                    outT = glm::vec3(0.0f);
                    outR = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                    outS = glm::vec3(1.0f);
                    return;
                }
            }
        }

        outT = glm::vec3(m[3]);

        // glm matrices are column-major, so m[0], m[1], m[2] are columns.
        glm::vec3 c0(m[0]); glm::vec3 c1(m[1]); glm::vec3 c2(m[2]);

        float sx = glm::length(c0);
        float sy = glm::length(c1);
        float sz = glm::length(c2);

        const float EPS = 1e-8f;
        glm::vec3 r0, r1, r2;
        if (sx < EPS) { sx = EPS; r0 = glm::vec3(1.0f, 0.0f, 0.0f); } else r0 = c0 / sx;
        if (sy < EPS) { sy = EPS; r1 = glm::vec3(0.0f, 1.0f, 0.0f); } else r1 = c1 / sy;
        if (sz < EPS) { sz = EPS; r2 = glm::vec3(0.0f, 0.0f, 1.0f); } else r2 = c2 / sz;

        // Detect reflection - if r0,r1,r2 form a left-handed basis we have a mirror
        if (glm::dot(r0, glm::cross(r1, r2)) < 0.0f) {
            sx = -sx;
            r0 = -r0;
        }

        outS = glm::vec3(sx, sy, sz);

        glm::mat3 rotMat(r0, r1, r2);
        glm::quat q = glm::quat_cast(rotMat);
        // Defensive: if the basis was non-orthogonal due to skew/numerical error,
        // quat_cast can produce a non-unit or NaN quaternion.
        float qLen2 = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
        if (!std::isfinite(qLen2) || qLen2 < 1e-12f) {
            q = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        } else {
            q = q * (1.0f / std::sqrt(qLen2));
        }
        if (q.w < 0.0f) q = -q;
        outR = q;
    }

    // Result of baking one bone's animation. Keyframes are in MILLISECONDS
    // relative to the start of the file, matching the input track timestamps.
    struct BakedBoneAnim {
        std::vector<uint32_t> times;
        std::vector<glm::vec3> translations;
        std::vector<glm::quat> rotations;
        std::vector<glm::vec3> scales;
        bool hasTranslation = false;
        bool hasRotation    = false;
        bool hasScale       = false;
    };

    // Bake an animation for one bone. We sample at the union of all relevant
    // track keyframe times (clamped to anim range) so that we don't lose any
    // keyframe information. Mirrored bones get T*S*R applied per sample, then
    // decomposed into TRS that re-composes to the same matrix with T*R*S.
    static BakedBoneAnim BakeBoneAnimation(const M3Bone& bone, const BoneRuntimeState& st,
                                           uint32_t startMs, uint32_t endMs)
    {
        BakedBoneAnim out;
        BoneTrackSel sel = SelectTracksForBone(bone, st);
        if (!sel.scaleTrack && !sel.rotTrack && !sel.transTrack) return out;

        // Gather unique timestamps from used tracks within [startMs, endMs]
        std::vector<uint32_t> times;
        auto addTimes = [&](const M3AnimationTrack* tr) {
            if (!tr) return;
            for (const auto& kf : tr->keyframes) {
                if (kf.timestamp >= startMs && kf.timestamp <= endMs)
                    times.push_back(kf.timestamp);
            }
        };
        addTimes(sel.scaleTrack);
        addTimes(sel.rotTrack);
        addTimes(sel.transTrack);
        // Always anchor the start and end so the animation has well-defined endpoints
        times.push_back(startMs);
        times.push_back(endMs);
        std::sort(times.begin(), times.end());
        times.erase(std::unique(times.begin(), times.end()), times.end());
        if (times.size() < 2) return out;

        out.times.reserve(times.size());
        out.translations.reserve(times.size());
        out.rotations.reserve(times.size());
        out.scales.reserve(times.size());

        glm::quat prevR(1.0f, 0.0f, 0.0f, 0.0f);
        bool hasPrev = false;

        for (uint32_t t : times) {
            glm::mat4 local = ComputeRuntimeLocalAt(bone, st, sel, (float)t);
            glm::vec3 T_, S_;
            glm::quat R_;
            DecomposeForExport(local, T_, R_, S_);

            // Maintain quaternion continuity across keyframes (avoid 360 flips)
            if (hasPrev && glm::dot(prevR, R_) < 0.0f) R_ = -R_;
            prevR = R_;
            hasPrev = true;

            out.times.push_back(t);
            out.translations.push_back(T_);
            out.rotations.push_back(R_);
            out.scales.push_back(S_);
        }
        // Each bone always has all three since we baked them; the export side
        // can choose to skip channels that are constant if it wants to.
        out.hasTranslation = (sel.transTrack != nullptr);
        out.hasRotation    = (sel.rotTrack != nullptr);
        out.hasScale       = (sel.scaleTrack != nullptr);
        // For mirrored bones we MUST emit all three channels because the
        // composition order is non-standard: missing channels would let the
        // target app fall back to the node's bind TRS, which assumes T*R*S.
        if (st.boneMirrored) {
            out.hasTranslation = out.hasRotation = out.hasScale = true;
        }
        return out;
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
        std::string debugPath = outputDir + "/" + baseName + "_bones.txt";
        WriteBoneDebugReport(debugPath, render);
        const float SCALE = 100.0f;
        if (progress) progress(0, 100, "Collecting geometry...");
        const auto& allVertices = render->getVertices();
        const auto& allIndices = render->getIndices();
        const auto& submeshes = render->getAllSubmeshes();
        const auto& bones = render->getAllBones();
        const auto& materials = render->getAllMaterials();
        const auto& textures = render->getAllTextures();

        // FIX: Use runtime-faithful bone states instead of just translation
        // (was dropping rotation+scale from tracks for AT_ORIGIN bones)
        std::vector<BoneRuntimeState> boneStates;
        PrecomputeBoneStates(bones, boneStates);
        std::vector<glm::mat4> effectiveBindGlobal(bones.size());
        for (size_t i = 0; i < bones.size(); ++i)
            effectiveBindGlobal[i] = boneStates[i].effectiveBindGlobal;
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
        result.animationCount = settings.exportAnimations ? static_cast<int>(render->getAllAnimations().size()) : 0;
        bool hasSkeleton = !bones.empty() && settings.exportSkeleton;
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
        if (progress) progress(30, 100, "Preparing skeleton...");

        const auto& animations = render->getAllAnimations();
        bool hasAnimations = hasSkeleton && settings.exportAnimations && !animations.empty();

        struct FbxAnimCurve { int64_t id; std::vector<int64_t> times; std::vector<float> values; };
        struct FbxAnimCurveNode { int64_t id; std::string prop; int64_t curveX, curveY, curveZ; size_t boneIdx; };
        struct FbxAnimLayer { int64_t id; std::vector<FbxAnimCurveNode> curveNodes; std::vector<FbxAnimCurve> curves; };
        struct FbxAnimStack { int64_t id; std::string name; int64_t layerId; int64_t startTime; int64_t endTime; FbxAnimLayer layer; };
        std::vector<FbxAnimStack> animStacks;

        auto QuatToEulerXYZ = [](const glm::quat& q) -> glm::vec3 {
            glm::mat3 m = glm::mat3_cast(SafeNormalize(q));
            float rx = atan2(m[1][2], m[2][2]) * 57.2957795f;
            float ry = atan2(-m[0][2], sqrt(m[1][2]*m[1][2] + m[2][2]*m[2][2])) * 57.2957795f;
            float rz = atan2(m[0][1], m[0][0]) * 57.2957795f;
            if (!std::isfinite(rx)) rx = 0.0f;
            if (!std::isfinite(ry)) ry = 0.0f;
            if (!std::isfinite(rz)) rz = 0.0f;
            return glm::vec3(rx, ry, rz);
        };

        // Continuous unwrap of Euler angles to avoid 360-degree pops between keyframes
        auto UnwrapEuler = [](float prev, float current) -> float {
            while (current - prev >  180.0f) current -= 360.0f;
            while (current - prev < -180.0f) current += 360.0f;
            return current;
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

                    // FIX: bake the runtime composition (T*S*R for mirrored,
                    // T*R*S otherwise) at every keyframe time, then decompose
                    // for FBX (which always uses T*R*S).
                    BakedBoneAnim baked = BakeBoneAnimation(bone, boneStates[bi],
                                                            anim.timestampStart, anim.timestampEnd);
                    if (baked.times.size() < 2) continue;

                    auto emitChannel = [&](const std::string& prop, auto extractValue) {
                        FbxAnimCurveNode node;
                        node.id = GenFbxId();
                        node.prop = prop;
                        node.boneIdx = bi;
                        FbxAnimCurve cx, cy, cz;
                        cx.id = GenFbxId(); cy.id = GenFbxId(); cz.id = GenFbxId();
                        for (size_t k = 0; k < baked.times.size(); ++k) {
                            int64_t fbxTime = (int64_t)((((float)baked.times[k] - startMs) / 1000.0f) * 46186158000.0);
                            glm::vec3 v = extractValue(k);
                            cx.times.push_back(fbxTime); cx.values.push_back(v.x);
                            cy.times.push_back(fbxTime); cy.values.push_back(v.y);
                            cz.times.push_back(fbxTime); cz.values.push_back(v.z);
                        }
                        node.curveX = cx.id; node.curveY = cy.id; node.curveZ = cz.id;
                        stack.layer.curveNodes.push_back(node);
                        stack.layer.curves.push_back(std::move(cx));
                        stack.layer.curves.push_back(std::move(cy));
                        stack.layer.curves.push_back(std::move(cz));
                    };

                    if (baked.hasTranslation) {
                        emitChannel("Lcl Translation", [&](size_t k) { return baked.translations[k]; });
                    }
                    if (baked.hasRotation) {
                        // Convert each baked quaternion to Euler XYZ with continuity.
                        std::vector<glm::vec3> eulers(baked.times.size());
                        glm::vec3 prev(0.0f);
                        for (size_t k = 0; k < baked.times.size(); ++k) {
                            glm::vec3 e = QuatToEulerXYZ(baked.rotations[k]);
                            if (k > 0) {
                                e.x = UnwrapEuler(prev.x, e.x);
                                e.y = UnwrapEuler(prev.y, e.y);
                                e.z = UnwrapEuler(prev.z, e.z);
                            }
                            eulers[k] = e;
                            prev = e;
                        }
                        emitChannel("Lcl Rotation", [&](size_t k) { return eulers[k]; });
                    }
                    if (baked.hasScale) {
                        emitChannel("Lcl Scaling", [&](size_t k) { return baked.scales[k]; });
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
                    glm::mat4 parentEffectiveInv = SafeInverse(effectiveBindGlobal[bone.parentId]);
                    localMat = parentEffectiveInv * effectiveBindGlobal[bi];
                } else {
                    localMat = effectiveBindGlobal[bi];
                }
                // Use DecomposeForExport (not glm::decompose) so that mirrored
                // bind matrices (negative determinant) are handled by negating
                // X scale rather than producing NaN in the rotation quaternion.
                glm::vec3 translation, scale;
                glm::quat rotation;
                DecomposeForExport(localMat, translation, rotation, scale);
                glm::mat3 rm = glm::mat3_cast(SafeNormalize(rotation));
                float rx = atan2(rm[1][2], rm[2][2]) * 57.2957795f;
                float ry = atan2(-rm[0][2], sqrt(rm[1][2]*rm[1][2] + rm[2][2]*rm[2][2])) * 57.2957795f;
                float rz = atan2(rm[0][1], rm[0][0]) * 57.2957795f;
                if (!std::isfinite(rx)) rx = 0.0f;
                if (!std::isfinite(ry)) ry = 0.0f;
                if (!std::isfinite(rz)) rz = 0.0f;
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
                    glm::mat4 ibm = SafeInverse(effectiveBindGlobal[bi]);
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
                fbx << "\tAnimationCurveNode: " << node.id << ", \"AnimCurveNode::" << node.prop << "\", \"\" {\n";
                fbx << "\t\tProperties70:  {\n";
                if (node.prop == "Lcl Translation" || node.prop == "Lcl Rotation") {
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
        std::vector<BoneRuntimeState> boneStates;
        PrecomputeBoneStates(bones, boneStates);
        std::vector<glm::mat4> effectiveBindGlobal(bones.size());
        for (size_t i = 0; i < bones.size(); ++i) {
            effectiveBindGlobal[i] = boneStates[i].effectiveBindGlobal;
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
            for (size_t bi = 0; bi < bones.size(); ++bi)
            {
                glm::mat4 ibm = SafeInverse(effectiveBindGlobal[bi]);
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
                if (anim.timestampEnd <= anim.timestampStart) continue;
                AnimData animData;
                animData.name = "Animation_" + std::to_string(anim.sequenceId);

                for (size_t boneIdx = 0; boneIdx < bones.size(); ++boneIdx)
                {
                    const auto& bone = bones[boneIdx];
                    BakedBoneAnim baked = BakeBoneAnimation(bone, boneStates[boneIdx],
                                                            anim.timestampStart, anim.timestampEnd);
                    if (baked.times.size() < 2) continue;

                    // Build a single shared input (time) accessor for this bone+anim:
                    // all three channels share the same set of sample times.
                    std::vector<float> times;
                    times.reserve(baked.times.size());
                    for (uint32_t ms : baked.times) {
                        times.push_back((ms - anim.timestampStart) / 1000.0f);
                    }
                    float minT = times.front();
                    float maxT = times.back();

                    size_t timeOff = bin.size();
                    for (float t : times) WriteF32(bin, t);
                    Pad(bin, 4);
                    views.push_back({timeOff, times.size() * 4, 0});
                    int timeAcc = static_cast<int>(accessors.size());
                    accessors.push_back({(int)views.size() - 1, 5126, (int)times.size(), "SCALAR",
                        glm::vec3(minT), glm::vec3(maxT), true});

                    if (baked.hasTranslation)
                    {
                        size_t valOff = bin.size();
                        for (const auto& v : baked.translations)
                        {
                            WriteF32(bin, v.x);
                            WriteF32(bin, v.y);
                            WriteF32(bin, v.z);
                        }
                        Pad(bin, 4);
                        views.push_back({valOff, baked.translations.size() * 12, 0});
                        int valAcc = static_cast<int>(accessors.size());
                        accessors.push_back({(int)views.size() - 1, 5126, (int)baked.translations.size(), "VEC3", {}, {}, false});
                        animData.channels.push_back({(int)boneIdx, "translation", timeAcc, valAcc});
                    }
                    if (baked.hasRotation)
                    {
                        size_t valOff = bin.size();
                        for (const auto& q : baked.rotations)
                        {
                            WriteF32(bin, q.x);
                            WriteF32(bin, q.y);
                            WriteF32(bin, q.z);
                            WriteF32(bin, q.w);
                        }
                        Pad(bin, 4);
                        views.push_back({valOff, baked.rotations.size() * 16, 0});
                        int valAcc = static_cast<int>(accessors.size());
                        accessors.push_back({(int)views.size() - 1, 5126, (int)baked.rotations.size(), "VEC4", {}, {}, false});
                        animData.channels.push_back({(int)boneIdx, "rotation", timeAcc, valAcc});
                    }
                    if (baked.hasScale)
                    {
                        size_t valOff = bin.size();
                        for (const auto& v : baked.scales)
                        {
                            WriteF32(bin, v.x);
                            WriteF32(bin, v.y);
                            WriteF32(bin, v.z);
                        }
                        Pad(bin, 4);
                        views.push_back({valOff, baked.scales.size() * 12, 0});
                        int valAcc = static_cast<int>(accessors.size());
                        accessors.push_back({(int)views.size() - 1, 5126, (int)baked.scales.size(), "VEC3", {}, {}, false});
                        animData.channels.push_back({(int)boneIdx, "scale", timeAcc, valAcc});
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
                    glm::mat4 parentEffectiveInv = SafeInverse(effectiveBindGlobal[bone.parentId]);
                    localMatrix = parentEffectiveInv * effectiveBindGlobal[i];
                }
                else
                {
                    localMatrix = effectiveBindGlobal[i];
                }
                // glTF requires TRS (not matrix) for nodes that get animated.
                // Decompose handles negative-determinant (mirrored) matrices by
                // negating X scale and producing a proper rotation, so the TRS
                // re-composes to the same matrix under T*R*S.
                glm::vec3 nodeT, nodeS;
                glm::quat nodeR;
                DecomposeForExport(localMatrix, nodeT, nodeR, nodeS);
                json += ",\"translation\":[" + FloatStr(nodeT.x) + "," + FloatStr(nodeT.y) + "," + FloatStr(nodeT.z) + "]";
                json += ",\"rotation\":[" + FloatStr(nodeR.x) + "," + FloatStr(nodeR.y) + "," + FloatStr(nodeR.z) + "," + FloatStr(nodeR.w) + "]";
                json += ",\"scale\":[" + FloatStr(nodeS.x) + "," + FloatStr(nodeS.y) + "," + FloatStr(nodeS.z) + "]";
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
                int comps = (a.type == "SCALAR") ? 1 : (a.type == "VEC2" ? 2 : (a.type == "VEC4" ? 4 : 3));
                json += ",\"min\":[" + FloatStr(a.minV.x);
                if (comps >= 2) json += "," + FloatStr(a.minV.y);
                if (comps >= 3) json += "," + FloatStr(a.minV.z);
                json += "]";
                json += ",\"max\":[" + FloatStr(a.maxV.x);
                if (comps >= 2) json += "," + FloatStr(a.maxV.y);
                if (comps >= 3) json += "," + FloatStr(a.maxV.z);
                json += "]";
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