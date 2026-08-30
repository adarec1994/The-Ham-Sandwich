#pragma once
#include "M3Common.h"
#include "../Archive.h"
#include <memory>
#include <cstdint>

class M3Loader {
public:
    static M3ModelData Load(const std::vector<uint8_t>& buffer);
    static M3ModelData LoadFromFile(const ArchivePtr& arc, const std::shared_ptr<FileEntry>& entry);

private:
    static constexpr size_t HEADER_SIZE = 1584;
    static constexpr size_t TEX_ENTRY_SIZE = 32;
    static constexpr size_t MAT_ENTRY_SIZE = 48;
    static constexpr size_t MAT_DESC_SIZE = 296;
    static constexpr size_t GEOM_RECORD_SIZE = 200;
    static constexpr size_t SUBMESH_SIZE = 112;
    static constexpr size_t BONE_SIZE = 352;
    static constexpr size_t ANIMATION_SIZE = 112;
    static constexpr size_t LIGHT_SIZE = 400;
    static constexpr size_t SUBMESH_GROUP_SIZE = 4;

    static float HalfToFloat(uint16_t h);
    static float Int16ToFloat(int16_t v);
    static glm::vec3 ReadVertexV3(const uint8_t* data, uint8_t type, size_t& offset);
    static glm::vec4 ReadVertexV4(const uint8_t* data, uint8_t type, size_t& offset);
    static glm::vec2 ReadVertexV2(const uint8_t* data, uint8_t type, size_t& offset);

    static bool ReadHeader(const uint8_t* data, size_t size, M3Header& header);
    static bool ReadLUTs(const uint8_t* data, size_t size, M3ModelData& model);
    static bool ReadTextures(const uint8_t* data, size_t size, M3ModelData& model);
    static bool ReadMaterials(const uint8_t* data, size_t size, M3ModelData& model);
    static bool ReadBones(const uint8_t* data, size_t size, M3ModelData& model);
    static bool ReadAnimations(const uint8_t* data, size_t size, M3ModelData& model);
    static bool ReadGeometry(const uint8_t* data, size_t size, M3ModelData& model);
    static bool ReadLights(const uint8_t* data, size_t size, M3ModelData& model);
    static bool ReadSubmeshGroups(const uint8_t* data, size_t size, M3ModelData& model);
    static bool ReadBoneAnimationTrack(const uint8_t* data, size_t size, size_t animStart, M3AnimationTrack& track);
    static void ApplyBoneMapping(M3ModelData& model);
    static void BuildBonePaths(M3ModelData& model);
    static void FixMirroredBones(M3ModelData& model);

    template<typename T>
    static T Read(const uint8_t* data, size_t offset);

    template<typename T>
    static void ReadArray(const uint8_t* data, size_t offset, size_t count, std::vector<T>& out);
};
