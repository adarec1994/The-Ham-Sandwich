#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct World
    {
        static constexpr const char* GetFileName() { return "World"; }
        uint32_t GetID() const { return ID; }

        enum Flags : uint32_t
        {
            unk0x1 = 0x1,
            unk0x2 = 0x2,
            unk0x4 = 0x4,
            unk0x8 = 0x8,
            unk0x10 = 0x10,
            unused0x20 = 0x20,
            unk0x40 = 0x40,
            unk0x80 = 0x80,
            unk0x100 = 0x100
        };

        uint32_t ID;
        std::wstring assetPath;
        uint32_t flags;
        uint32_t type;
        std::wstring screenPath;
        std::wstring screenModelPath;
        uint32_t chunkBounds00;
        uint32_t chunkBounds01;
        uint32_t chunkBounds02;
        uint32_t chunkBounds03;
        uint32_t plugAverageHeight;
        uint32_t localizedTextIdName;
        uint32_t minItemLevel;
        uint32_t maxItemLevel;
        uint32_t primeLevelOffset;
        uint32_t primeLevelMax;
        uint32_t veteranTierScalingType;
        uint32_t heroismMenaceLevel;
        uint32_t rewardRotationContentId;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            assetPath = file.getString(recordIndex, col++);
            flags = file.getUint(recordIndex, col++);
            type = file.getUint(recordIndex, col++);
            screenPath = file.getString(recordIndex, col++);
            screenModelPath = file.getString(recordIndex, col++);
            chunkBounds00 = file.getUint(recordIndex, col++);
            chunkBounds01 = file.getUint(recordIndex, col++);
            chunkBounds02 = file.getUint(recordIndex, col++);
            chunkBounds03 = file.getUint(recordIndex, col++);
            plugAverageHeight = file.getUint(recordIndex, col++);
            localizedTextIdName = file.getUint(recordIndex, col++);
            minItemLevel = file.getUint(recordIndex, col++);
            maxItemLevel = file.getUint(recordIndex, col++);
            primeLevelOffset = file.getUint(recordIndex, col++);
            primeLevelMax = file.getUint(recordIndex, col++);
            veteranTierScalingType = file.getUint(recordIndex, col++);
            heroismMenaceLevel = file.getUint(recordIndex, col++);
            rewardRotationContentId = file.getUint(recordIndex, col++);
        }
    };
}