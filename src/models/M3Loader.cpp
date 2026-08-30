#include "M3Loader.h"
#include "M3LoaderV95.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <vector>
#include <limits>

static bool g_M3DebugBonesOnLoad = false;
static constexpr size_t kM3HeaderSize = 1584;
static constexpr uint32_t kM3Magic = 0x4D4F444Cu;
static constexpr uint32_t kM3Version = 100u;

static_assert(sizeof(M3MetaDef) == 16, "M3MetaDef must match the on-disk descriptor layout");
static_assert(sizeof(M3TrackDef) == 24, "M3TrackDef must match the on-disk track descriptor layout");

static size_t Align16(size_t value) {
    return (value + 15u) & ~size_t(15u);
}

static bool CheckedMul(size_t a, size_t b, size_t& out) {
    if (a != 0 && b > (std::numeric_limits<size_t>::max)() / a) return false;
    out = a * b;
    return true;
}

static bool CheckedAdd(size_t a, size_t b, size_t& out) {
    if (b > (std::numeric_limits<size_t>::max)() - a) return false;
    out = a + b;
    return true;
}

static bool ResolveRange(size_t fileSize, size_t base, int64_t relativeOffset, size_t byteCount, size_t& outOffset) {
    if (relativeOffset < 0) return false;

    size_t rel = static_cast<size_t>(relativeOffset);
    size_t start = 0;
    if (!CheckedAdd(base, rel, start)) return false;
    if (start > fileSize || byteCount > fileSize - start) return false;

    outOffset = start;
    return true;
}

static bool ResolveHeaderDataRange(size_t fileSize, int64_t relativeOffset, size_t byteCount, size_t& outOffset) {
    return ResolveRange(fileSize, kM3HeaderSize, relativeOffset, byteCount, outOffset);
}

static bool ResolveMetaRange(size_t fileSize, size_t base, const M3MetaDef& def,
                             size_t elementSize, size_t& outOffset) {
    size_t byteCount = 0;
    return CheckedMul(def.count, elementSize, byteCount) &&
           ResolveRange(fileSize, base, def.offset, byteCount, outOffset);
}

static bool ResolveNestedDataStart(size_t fileSize, size_t tableStart, uint32_t count,
                                   size_t recordSize, size_t& outOffset) {
    size_t tableBytes = 0;
    if (!CheckedMul(count, recordSize, tableBytes)) return false;
    if (tableBytes > (std::numeric_limits<size_t>::max)() - 15u) return false;
    return CheckedAdd(tableStart, Align16(tableBytes), outOffset) && outOffset <= fileSize;
}

template<typename T>
static bool ReadMetaVector(const uint8_t* ptr, size_t size, const M3MetaDef& def,
                           std::vector<T>& out) {
    size_t offset = 0;
    if (!ResolveMetaRange(size, kM3HeaderSize, def, sizeof(T), offset)) return false;
    out.resize(def.count);
    if (!out.empty()) std::memcpy(out.data(), ptr + offset, out.size() * sizeof(T));
    return true;
}

static bool ValidateTrackRecord(const uint8_t* ptr, size_t size, size_t base,
                                size_t trackOffset, size_t valueSize) {
    const M3TrackDef track = [&]() {
        M3TrackDef value{};
        std::memcpy(&value, ptr + trackOffset, sizeof(value));
        return value;
    }();
    size_t keyBytes = 0;
    size_t valueBytes = 0;
    size_t ignored = 0;
    return CheckedMul(track.count, sizeof(uint32_t), keyBytes) &&
           CheckedMul(track.count, valueSize, valueBytes) &&
           ResolveRange(size, base, track.offsetA, keyBytes, ignored) &&
           ResolveRange(size, base, track.offsetB, valueBytes, ignored);
}

static void DebugPrintBonesOnLoad(const M3ModelData& model) {
    if (!g_M3DebugBonesOnLoad || model.bones.empty()) return;

    std::cout << "\n===============================================================================\n";
    std::cout << "M3 BONE DEBUG - LOAD TIME (showing legs + mirrored only)\n";
    std::cout << "===============================================================================\n";
    std::cout << "Total bones: " << model.bones.size() << "\n\n";

    std::vector<bool> isMirrored(model.bones.size(), false);
    for (size_t i = 0; i < model.bones.size(); ++i) {
        glm::mat4 localMatrix = model.bones[i].globalMatrix;
        if (model.bones[i].parentId >= 0 && model.bones[i].parentId < (int)model.bones.size()) {
            localMatrix = model.bones[model.bones[i].parentId].inverseGlobalMatrix * model.bones[i].globalMatrix;
        }
        float det = glm::determinant(glm::mat3(localMatrix));
        isMirrored[i] = (det < 0);
    }

    std::cout << "--- BONE TRANSFORMS (legs 0-20 + mirrored) ---\n";
    for (size_t i = 0; i < model.bones.size(); ++i) {
        bool isLegBone = (i <= 20);
        if (!isLegBone && !isMirrored[i]) continue;

        const auto& bone = model.bones[i];

        glm::vec3 pos = glm::vec3(bone.globalMatrix[3]);

        glm::mat4 localMatrix = bone.globalMatrix;
        if (bone.parentId >= 0 && bone.parentId < (int)model.bones.size()) {
            localMatrix = model.bones[bone.parentId].inverseGlobalMatrix * bone.globalMatrix;
        }

        glm::vec3 localPos = glm::vec3(localMatrix[3]);
        float det = glm::determinant(glm::mat3(localMatrix));

        std::cout << std::fixed << std::setprecision(4);
        std::cout << "[" << std::setw(3) << i << "] " << std::setw(12) << bone.name;
        if (isMirrored[i]) std::cout << " [MIRRORED det=" << det << "]";
        std::cout << "\n";
        std::cout << "      GlobalPos: (" << std::setw(8) << pos.x << ", " << std::setw(8) << pos.y << ", " << std::setw(8) << pos.z << ")\n";
        std::cout << "      LocalPos:  (" << std::setw(8) << localPos.x << ", " << std::setw(8) << localPos.y << ", " << std::setw(8) << localPos.z << ")\n";

        bool hasAnyTrack = false;
        for (int t = 0; t < 8; ++t) {
            if (!bone.tracks[t].keyframes.empty()) hasAnyTrack = true;
        }
        if (hasAnyTrack) {
            std::cout << "      Tracks:    ";
            for (int t = 0; t < 8; ++t) {
                if (bone.tracks[t].keyframes.empty()) continue;
                const auto& track = bone.tracks[t];
                const auto& kf = track.keyframes[0];
                std::cout << track.name << "=";
                if (track.valueSize == 6) {
                    std::cout << "(" << kf.scale.x << "," << kf.scale.y << "," << kf.scale.z << ") ";
                } else if (track.valueSize == 8) {
                    std::cout << "(w=" << kf.rotation.w << ",xyz=" << kf.rotation.x << "," << kf.rotation.y << "," << kf.rotation.z << ") ";
                } else if (track.valueSize == 12) {
                    std::cout << "(" << kf.translation.x << "," << kf.translation.y << "," << kf.translation.z << ") ";
                }
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }

    std::cout << "===============================================================================\n\n";
}

template<typename T>
T M3Loader::Read(const uint8_t* data, size_t offset) {
    T val;
    std::memcpy(&val, data + offset, sizeof(T));
    return val;
}

template<typename T>
void M3Loader::ReadArray(const uint8_t* data, size_t offset, size_t count, std::vector<T>& out) {
    out.resize(count);
    std::memcpy(out.data(), data + offset, count * sizeof(T));
}

float M3Loader::HalfToFloat(uint16_t h) {
    const uint32_t bits = h;
    const uint32_t sign = (bits & 0xFFFF8000u) << 16;
    const uint32_t rest = bits & ~0x8000u;
    uint32_t resultBits = 0;

    if (rest & 0x7C00u) {
        resultBits = ((rest + 0x1C000u) << 13) | sign;
    } else {
        uint32_t mantissa = rest & 0x3FFu;
        if (mantissa != 0) {
            mantissa <<= 13;
            uint32_t exponent = 0x71u;
            while (mantissa <= 0x7FFFFFu) {
                mantissa += mantissa;
                --exponent;
            }
            resultBits = (mantissa & 0x7FFFFFu) | (exponent << 23) | sign;
        } else {
            resultBits = rest | sign;
        }
    }

    float result = 0.0f;
    std::memcpy(&result, &resultBits, sizeof(result));
    return result;
}

float M3Loader::Int16ToFloat(int16_t v) {
    return static_cast<float>(v) * (1.0f / 16384.0f);
}

glm::vec3 M3Loader::ReadVertexV3(const uint8_t* data, uint8_t type, size_t& offset) {
    glm::vec3 res(0.0f);
    if (type == 1) {
        std::memcpy(&res, data + offset, 12);
        offset += 12;
    } else if (type == 2) {
        int16_t xyz[3];
        std::memcpy(xyz, data + offset, 6);
        res = glm::vec3(xyz[0], xyz[1], xyz[2]) / 1024.0f;
        offset += 6;
    } else if (type == 3) {
        uint8_t x = data[offset];
        uint8_t y = data[offset + 1];
        float fx = (static_cast<float>(x) / 255.0f) * 2.0f - 1.0f;
        float fy = (static_cast<float>(y) / 255.0f) * 2.0f - 1.0f;
        float fz = 1.0f - std::abs(fx) - std::abs(fy);

        if (fz < 0.0f) {
            float oldX = fx;
            float oldY = fy;
            fx = (1.0f - std::abs(oldY)) * (oldX >= 0.0f ? 1.0f : -1.0f);
            fy = (1.0f - std::abs(oldX)) * (oldY >= 0.0f ? 1.0f : -1.0f);
        }

        glm::vec3 decoded(fx, fy, fz);
        float len2 = glm::dot(decoded, decoded);
        res = len2 > 0.000001f ? decoded / std::sqrt(len2) : glm::vec3(0.0f, 0.0f, 1.0f);
        offset += 2;
    }
    return res;
}

glm::vec4 M3Loader::ReadVertexV4(const uint8_t* data, uint8_t type, size_t& offset) {
    glm::vec4 res(1.0f);
    if (type == 4) {
        res.x = data[offset];
        res.y = data[offset + 1];
        res.z = data[offset + 2];
        res.w = data[offset + 3];
        offset += 4;
    }
    return res;
}

glm::vec2 M3Loader::ReadVertexV2(const uint8_t* data, uint8_t type, size_t& offset) {
    glm::vec2 res(0.0f);
    if (type == 5) {
        uint16_t xy[2];
        std::memcpy(xy, data + offset, 4);
        res.x = HalfToFloat(xy[0]);
        res.y = HalfToFloat(xy[1]);
        offset += 4;
    }
    return res;
}

M3ModelData M3Loader::LoadFromFile(const ArchivePtr& arc, const std::shared_ptr<FileEntry>& entry) {
    if (!arc || !entry) return {};
    std::vector<uint8_t> buffer;
    arc->getFileData(entry, buffer);
    if (buffer.empty()) return {};

    if (buffer.size() >= 8) {
        uint32_t version = 0;
        std::memcpy(&version, buffer.data() + 4, sizeof(version));

        if (version >= 90 && version < 100) {
            return M3LoaderV95::Load(buffer);
        }
    }

    return Load(buffer);
}

bool M3Loader::ReadHeader(const uint8_t* ptr, size_t size, M3Header& h) {
    if (size < HEADER_SIZE) return false;

    std::memcpy(h.signature, ptr, 4);
    h.version = Read<uint32_t>(ptr, 4);

    if (Read<uint32_t>(ptr, 0) != kM3Magic || h.version != kM3Version) return false;

    h.unk008 = Read<uint64_t>(ptr, 8);

    h.animationsMeta = Read<M3MetaDef>(ptr, 0x010);
    for (int i = 0; i < 4; ++i) h.trackdefAnim[i] = Read<M3TrackDef>(ptr, 0x020 + i * 0x18);
    h.struct080 = Read<M3MetaDef>(ptr, 0x080);
    h.trackdef090 = Read<M3TrackDef>(ptr, 0x090);
    h.trackdef0A8 = Read<M3TrackDef>(ptr, 0x0A8);
    h.trackdef0C0 = Read<M3TrackDef>(ptr, 0x0C0);
    h.trackdef0D8 = Read<M3TrackDef>(ptr, 0x0D8);
    h.struct0F0 = Read<M3MetaDef>(ptr, 0x0F0);
    h.trackdef100 = Read<M3TrackDef>(ptr, 0x100);
    h.trackdef118 = Read<M3TrackDef>(ptr, 0x118);
    h.trackdef130 = Read<M3TrackDef>(ptr, 0x130);
    h.trackdef148 = Read<M3TrackDef>(ptr, 0x148);
    h.trackdef160 = Read<M3TrackDef>(ptr, 0x160);
    h.unkFloat178 = Read<float>(ptr, 0x178);
    h.bones = Read<M3MetaDef>(ptr, 0x180);
    h.lut190 = Read<M3MetaDef>(ptr, 0x190);
    h.lut1A0 = Read<M3MetaDef>(ptr, 0x1A0);
    h.lutBoneIds = Read<M3MetaDef>(ptr, 0x1B0);
    h.textures = Read<M3MetaDef>(ptr, 0x1C0);
    h.lut1D0 = Read<M3MetaDef>(ptr, 0x1D0);
    h.struct1E0 = Read<M3MetaDef>(ptr, 0x1E0);
    h.materials = Read<M3MetaDef>(ptr, 0x1F0);
    h.submeshIds = Read<M3MetaDef>(ptr, 0x200);
    h.lut210 = Read<M3MetaDef>(ptr, 0x210);
    h.geometry = Read<M3MetaDef>(ptr, 0x250);
    h.lut260 = Read<M3MetaDef>(ptr, 0x260);
    h.lut270 = Read<M3MetaDef>(ptr, 0x270);
    h.lut280 = Read<M3MetaDef>(ptr, 0x280);
    h.trackdef290 = Read<M3TrackDef>(ptr, 0x290);
    h.struct2B8 = Read<M3MetaDef>(ptr, 0x2B8);
    h.lut2C8 = Read<M3MetaDef>(ptr, 0x2C8);
    h.struct2F8 = Read<M3MetaDef>(ptr, 0x2F8);
    h.struct308 = Read<M3MetaDef>(ptr, 0x308);
    h.lights = Read<M3MetaDef>(ptr, 0x318);
    h.struct328 = Read<M3MetaDef>(ptr, 0x328);
    h.lut338 = Read<M3MetaDef>(ptr, 0x338);
    h.idUnk348 = Read<int64_t>(ptr, 0x348);
    h.trackdef350 = Read<M3TrackDef>(ptr, 0x350);
    h.idUnk368 = Read<int64_t>(ptr, 0x368);
    h.trackdef370 = Read<M3TrackDef>(ptr, 0x370);
    std::memcpy(h.floatUnk380, ptr + 0x380, sizeof(h.floatUnk380));
    h.struct490 = Read<M3MetaDef>(ptr, 0x490);
    h.lut4A0 = Read<M3MetaDef>(ptr, 0x4A0);
    h.floatUnk4F0 = Read<float>(ptr, 0x4F0);
    h.floatUnk4F8 = Read<float>(ptr, 0x4F8);
    std::memcpy(&h.posUnk500, ptr + 0x500, 12);
    h.lut510 = Read<M3MetaDef>(ptr, 0x510);
    h.lut520 = Read<M3MetaDef>(ptr, 0x520);
    h.lut530 = Read<M3MetaDef>(ptr, 0x530);
    h.struct540 = Read<M3MetaDef>(ptr, 0x540);
    h.lut550 = Read<M3MetaDef>(ptr, 0x550);
    h.struct560 = Read<M3MetaDef>(ptr, 0x560);
    h.struct570 = Read<M3MetaDef>(ptr, 0x570);
    h.idUnk580 = Read<int64_t>(ptr, 0x580);
    h.struct588 = Read<M3MetaDef>(ptr, 0x588);
    h.customBoneMinMax = Read<M3MetaDef>(ptr, 0x598);
    h.lutBoneToCustom = Read<M3MetaDef>(ptr, 0x5A8);
    h.trackdef5C0 = Read<M3TrackDef>(ptr, 0x5C0);

    return true;
}

bool M3Loader::ReadLUTs(const uint8_t* ptr, size_t size, M3ModelData& model) {
    const auto& h = model.header;

    return ReadMetaVector(ptr, size, h.lut190, model.lut190) &&
           ReadMetaVector(ptr, size, h.lut1A0, model.lut1A0) &&
           ReadMetaVector(ptr, size, h.lutBoneIds, model.lutBoneMapping) &&
           ReadMetaVector(ptr, size, h.lut1D0, model.lut1D0) &&
           ReadMetaVector(ptr, size, h.lut210, model.lut210) &&
           ReadMetaVector(ptr, size, h.lut260, model.lut260) &&
           ReadMetaVector(ptr, size, h.lut270, model.lut270) &&
           ReadMetaVector(ptr, size, h.lut280, model.lut280) &&
           ReadMetaVector(ptr, size, h.lut338, model.lut338) &&
           ReadMetaVector(ptr, size, h.lut4A0, model.lut4A0) &&
           ReadMetaVector(ptr, size, h.lut510, model.lut510) &&
           ReadMetaVector(ptr, size, h.lut520, model.lut520) &&
           ReadMetaVector(ptr, size, h.lut530, model.lut530) &&
           ReadMetaVector(ptr, size, h.lut550, model.lut550);
}

bool M3Loader::ReadTextures(const uint8_t* ptr, size_t size, M3ModelData& model) {
    const auto& h = model.header;
    size_t tableStart = 0;
    if (!ResolveMetaRange(size, HEADER_SIZE, h.textures, TEX_ENTRY_SIZE, tableStart)) return false;

    size_t dataStart = 0;
    if (!ResolveNestedDataStart(size, tableStart, h.textures.count, TEX_ENTRY_SIZE, dataStart)) return false;

    model.textures.resize(h.textures.count);
    for (uint32_t i = 0; i < h.textures.count; ++i) {
        size_t ofs = tableStart + i * TEX_ENTRY_SIZE;

        auto& tex = model.textures[i];
        tex.slotId = Read<uint16_t>(ptr, ofs);
        tex.fallbackType = Read<uint16_t>(ptr, ofs + 2);
        tex.flags = Read<int32_t>(ptr, ofs + 4);
        tex.intensity = Read<float>(ptr, ofs + 8);
        tex.unk4 = ptr[ofs + 12];
        tex.unk5 = ptr[ofs + 13];
        tex.unk6 = ptr[ofs + 14];
        tex.unk7 = ptr[ofs + 15];
        tex.nrLetters = Read<uint32_t>(ptr, ofs + 16);
        tex.pathFlags = Read<uint32_t>(ptr, ofs + 20);
        const int64_t pathOffset = Read<int64_t>(ptr, ofs + 24);
        if (pathOffset < 0) return false;
        tex.offset = static_cast<uint64_t>(pathOffset);

        size_t byteLen = 0;
        size_t strPos = 0;
        if (!CheckedMul(tex.nrLetters, sizeof(char16_t), byteLen) ||
            !ResolveRange(size, dataStart, pathOffset, byteLen, strPos)) return false;
        tex.path.reserve(tex.nrLetters);
        for (uint32_t j = 0; j < tex.nrLetters; ++j) {
            char16_t c = 0;
            std::memcpy(&c, ptr + strPos + j * sizeof(char16_t), sizeof(c));
            if (c == 0) break;
            tex.path.push_back((c < 128) ? static_cast<char>(c) : '_');
        }

        if (tex.fallbackType == 0) tex.textureType = "color";
        else if (tex.fallbackType == 1) tex.textureType = "normal";
        else if (tex.fallbackType == 2) tex.textureType = "special";
        else tex.textureType = "unknown";
    }

    return true;
}

bool M3Loader::ReadMaterials(const uint8_t* ptr, size_t size, M3ModelData& model) {
    const auto& h = model.header;
    size_t tableStart = 0;
    if (!ResolveMetaRange(size, HEADER_SIZE, h.materials, MAT_ENTRY_SIZE, tableStart)) return false;

    size_t dataStart = 0;
    if (!ResolveNestedDataStart(size, tableStart, h.materials.count, MAT_ENTRY_SIZE, dataStart)) return false;

    model.materials.resize(h.materials.count);
    for (uint32_t i = 0; i < h.materials.count; ++i) {
        size_t ofs = tableStart + i * MAT_ENTRY_SIZE;

        auto& mat = model.materials[i];
        mat.unk0 = ptr[ofs]; mat.unk1 = ptr[ofs+1]; mat.unk2 = ptr[ofs+2]; mat.unk3 = ptr[ofs+3];
        mat.unk4 = ptr[ofs+4]; mat.unk5 = ptr[ofs+5]; mat.unk6 = ptr[ofs+6]; mat.unk7 = ptr[ofs+7];
        mat.unk8 = ptr[ofs+8]; mat.unk9 = ptr[ofs+9]; mat.unk10 = ptr[ofs+10]; mat.unk11 = ptr[ofs+11];
        mat.unk12 = Read<uint16_t>(ptr, ofs + 12);
        mat.unk14 = Read<uint16_t>(ptr, ofs + 14);
        mat.unk16 = Read<uint32_t>(ptr, ofs + 16);
        mat.unk20 = Read<uint32_t>(ptr, ofs + 20);
        mat.specularX = Read<int32_t>(ptr, ofs + 24);
        mat.specularY = Read<int32_t>(ptr, ofs + 28);
        mat.nrDescriptions = Read<uint32_t>(ptr, ofs + 32);
        mat.descriptionFlags = Read<uint32_t>(ptr, ofs + 36);
        const int64_t descriptionsOffset = Read<int64_t>(ptr, ofs + 40);
        if (descriptionsOffset < 0) return false;
        mat.ofsDescriptions = static_cast<uint64_t>(descriptionsOffset);

        size_t descriptionBytes = 0;
        size_t descBase = 0;
        if (!CheckedMul(mat.nrDescriptions, MAT_DESC_SIZE, descriptionBytes) ||
            !ResolveRange(size, dataStart, descriptionsOffset, descriptionBytes, descBase)) return false;

        size_t descriptionDataStart = 0;
        if (descriptionBytes > (std::numeric_limits<size_t>::max)() - 15u ||
            !CheckedAdd(descBase, Align16(descriptionBytes), descriptionDataStart) ||
            descriptionDataStart > size) return false;

        if (mat.nrDescriptions > 0) {
            mat.variants.resize(mat.nrDescriptions);
            for (uint32_t d = 0; d < mat.nrDescriptions; ++d) {
                size_t descOfs = descBase + d * MAT_DESC_SIZE;

                static constexpr size_t kTrackOffsets[] = {
                    0x18, 0x30, 0x48, 0x60, 0x78, 0xA8, 0xC0, 0xD8, 0xF0, 0x108
                };
                for (size_t t = 0; t < std::size(kTrackOffsets); ++t) {
                    const size_t valueSize = t == std::size(kTrackOffsets) - 1 ? 12u : 4u;
                    if (!ValidateTrackRecord(ptr, size, descriptionDataStart,
                                             descOfs + kTrackOffsets[t], valueSize)) return false;
                }

                auto& var = mat.variants[d];
                var.textureIndexA = Read<int16_t>(ptr, descOfs);
                var.textureIndexB = Read<int16_t>(ptr, descOfs + 2);
                std::memcpy(var.unkValues.data(), ptr + descOfs + 4, 292);

                if (var.textureIndexA >= 0 && var.textureIndexA < (int)model.textures.size()) {
                    auto& tex = model.textures[var.textureIndexA];
                    size_t dotPos = tex.path.find('.');
                    var.textureColorPath = (dotPos != std::string::npos) ?
                        tex.path.substr(0, dotPos) + ".tex" : tex.path + ".tex";
                }
                if (var.textureIndexB >= 0 && var.textureIndexB < (int)model.textures.size()) {
                    auto& tex = model.textures[var.textureIndexB];
                    size_t dotPos = tex.path.find('.');
                    var.textureNormalPath = (dotPos != std::string::npos) ?
                        tex.path.substr(0, dotPos) + ".tex" : tex.path + ".tex";
                }
            }
        }

        if (mat.variants.empty()) {
            mat.variants.push_back(M3MaterialVariant{});
        }
    }

    return true;
}

bool M3Loader::ReadBoneAnimationTrack(const uint8_t* ptr, size_t size, size_t animStart, M3AnimationTrack& track) {
    if (track.valueSize != 6 && track.valueSize != 8 && track.valueSize != 12) return false;

    size_t timeBytes = 0;
    size_t valueBytes = 0;
    size_t timeOfs = 0;
    size_t valOfs = 0;
    if (!CheckedMul(track.duration, sizeof(uint32_t), timeBytes) ||
        !CheckedMul(track.duration, track.valueSize, valueBytes) ||
        !ResolveRange(size, animStart, track.timeOffset, timeBytes, timeOfs) ||
        !ResolveRange(size, animStart, track.valueOffset, valueBytes, valOfs)) {
        return false;
    }

    if (track.duration == 0) return true;

    track.keyframes.resize(track.duration);

    for (uint32_t i = 0; i < track.duration; i++) {
        track.keyframes[i].timestamp = Read<uint32_t>(ptr, timeOfs + i * sizeof(uint32_t));
    }

    switch (track.valueSize) {
    case 6:
        for (uint32_t i = 0; i < track.duration; i++) {
            uint16_t h[3];
            std::memcpy(h, ptr + valOfs + i * 6, 6);
            track.keyframes[i].scale = glm::vec3(HalfToFloat(h[0]), HalfToFloat(h[1]), HalfToFloat(h[2]));
        }
        break;
    case 8:
        for (uint32_t i = 0; i < track.duration; i++) {
            int16_t q[4];
            std::memcpy(q, ptr + valOfs + i * 8, 8);
            track.keyframes[i].rotation = glm::quat(
                Int16ToFloat(q[3]), Int16ToFloat(q[0]), Int16ToFloat(q[1]), Int16ToFloat(q[2])
            );
        }
        break;
    case 12:
        for (uint32_t i = 0; i < track.duration; i++) {
            std::memcpy(&track.keyframes[i].translation, ptr + valOfs + i * 12, 12);
        }
        break;
    default:
        return false;
    }

    return true;
}

bool M3Loader::ReadBones(const uint8_t* ptr, size_t size, M3ModelData& model) {
    const auto& h = model.header;

    static constexpr uint8_t kBoneTrackValueSizes[8] = {6, 6, 6, 6, 8, 8, 12, 12};
    static constexpr const char* kBoneTrackNames[8] = {
        "scale",
        "scaleSecondary",
        "scaleDivisor",
        "scaleSecondaryDivisor",
        "rotation",
        "rotationSecondary",
        "translation",
        "translationSecondary"
    };

    size_t tableBytes = 0;
    size_t tableStart = 0;
    if (!CheckedMul(h.bones.count, BONE_SIZE, tableBytes) ||
        !ResolveHeaderDataRange(size, h.bones.offset, tableBytes, tableStart)) {
        return false;
    }

    size_t animStart = 0;
    if (!CheckedAdd(tableStart, Align16(tableBytes), animStart) || animStart > size) {
        return false;
    }

    model.bones.resize(h.bones.count);
    for (uint32_t i = 0; i < h.bones.count; ++i) {
        size_t ofs = tableStart + i * BONE_SIZE;

        auto& bone = model.bones[i];
        bone.id = static_cast<int32_t>(i);
        bone.name = "Bone_" + std::to_string(i);

        bone.globalId = static_cast<int16_t>(Read<uint16_t>(ptr, ofs));
        bone.flags = Read<uint16_t>(ptr, ofs + 2);
        const uint16_t parentId = Read<uint16_t>(ptr, ofs + 4);
        bone.parentId = parentId == 0xFFFFu ? -1 : static_cast<int16_t>(parentId);
        bone.unk01 = static_cast<int16_t>(Read<uint16_t>(ptr, ofs + 6));
        bone.unk02 = ptr[ofs + 8];
        bone.unk03 = ptr[ofs + 9];
        bone.unk04 = ptr[ofs + 10];
        bone.unk05 = ptr[ofs + 11];
        bone.unk06 = Read<uint32_t>(ptr, ofs + 12);

        size_t trackOfs = ofs + 16;
        for (int t = 0; t < 8; t++) {
            bone.tracks[t].duration = Read<uint32_t>(ptr, trackOfs);
            bone.tracks[t].flags = Read<uint32_t>(ptr, trackOfs + 4);
            bone.tracks[t].timeOffset = Read<int64_t>(ptr, trackOfs + 8);
            bone.tracks[t].valueOffset = Read<int64_t>(ptr, trackOfs + 16);
            bone.tracks[t].valueSize = kBoneTrackValueSizes[t];
            bone.tracks[t].trackType = t;
            bone.tracks[t].name = kBoneTrackNames[t];
            trackOfs += 24;
        }

        size_t matOfs = ofs + 0xD0;
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                bone.globalMatrix[r][c] = Read<float>(ptr, matOfs + (r * 4 + c) * 4);
            }
        }

        matOfs = ofs + 0x110;
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                bone.inverseGlobalMatrix[r][c] = Read<float>(ptr, matOfs + (r * 4 + c) * 4);
            }
        }

        std::memcpy(&bone.position, ptr + ofs + 0x150, 12);

        for (int t = 0; t < 8; t++) {
            if (!ReadBoneAnimationTrack(ptr, size, animStart, bone.tracks[t])) return false;
        }
    }

    BuildBonePaths(model);
    return true;
}

void M3Loader::FixMirroredBones(M3ModelData& model) {
    std::vector<int> mirroredBoneList;
    std::vector<int> mirroredAnimList;

    for (size_t i = 0; i < model.bones.size(); ++i) {
        auto& bone = model.bones[i];

        glm::mat4 localMatrix = bone.globalMatrix;
        if (bone.parentId >= 0 && bone.parentId < (int)model.bones.size()) {
            localMatrix = model.bones[bone.parentId].inverseGlobalMatrix * bone.globalMatrix;
        }

        float det = glm::determinant(glm::mat3(localMatrix));
        if (det < 0) {
            mirroredBoneList.push_back((int)i);
        }

        if (!bone.tracks[0].keyframes.empty() && bone.tracks[0].keyframes[0].scale.x < 0) {
            mirroredAnimList.push_back((int)i);
        }
        if (!bone.tracks[1].keyframes.empty() && bone.tracks[1].keyframes[0].scale.x < 0) {
            mirroredAnimList.push_back((int)i);
        }
    }

    std::sort(mirroredAnimList.begin(), mirroredAnimList.end());
    mirroredAnimList.erase(std::unique(mirroredAnimList.begin(), mirroredAnimList.end()), mirroredAnimList.end());

    for (int boneId : mirroredAnimList) {
        bool isMirroredBone = std::find(mirroredBoneList.begin(), mirroredBoneList.end(), boneId) != mirroredBoneList.end();
        if (!isMirroredBone) {
            auto& bone = model.bones[boneId];
            for (auto& kf : bone.tracks[0].keyframes) {
                kf.scale.x *= -1;
                kf.scale.y *= -1;
                kf.scale.z *= -1;
            }
        }
    }
}

void M3Loader::BuildBonePaths(M3ModelData& model) {
    for (auto& bone : model.bones) {
        if (bone.parentId < 0) {
            bone.parentPath = bone.name;
        } else if (bone.parentId < (int)model.bones.size()) {
            bone.parentPath = model.bones[bone.parentId].parentPath + "/" + bone.name;
        }
    }
}

static bool KeyFrameValueDiffers(const M3AnimationTrack& track, const M3KeyFrame& a, const M3KeyFrame& b) {
    constexpr float kVecEpsilon = 0.0001f;
    constexpr float kQuatDotEpsilon = 0.9999f;

    if (track.trackType >= 4 && track.trackType <= 5) {
        return std::abs(glm::dot(glm::normalize(a.rotation), glm::normalize(b.rotation))) < kQuatDotEpsilon;
    }

    const glm::vec3& av = (track.trackType >= 6) ? a.translation : a.scale;
    const glm::vec3& bv = (track.trackType >= 6) ? b.translation : b.scale;
    return std::abs(av.x - bv.x) > kVecEpsilon ||
           std::abs(av.y - bv.y) > kVecEpsilon ||
           std::abs(av.z - bv.z) > kVecEpsilon;
}

static bool TrackHasAnimatedKeysInRange(const M3AnimationTrack& track, uint32_t startTime, uint32_t endTime) {
    if (track.keyframes.size() < 2 || endTime <= startTime) return false;

    const M3KeyFrame* firstInRange = nullptr;
    for (const auto& keyframe : track.keyframes) {
        if (keyframe.timestamp < startTime || keyframe.timestamp > endTime) continue;

        if (!firstInRange) {
            firstInRange = &keyframe;
            continue;
        }

        if (KeyFrameValueDiffers(track, *firstInRange, keyframe)) {
            return true;
        }
    }

    const M3KeyFrame* beforeOrAtStart = nullptr;
    const M3KeyFrame* afterOrAtEnd = nullptr;
    for (const auto& keyframe : track.keyframes) {
        if (keyframe.timestamp <= startTime) {
            beforeOrAtStart = &keyframe;
        }
        if (keyframe.timestamp >= endTime) {
            afterOrAtEnd = &keyframe;
            break;
        }
    }

    return beforeOrAtStart &&
           afterOrAtEnd &&
           beforeOrAtStart != afterOrAtEnd &&
           KeyFrameValueDiffers(track, *beforeOrAtStart, *afterOrAtEnd);
}

static bool AnimationHasAnimatedBoneTracks(const M3ModelData& model, const M3ModelAnimation& animation) {
    for (const auto& bone : model.bones) {
        for (const auto& track : bone.tracks) {
            if (TrackHasAnimatedKeysInRange(track, animation.timestampStart, animation.timestampEnd)) {
                return true;
            }
        }
    }
    return false;
}

static void MarkAnimatedSequences(M3ModelData& model) {
    for (auto& animation : model.animations) {
        animation.hasAnimatedTracks = AnimationHasAnimatedBoneTracks(model, animation);
    }
}

bool M3Loader::ReadAnimations(const uint8_t* ptr, size_t size, M3ModelData& model) {
    const auto& h = model.header;
    size_t tableStart = 0;
    if (!ResolveMetaRange(size, HEADER_SIZE, h.animationsMeta, ANIMATION_SIZE, tableStart)) return false;

    size_t nestedStart = 0;
    if (!ResolveNestedDataStart(size, tableStart, h.animationsMeta.count,
                                ANIMATION_SIZE, nestedStart)) return false;

    model.animations.resize(h.animationsMeta.count);
    for (uint32_t i = 0; i < h.animationsMeta.count; ++i) {
        size_t ofs = tableStart + i * ANIMATION_SIZE;

        const M3MetaDef sequenceData = Read<M3MetaDef>(ptr, ofs + 0x60);
        size_t ignored = 0;
        if (!ResolveMetaRange(size, nestedStart, sequenceData, sizeof(uint16_t), ignored)) return false;

        auto& anim = model.animations[i];
        anim.sequenceId = Read<uint16_t>(ptr, ofs);
        anim.unk1 = Read<uint16_t>(ptr, ofs + 2);
        anim.unk2 = Read<uint16_t>(ptr, ofs + 4);
        anim.unk3 = Read<uint16_t>(ptr, ofs + 6);
        anim.unk4 = Read<uint16_t>(ptr, ofs + 8);
        anim.fallbackSequence = Read<uint16_t>(ptr, ofs + 10);
        anim.timestampStart = Read<uint32_t>(ptr, ofs + 12);
        anim.timestampEnd = Read<uint32_t>(ptr, ofs + 16);
        anim.unk10 = Read<uint16_t>(ptr, ofs + 20);
        anim.unk11 = Read<uint16_t>(ptr, ofs + 22);
        anim.unk12 = Read<uint16_t>(ptr, ofs + 24);
        anim.unk13 = Read<uint16_t>(ptr, ofs + 26);
        anim.unk14 = Read<uint16_t>(ptr, ofs + 28);
        anim.unk15 = Read<uint16_t>(ptr, ofs + 30);
        std::memcpy(&anim.bound1, ptr + ofs + 32, 12);
        anim.unk19 = Read<uint32_t>(ptr, ofs + 44);
        std::memcpy(&anim.bound2, ptr + ofs + 48, 12);
        anim.unk23 = Read<uint32_t>(ptr, ofs + 60);
        std::memcpy(&anim.bound3, ptr + ofs + 64, 12);
        anim.unk25 = Read<uint32_t>(ptr, ofs + 76);
        std::memcpy(&anim.bound4, ptr + ofs + 80, 12);
        anim.unk27 = Read<uint32_t>(ptr, ofs + 92);
        anim.unk28 = Read<uint64_t>(ptr, ofs + 96);
        anim.unk29 = Read<uint64_t>(ptr, ofs + 104);
    }

    MarkAnimatedSequences(model);
    return true;
}

bool M3Loader::ReadGeometry(const uint8_t* ptr, size_t size, M3ModelData& model) {
    const auto& h = model.header;
    size_t geomOfs = 0;
    if (!ResolveMetaRange(size, HEADER_SIZE, h.geometry, GEOM_RECORD_SIZE, geomOfs)) return false;

    size_t nestedStart = 0;
    if (!ResolveNestedDataStart(size, geomOfs, h.geometry.count,
                                GEOM_RECORD_SIZE, nestedStart)) return false;
    if (h.geometry.count == 0) return true;

    auto& geo = model.geometry;
    geo.nrVertices = Read<uint32_t>(ptr, geomOfs + 0x18);
    geo.vertexSize = ptr[geomOfs + 0x1C];
    geo.vertexSizePadding = ptr[geomOfs + 0x1D];
    geo.vertexFlags = Read<int16_t>(ptr, geomOfs + 0x1E);
    std::memcpy(geo.fieldTypes.data(), ptr + geomOfs + 0x20, 11);
    std::memcpy(geo.fieldOffsets.data(), ptr + geomOfs + 0x2B, 11);
    geo.nrIndices = Read<uint32_t>(ptr, geomOfs + 0x68);
    geo.indexSize = ptr[geomOfs + 0x6C];
    geo.indexFlags = ptr[geomOfs + 0x6D];

    const M3MetaDef vertexBlob = Read<M3MetaDef>(ptr, geomOfs + 0x38);
    const M3MetaDef indexBlob = Read<M3MetaDef>(ptr, geomOfs + 0x70);
    const M3MetaDef submeshArray = Read<M3MetaDef>(ptr, geomOfs + 0x80);
    geo.ofsIndices = indexBlob.offset < 0 ? 0u : static_cast<uint64_t>(indexBlob.offset);
    geo.nrSubmeshes = submeshArray.count;
    geo.ofsSubmeshes = submeshArray.offset < 0 ? 0u : static_cast<uint64_t>(submeshArray.offset);

    struct ArrayToValidate { size_t offset; size_t elementSize; };
    static constexpr ArrayToValidate kArrays[] = {
        {0x08, 4}, {0x38, 1}, {0x48, 4}, {0x58, 4}, {0x70, 1},
        {0x80, SUBMESH_SIZE}, {0x98, 4}, {0xA8, 2}, {0xB8, 4}
    };
    for (const auto& array : kArrays) {
        size_t ignored = 0;
        const M3MetaDef def = Read<M3MetaDef>(ptr, geomOfs + array.offset);
        if (!ResolveMetaRange(size, nestedStart, def, array.elementSize, ignored)) return false;
    }

    size_t vertexStart = 0;
    if (!ResolveMetaRange(size, nestedStart, vertexBlob, 1, vertexStart)) return false;
    size_t requiredVertexBytes = 0;
    if (!CheckedMul(geo.nrVertices, geo.vertexSize, requiredVertexBytes) ||
        requiredVertexBytes > vertexBlob.count) return false;

    const uint16_t streamMask = static_cast<uint16_t>(geo.vertexFlags);
    std::array<uint8_t, 11> streamOffsets = geo.fieldOffsets;
    if ((streamMask & 0x0300u) == 0x0100u) streamOffsets[9] = streamOffsets[8];

    for (size_t stream = 0; stream < streamOffsets.size(); ++stream) {
        if ((streamMask & (1u << stream)) == 0 && !(stream == 9 && (streamMask & 0x0100u))) continue;
        const size_t fieldSize = stream == 0 ? (geo.fieldTypes[0] == 1 ? 12u : 6u)
                              : stream <= 3 ? 2u
                              : stream <= 9 ? 4u
                              : 1u;
        if (streamOffsets[stream] > geo.vertexSize ||
            fieldSize > static_cast<size_t>(geo.vertexSize) - streamOffsets[stream]) return false;
    }

    geo.vertices.resize(geo.nrVertices);

    for (uint32_t i = 0; i < geo.nrVertices; i++) {
        const uint8_t* vData = ptr + vertexStart + i * geo.vertexSize;
        auto& v = geo.vertices[i];

        if (streamMask & 0x0001u) {
            size_t fieldOfs = streamOffsets[0];
            v.position = ReadVertexV3(vData, geo.fieldTypes[0] == 1 ? 1 : 2, fieldOfs);
        }
        if (streamMask & 0x0002u) {
            size_t fieldOfs = streamOffsets[1];
            v.tangent = ReadVertexV3(vData, 3, fieldOfs);
        }
        if (streamMask & 0x0004u) {
            size_t fieldOfs = streamOffsets[2];
            v.normal = ReadVertexV3(vData, 3, fieldOfs);
        }
        if (streamMask & 0x0008u) {
            size_t fieldOfs = streamOffsets[3];
            v.bitangent = ReadVertexV3(vData, 3, fieldOfs);
        }
        if (streamMask & 0x0010u) {
            size_t fieldOfs = streamOffsets[4];
            glm::vec4 bi = ReadVertexV4(vData, 4, fieldOfs);
            v.boneIndices = glm::uvec4(uint32_t(bi.x), uint32_t(bi.y), uint32_t(bi.z), uint32_t(bi.w));
        }
        if (streamMask & 0x0020u) {
            size_t fieldOfs = streamOffsets[5];
            glm::vec4 bw = ReadVertexV4(vData, 4, fieldOfs);
            v.boneWeights = bw / 255.0f;
        }
        if (streamMask & 0x0040u) {
            size_t fieldOfs = streamOffsets[6];
            glm::vec4 col = ReadVertexV4(vData, 4, fieldOfs);
            v.color = col / 255.0f;
        }
        if (streamMask & 0x0080u) {
            size_t fieldOfs = streamOffsets[7];
            glm::vec4 bl = ReadVertexV4(vData, 4, fieldOfs);
            v.blend = bl / 255.0f;
        }
        if (streamMask & 0x0100u) {
            size_t fieldOfs = streamOffsets[8];
            v.uv1 = ReadVertexV2(vData, 5, fieldOfs);
        }
        if (streamMask & 0x0200u) {
            size_t fieldOfs = streamOffsets[9];
            v.uv2 = ReadVertexV2(vData, 5, fieldOfs);
        } else if ((streamMask & 0x0300u) == 0x0100u) {
            v.uv2 = v.uv1;
        }
    }

    if (streamMask & 0x0080u) {
        int layerBlendCount = 0;
        int sampleCount = std::min((uint32_t)500, geo.nrVertices);

        for (int i = 0; i < sampleCount; i++) {
            const auto& bl = geo.vertices[i].blend;
            float total = bl.x + bl.y + bl.z + bl.w;
            if (total >= 0.95f && total <= 1.05f) {
                layerBlendCount++;
            }
        }

        geo.usesTextureLayerBlending = (layerBlendCount >= (sampleCount * 9 / 10));
    }

    size_t indexStart = 0;
    if (!ResolveMetaRange(size, nestedStart, indexBlob, 1, indexStart)) return false;
    if (geo.nrIndices != 0 && geo.indexSize != 2 && geo.indexSize != 4) return false;
    size_t requiredIndexBytes = 0;
    if (!CheckedMul(geo.nrIndices, geo.indexSize, requiredIndexBytes) ||
        requiredIndexBytes > indexBlob.count) return false;
    geo.indices.resize(geo.nrIndices);

    for (uint32_t i = 0; i < geo.nrIndices; i++) {
        if (geo.indexSize == 4) {
            geo.indices[i] = Read<uint32_t>(ptr, indexStart + i * 4);
        } else {
            geo.indices[i] = Read<uint16_t>(ptr, indexStart + i * 2);
        }
    }

    size_t submeshStart = 0;
    if (!ResolveMetaRange(size, nestedStart, submeshArray, SUBMESH_SIZE, submeshStart)) return false;
    geo.submeshes.resize(geo.nrSubmeshes);

    for (uint32_t i = 0; i < geo.nrSubmeshes; i++) {
        size_t smOfs = submeshStart + i * SUBMESH_SIZE;
        auto& sm = geo.submeshes[i];

        sm.startIndex = Read<uint32_t>(ptr, smOfs);
        sm.startVertex = Read<uint32_t>(ptr, smOfs + 4);
        sm.indexCount = Read<uint32_t>(ptr, smOfs + 8);
        sm.vertexCount = Read<uint32_t>(ptr, smOfs + 12);
        sm.startBoneMapping = Read<uint16_t>(ptr, smOfs + 16);
        sm.nrBoneMapping = Read<uint16_t>(ptr, smOfs + 18);
        sm.unk1 = Read<uint16_t>(ptr, smOfs + 20);
        sm.materialID = Read<int16_t>(ptr, smOfs + 22);
        sm.unk2 = Read<int16_t>(ptr, smOfs + 24);
        sm.unk3 = Read<int16_t>(ptr, smOfs + 26);
        sm.unk4 = Read<int16_t>(ptr, smOfs + 28);
        sm.groupId = static_cast<int8_t>(ptr[smOfs + 30]);
        sm.unkGroupRelated = ptr[smOfs + 31];
        sm.unk7 = Read<int16_t>(ptr, smOfs + 32);
        sm.anatomyId = Read<int16_t>(ptr, smOfs + 34);
        std::memcpy(sm.unk8To13.data(), ptr + smOfs + 36, 12);
        std::memcpy(sm.color0.data(), ptr + smOfs + 48, 4);
        std::memcpy(sm.color1.data(), ptr + smOfs + 52, 4);
        sm.unk16 = ptr[smOfs + 56];
        sm.unk17 = ptr[smOfs + 57];
        std::memcpy(&sm.boundMin, ptr + smOfs + 64, 16);
        std::memcpy(&sm.boundMax, ptr + smOfs + 80, 16);
        std::memcpy(&sm.unkVec4, ptr + smOfs + 96, 16);
    }

    return true;
}

void M3Loader::ApplyBoneMapping(M3ModelData& model) {
    if (model.lutBoneMapping.empty()) return;

    auto& geo = model.geometry;
    for (auto& vertex : geo.vertices) {
        for (int k = 0; k < 4; ++k) {
            if (vertex.boneIndices[k] < model.lutBoneMapping.size()) {
                const int16_t mappedBone = model.lutBoneMapping[vertex.boneIndices[k]];
                if (mappedBone >= 0 && mappedBone < static_cast<int16_t>(model.bones.size())) {
                    vertex.boneIndices[k] = static_cast<uint32_t>(mappedBone);
                }
            }
        }
    }
}

bool M3Loader::ReadLights(const uint8_t* ptr, size_t size, M3ModelData& model) {
    const auto& h = model.header;
    size_t tableStart = 0;
    if (!ResolveMetaRange(size, HEADER_SIZE, h.lights, LIGHT_SIZE, tableStart)) return false;

    size_t nestedStart = 0;
    if (!ResolveNestedDataStart(size, tableStart, h.lights.count, LIGHT_SIZE, nestedStart)) return false;

    model.lights.resize(h.lights.count);
    for (uint32_t i = 0; i < h.lights.count; ++i) {
        size_t ofs = tableStart + i * LIGHT_SIZE;

        auto& light = model.lights[i];
        light.boneId = Read<uint16_t>(ptr, ofs);
        light.unk00 = Read<uint16_t>(ptr, ofs + 2);
        light.unk01 = Read<int16_t>(ptr, ofs + 4);
        light.unk02 = Read<int16_t>(ptr, ofs + 6);
        light.unk03 = Read<int16_t>(ptr, ofs + 8);
        for (size_t track = 0; track < 16; ++track) {
            const size_t trackOffset = ofs + 0x10 + track * sizeof(M3TrackDef);
            const size_t valueSize = (track == 3 || track == 4) ? 4u : 2u;
            if (!ValidateTrackRecord(ptr, size, nestedStart, trackOffset, valueSize)) return false;
            if (track < 6) light.trackdefs[track] = Read<M3TrackDef>(ptr, trackOffset);
        }
    }

    return true;
}

bool M3Loader::ReadSubmeshGroups(const uint8_t* ptr, size_t size, M3ModelData& model) {
    const auto& h = model.header;
    size_t tableStart = 0;
    if (!ResolveMetaRange(size, HEADER_SIZE, h.submeshIds,
                          SUBMESH_GROUP_SIZE, tableStart)) return false;

    model.submeshGroups.resize(h.submeshIds.count);
    for (uint32_t i = 0; i < h.submeshIds.count; ++i) {
        size_t ofs = tableStart + i * SUBMESH_GROUP_SIZE;

        model.submeshGroups[i].submeshId = Read<uint16_t>(ptr, ofs);
        model.submeshGroups[i].unk1 = Read<uint16_t>(ptr, ofs + 2);
    }

    return true;
}

M3ModelData M3Loader::Load(const std::vector<uint8_t>& buffer) {
    M3ModelData model;
    if (buffer.size() < HEADER_SIZE) return model;

    const uint8_t* ptr = buffer.data();
    size_t size = buffer.size();

    if (!ReadHeader(ptr, size, model.header)) return model;

    if (!ReadLUTs(ptr, size, model) ||
        !ReadTextures(ptr, size, model) ||
        !ReadMaterials(ptr, size, model) ||
        !ReadBones(ptr, size, model) ||
        !ReadAnimations(ptr, size, model) ||
        !ReadGeometry(ptr, size, model)) return {};
    ApplyBoneMapping(model);
    if (!ReadLights(ptr, size, model) || !ReadSubmeshGroups(ptr, size, model)) return {};

    DebugPrintBonesOnLoad(model);

    model.success = true;
    return model;
}
