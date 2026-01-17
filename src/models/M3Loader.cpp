#include "M3Loader.h"
#include <cstring>
#include <cmath>
#include <algorithm>

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
    uint32_t s = (h & 0x8000) << 16;
    int32_t e = (h & 0x7C00) >> 10;
    uint32_t m = (h & 0x03FF) << 13;
    uint32_t v;
    if (e == 0) v = (m == 0) ? s : s | 0x00800000 | (m << 1);
    else if (e == 0x1F) v = s | 0x7F800000 | m;
    else v = s | ((e + 112) << 23) | m;
    float f;
    std::memcpy(&f, &v, sizeof(f));
    return f;
}

float M3Loader::Int16ToFloat(int16_t v) {
    return v / 16383.5f;
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
        float fx = (x - 127.0f) / 127.0f;
        float fy = (y - 127.0f) / 127.0f;
        float fz = std::sqrt(std::max(1.0f - fx*fx - fy*fy, 0.0f));
        res = glm::vec3(fx, fy, fz);
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
    return Load(buffer);
}

bool M3Loader::ReadHeader(const uint8_t* ptr, size_t size, M3Header& h) {
    if (size < HEADER_SIZE) return false;

    std::memcpy(h.signature, ptr, 4);
    h.version = Read<uint32_t>(ptr, 4);
    if (h.version != 100) return false;

    h.unk008 = Read<uint32_t>(ptr, 8);

    size_t ofs = 0x10;
    h.animationsMeta = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    for (int i = 0; i < 4; i++) { h.trackdefAnim[i] = Read<M3TrackDef>(ptr, ofs); ofs += 24; }
    h.struct080 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.trackdef090 = Read<M3TrackDef>(ptr, ofs); ofs += 24;
    h.trackdef0A8 = Read<M3TrackDef>(ptr, ofs); ofs += 24;
    h.trackdef0C0 = Read<M3TrackDef>(ptr, ofs); ofs += 24;
    h.trackdef0D8 = Read<M3TrackDef>(ptr, ofs); ofs += 24;
    h.struct0F0 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.trackdef100 = Read<M3TrackDef>(ptr, ofs); ofs += 24;
    h.trackdef118 = Read<M3TrackDef>(ptr, ofs); ofs += 24;
    h.trackdef130 = Read<M3TrackDef>(ptr, ofs); ofs += 24;
    h.trackdef148 = Read<M3TrackDef>(ptr, ofs); ofs += 24;
    h.trackdef160 = Read<M3TrackDef>(ptr, ofs); ofs += 24;
    h.unkFloat178 = Read<float>(ptr, ofs); ofs += 8;
    h.bones = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.lut190 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.lut1A0 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.lutBoneIds = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.textures = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.lut1D0 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.struct1E0 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.materials = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.submeshIds = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.lut210 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    ofs += 48;
    h.geometry = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.lut260 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.lut270 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.lut280 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.trackdef290 = Read<M3TrackDef>(ptr, ofs); ofs += 24;
    ofs += 16;
    h.struct2B8 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.lut2C8 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    ofs += 32;
    h.struct2F8 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.struct308 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.lights = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.struct328 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.lut338 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.idUnk348 = Read<int64_t>(ptr, ofs); ofs += 8;
    h.trackdef350 = Read<M3TrackDef>(ptr, ofs); ofs += 24;
    h.idUnk368 = Read<int64_t>(ptr, ofs); ofs += 8;
    h.trackdef370 = Read<M3TrackDef>(ptr, ofs); ofs += 24;
    std::memcpy(h.floatUnk380, ptr + ofs, 8); ofs += 8;

    for (int i = 0; i < 4; i++) {
        h.bounds[i] = Read<M3Bounds>(ptr, ofs); ofs += 56;
        ofs += 8;
    }

    h.struct490 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.lut4A0 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.bounds[4] = Read<M3Bounds>(ptr, ofs); ofs += 56;
    ofs += 8;
    h.floatUnk4F0 = Read<float>(ptr, ofs); ofs += 8;
    h.floatUnk4F8 = Read<float>(ptr, ofs); ofs += 8;
    std::memcpy(&h.posUnk500, ptr + ofs, 12); ofs += 16;
    h.lut510 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.lut520 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.lut530 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.struct540 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.lut550 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.struct560 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.struct570 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.idUnk580 = Read<int64_t>(ptr, ofs); ofs += 8;
    h.struct588 = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.customBoneMinMax = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    h.lutBoneToCustom = Read<M3MetaDef>(ptr, ofs); ofs += 16;
    ofs += 8;
    h.trackdef5C0 = Read<M3TrackDef>(ptr, ofs); ofs += 24;

    return true;
}

void M3Loader::ReadLUTs(const uint8_t* ptr, size_t size, M3ModelData& model) {
    const auto& h = model.header;

    auto readLutI16 = [&](const M3MetaDef& def, std::vector<int16_t>& out) {
        if (def.count > 0 && HEADER_SIZE + def.offset + def.count * 2 <= size) {
            out.resize(def.count);
            std::memcpy(out.data(), ptr + HEADER_SIZE + def.offset, def.count * 2);
        }
    };

    auto readLutI32 = [&](const M3MetaDef& def, std::vector<int32_t>& out) {
        if (def.count > 0 && HEADER_SIZE + def.offset + def.count * 4 <= size) {
            out.resize(def.count);
            std::memcpy(out.data(), ptr + HEADER_SIZE + def.offset, def.count * 4);
        }
    };

    readLutI16(h.lut190, model.lut190);
    readLutI16(h.lut1A0, model.lut1A0);
    readLutI16(h.lutBoneIds, model.lutBoneMapping);
    readLutI16(h.lut1D0, model.lut1D0);
    readLutI16(h.lut210, model.lut210);
    readLutI16(h.lut260, model.lut260);
    readLutI16(h.lut270, model.lut270);
    readLutI16(h.lut280, model.lut280);
    readLutI16(h.lut338, model.lut338);
    readLutI16(h.lut4A0, model.lut4A0);
    readLutI32(h.lut520, model.lut520);
    readLutI32(h.lut530, model.lut530);
    readLutI32(h.lut550, model.lut550);

    if (h.lut510.count > 0) {
        model.lut510.resize(h.lut510.count);
        size_t ofs = HEADER_SIZE + h.lut510.offset;
        for (size_t i = 0; i < (size_t)h.lut510.count; i++) {
            std::memcpy(&model.lut510[i], ptr + ofs + i * 16, 16);
        }
    }
}

void M3Loader::ReadTextures(const uint8_t* ptr, size_t size, M3ModelData& model) {
    const auto& h = model.header;
    if (h.textures.count <= 0) return;

    size_t tableStart = HEADER_SIZE + h.textures.offset;
    size_t dataStart = tableStart + h.textures.count * TEX_ENTRY_SIZE;

    model.textures.resize(h.textures.count);
    for (int64_t i = 0; i < h.textures.count; i++) {
        size_t ofs = tableStart + i * TEX_ENTRY_SIZE;
        if (ofs + TEX_ENTRY_SIZE > size) break;

        auto& tex = model.textures[i];
        tex.unk0 = Read<int16_t>(ptr, ofs);
        tex.type = ptr[ofs + 2];
        tex.unk1 = ptr[ofs + 3];
        tex.flags = Read<int32_t>(ptr, ofs + 4);
        tex.intensity = Read<float>(ptr, ofs + 8);
        tex.unk4 = ptr[ofs + 12];
        tex.unk5 = ptr[ofs + 13];
        tex.unk6 = ptr[ofs + 14];
        tex.unk7 = ptr[ofs + 15];
        tex.nrLetters = Read<uint64_t>(ptr, ofs + 16);
        tex.offset = Read<uint64_t>(ptr, ofs + 24);

        size_t strPos = dataStart + tex.offset;
        size_t byteLen = tex.nrLetters * 2;
        if (strPos + byteLen <= size) {
            tex.path.reserve(tex.nrLetters);
            for (size_t j = 0; j < tex.nrLetters; j++) {
                char16_t c;
                std::memcpy(&c, ptr + strPos + j * 2, 2);
                tex.path.push_back((c < 128) ? char(c) : '_');
            }
        }

        if (tex.type == 0) tex.textureType = "color";
        else if (tex.type == 1) tex.textureType = "normal";
        else tex.textureType = "unknown";
    }
}

void M3Loader::ReadMaterials(const uint8_t* ptr, size_t size, M3ModelData& model) {
    const auto& h = model.header;
    if (h.materials.count <= 0) return;

    size_t tableStart = HEADER_SIZE + h.materials.offset;
    size_t dataStart = tableStart + h.materials.count * MAT_ENTRY_SIZE;

    model.materials.resize(h.materials.count);
    for (int64_t i = 0; i < h.materials.count; i++) {
        size_t ofs = tableStart + i * MAT_ENTRY_SIZE;
        if (ofs + MAT_ENTRY_SIZE > size) break;

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
        mat.nrDescriptions = Read<uint64_t>(ptr, ofs + 32);
        mat.ofsDescriptions = Read<uint64_t>(ptr, ofs + 40);

        if (mat.nrDescriptions > 0) {
            size_t descBase = dataStart + mat.ofsDescriptions;
            mat.variants.resize(mat.nrDescriptions);
            for (uint64_t d = 0; d < mat.nrDescriptions; d++) {
                size_t descOfs = descBase + d * MAT_DESC_SIZE;
                if (descOfs + 4 > size) break;

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
}

void M3Loader::ReadBoneAnimationTrack(const uint8_t* ptr, size_t animStart, M3AnimationTrack& track) {
    if (track.duration <= 0) return;

    track.keyframes.resize(track.duration);

    size_t timeOfs = animStart + track.timeOffset;
    for (int64_t i = 0; i < track.duration; i++) {
        track.keyframes[i].timestamp = Read<uint32_t>(ptr, timeOfs + i * 4);
    }

    size_t valOfs = animStart + track.valueOffset;
    int type = track.trackType;

    if (type >= 1 && type <= 3) {
        for (int64_t i = 0; i < track.duration; i++) {
            uint16_t h[3];
            std::memcpy(h, ptr + valOfs + i * 6, 6);
            track.keyframes[i].scale = glm::vec3(HalfToFloat(h[0]), HalfToFloat(h[1]), HalfToFloat(h[2]));
        }
    } else if (type == 5 || type == 6) {
        for (int64_t i = 0; i < track.duration; i++) {
            int16_t q[4];
            std::memcpy(q, ptr + valOfs + i * 8, 8);
            track.keyframes[i].rotation = glm::quat(
                Int16ToFloat(q[3]), Int16ToFloat(q[0]), Int16ToFloat(q[1]), Int16ToFloat(q[2])
            );
        }
    } else if (type == 7) {
        for (int64_t i = 0; i < track.duration; i++) {
            std::memcpy(&track.keyframes[i].translation, ptr + valOfs + i * 12, 12);
        }
    } else if (type == 8) {
        for (int64_t i = 0; i < track.duration; i++) {
            int16_t v[4];
            std::memcpy(v, ptr + valOfs + i * 8, 8);
            track.keyframes[i].unknown = glm::vec4(
                Int16ToFloat(v[0]), Int16ToFloat(v[1]), Int16ToFloat(v[2]), Int16ToFloat(v[3])
            );
        }
    } else {
        for (int64_t i = 0; i < track.duration; i++) {
            uint16_t h[4];
            std::memcpy(h, ptr + valOfs + i * 8, 8);
            track.keyframes[i].unknown = glm::vec4(
                HalfToFloat(h[0]), HalfToFloat(h[1]), HalfToFloat(h[2]), HalfToFloat(h[3])
            );
        }
    }
}

void M3Loader::ReadBones(const uint8_t* ptr, size_t size, M3ModelData& model) {
    const auto& h = model.header;
    if (h.bones.count <= 0) return;

    size_t tableStart = HEADER_SIZE + h.bones.offset;
    size_t animStart = tableStart + h.bones.count * BONE_SIZE;

    model.bones.resize(h.bones.count);
    for (int64_t i = 0; i < h.bones.count; i++) {
        size_t ofs = tableStart + i * BONE_SIZE;
        if (ofs + BONE_SIZE > size) break;

        auto& bone = model.bones[i];
        bone.id = static_cast<int32_t>(i);
        bone.name = "Bone_" + std::to_string(i);

        bone.globalId = Read<int16_t>(ptr, ofs);
        bone.flags = Read<uint16_t>(ptr, ofs + 2);
        bone.parentId = Read<int16_t>(ptr, ofs + 4);
        bone.unk01 = Read<int16_t>(ptr, ofs + 6);
        bone.unk02 = ptr[ofs + 8];
        bone.unk03 = ptr[ofs + 9];
        bone.unk04 = ptr[ofs + 10];
        bone.unk05 = ptr[ofs + 11];
        bone.unk06 = Read<uint32_t>(ptr, ofs + 12);

        size_t trackOfs = ofs + 16;
        for (int t = 0; t < 8; t++) {
            bone.tracks[t].duration = Read<int64_t>(ptr, trackOfs);
            bone.tracks[t].timeOffset = Read<int64_t>(ptr, trackOfs + 8);
            bone.tracks[t].valueOffset = Read<int64_t>(ptr, trackOfs + 16);
            bone.tracks[t].trackType = t + 1;
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
            ReadBoneAnimationTrack(ptr, animStart, bone.tracks[t]);
        }
    }

    BuildBonePaths(model);
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

void M3Loader::ReadAnimations(const uint8_t* ptr, size_t size, M3ModelData& model) {
    const auto& h = model.header;
    if (h.animationsMeta.count <= 0) return;

    size_t tableStart = HEADER_SIZE + h.animationsMeta.offset;

    model.animations.resize(h.animationsMeta.count);
    for (int64_t i = 0; i < h.animationsMeta.count; i++) {
        size_t ofs = tableStart + i * ANIMATION_SIZE;
        if (ofs + ANIMATION_SIZE > size) break;

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
}

void M3Loader::ReadGeometry(const uint8_t* ptr, size_t size, M3ModelData& model) {
    const auto& h = model.header;
    if (h.geometry.offset == 0) return;

    size_t geomOfs = HEADER_SIZE + h.geometry.offset;
    if (geomOfs + GEOM_SIZE > size) return;

    auto& geo = model.geometry;
    geo.nrVertices = Read<uint32_t>(ptr, geomOfs + 0x18);
    geo.vertexSize = Read<uint16_t>(ptr, geomOfs + 0x1C);
    geo.vertexFlags = Read<int16_t>(ptr, geomOfs + 0x1E);
    std::memcpy(geo.fieldTypes.data(), ptr + geomOfs + 0x20, 11);
    geo.nrIndices = Read<uint32_t>(ptr, geomOfs + 0x68);
    geo.indexFlags = Read<int16_t>(ptr, geomOfs + 0x6C);
    geo.ofsIndices = Read<uint32_t>(ptr, geomOfs + 0x78);
    geo.nrSubmeshes = Read<uint32_t>(ptr, geomOfs + 0x80);
    geo.ofsSubmeshes = Read<uint32_t>(ptr, geomOfs + 0x88);

    size_t vertexStart = geomOfs + GEOM_SIZE;
    geo.vertices.resize(geo.nrVertices);

    for (uint32_t i = 0; i < geo.nrVertices; i++) {
        const uint8_t* vData = ptr + vertexStart + i * geo.vertexSize;
        auto& v = geo.vertices[i];
        size_t localOfs = 0;

        if (geo.vertexFlags & 0x0001) v.position = ReadVertexV3(vData, geo.fieldTypes[0], localOfs);
        if (geo.vertexFlags & 0x0002) v.tangent = ReadVertexV3(vData, geo.fieldTypes[1], localOfs);
        if (geo.vertexFlags & 0x0004) v.normal = ReadVertexV3(vData, geo.fieldTypes[2], localOfs);
        if (geo.vertexFlags & 0x0008) v.bitangent = ReadVertexV3(vData, geo.fieldTypes[3], localOfs);
        if (geo.vertexFlags & 0x0010) {
            glm::vec4 bi = ReadVertexV4(vData, geo.fieldTypes[4], localOfs);
            v.boneIndices = glm::uvec4(uint32_t(bi.x), uint32_t(bi.y), uint32_t(bi.z), uint32_t(bi.w));
        }
        if (geo.vertexFlags & 0x0020) {
            glm::vec4 bw = ReadVertexV4(vData, geo.fieldTypes[5], localOfs);
            v.boneWeights = bw / 255.0f;
        }
        if (geo.vertexFlags & 0x0040) {
            glm::vec4 col = ReadVertexV4(vData, geo.fieldTypes[6], localOfs);
            v.color = col / 255.0f;
        }
        if (geo.vertexFlags & 0x0080) {
            v.blend = ReadVertexV4(vData, geo.fieldTypes[7], localOfs);
        }
        if (geo.vertexFlags & 0x0100) v.uv1 = ReadVertexV2(vData, geo.fieldTypes[8], localOfs);
        if (geo.vertexFlags & 0x0200) v.uv2 = ReadVertexV2(vData, geo.fieldTypes[9], localOfs);
    }

    size_t indexStart = geomOfs + GEOM_SIZE + geo.ofsIndices;
    bool is32Bit = (geo.indexFlags & 0x0200) == 0x0200;
    geo.indices.resize(geo.nrIndices);

    for (uint32_t i = 0; i < geo.nrIndices; i++) {
        if (is32Bit) {
            geo.indices[i] = Read<uint32_t>(ptr, indexStart + i * 4);
        } else {
            geo.indices[i] = Read<uint16_t>(ptr, indexStart + i * 2);
        }
    }

    size_t submeshStart = geomOfs + GEOM_SIZE + geo.ofsSubmeshes;
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
        sm.materialID = Read<uint16_t>(ptr, smOfs + 22);
        sm.unk2 = Read<uint16_t>(ptr, smOfs + 24);
        sm.unk3 = Read<uint16_t>(ptr, smOfs + 26);
        sm.unk4 = Read<uint16_t>(ptr, smOfs + 28);
        sm.groupId = ptr[smOfs + 30];
        sm.unkGroupRelated = ptr[smOfs + 31];
        std::memcpy(sm.color0.data(), ptr + smOfs + 32, 4);
        std::memcpy(sm.color1.data(), ptr + smOfs + 36, 4);
        std::memcpy(&sm.boundMin, ptr + smOfs + 40, 16);
        std::memcpy(&sm.boundMax, ptr + smOfs + 56, 16);
        std::memcpy(&sm.unkVec4, ptr + smOfs + 72, 16);
    }
}

void M3Loader::ApplyBoneMapping(M3ModelData& model) {
    if (model.lutBoneMapping.empty()) return;

    auto& geo = model.geometry;
    for (auto& sm : geo.submeshes) {
        std::vector<int16_t> boneSubmap;
        for (uint16_t i = sm.startBoneMapping; i < sm.startBoneMapping + sm.nrBoneMapping; i++) {
            if (i < model.lutBoneMapping.size()) {
                boneSubmap.push_back(model.lutBoneMapping[i]);
            }
        }

        if (boneSubmap.empty()) continue;

        for (uint32_t j = sm.startVertex; j < sm.startVertex + sm.vertexCount; j++) {
            if (j >= geo.vertices.size()) break;
            auto& v = geo.vertices[j];

            for (int k = 0; k < 4; k++) {
                if (v.boneWeights[k] > 0 && v.boneIndices[k] < boneSubmap.size()) {
                    v.boneIndices[k] = boneSubmap[v.boneIndices[k]];
                }
            }
        }
    }
}

void M3Loader::ReadLights(const uint8_t* ptr, size_t size, M3ModelData& model) {
    const auto& h = model.header;
    if (h.lights.count <= 0) return;

    size_t tableStart = HEADER_SIZE + h.lights.offset;

    model.lights.resize(h.lights.count);
    for (int64_t i = 0; i < h.lights.count; i++) {
        size_t ofs = tableStart + i * LIGHT_SIZE;
        if (ofs + LIGHT_SIZE > size) break;

        auto& light = model.lights[i];
        light.boneId = Read<uint16_t>(ptr, ofs);
        light.unk00 = Read<uint16_t>(ptr, ofs + 2);
        light.unk01 = Read<int16_t>(ptr, ofs + 4);
        light.unk02 = Read<int16_t>(ptr, ofs + 6);
        light.unk03 = Read<int16_t>(ptr, ofs + 8);
        std::memcpy(light.values1.data(), ptr + ofs + 10, 78);
        std::memcpy(light.trackdefs, ptr + ofs + 88, 72);
        std::memcpy(light.values2.data(), ptr + ofs + 160, 120);
        std::memcpy(light.trackdefs + 3, ptr + ofs + 280, 72);
        std::memcpy(light.values3.data(), ptr + ofs + 352, 48);
    }
}

void M3Loader::ReadSubmeshGroups(const uint8_t* ptr, size_t size, M3ModelData& model) {
    const auto& h = model.header;
    if (h.submeshIds.count <= 0) return;

    size_t tableStart = HEADER_SIZE + h.submeshIds.offset;

    model.submeshGroups.resize(h.submeshIds.count);
    for (int64_t i = 0; i < h.submeshIds.count; i++) {
        size_t ofs = tableStart + i * SUBMESH_GROUP_SIZE;
        if (ofs + SUBMESH_GROUP_SIZE > size) break;

        model.submeshGroups[i].submeshId = Read<uint16_t>(ptr, ofs);
        model.submeshGroups[i].unk1 = Read<uint16_t>(ptr, ofs + 2);
    }
}

M3ModelData M3Loader::Load(const std::vector<uint8_t>& buffer) {
    M3ModelData model;
    if (buffer.size() < HEADER_SIZE) return model;

    const uint8_t* ptr = buffer.data();
    size_t size = buffer.size();

    if (!ReadHeader(ptr, size, model.header)) return model;

    ReadLUTs(ptr, size, model);
    ReadTextures(ptr, size, model);
    ReadMaterials(ptr, size, model);
    ReadBones(ptr, size, model);
    ReadAnimations(ptr, size, model);
    ReadGeometry(ptr, size, model);
    ApplyBoneMapping(model);
    ReadLights(ptr, size, model);
    ReadSubmeshGroups(ptr, size, model);

    model.success = true;
    return model;
}