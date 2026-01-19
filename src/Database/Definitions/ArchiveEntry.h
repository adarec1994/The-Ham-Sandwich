#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct ArchiveEntry
    {
        uint32_t ID;
        uint32_t localizedTextIdHeading;
        uint32_t localizedTextIdText[6];
        uint32_t localizedTextIdTextScientist[6];
        uint32_t creature2IdPortrait;
        std::wstring iconAssetPath;
        std::wstring inlineAssetPath;
        uint32_t archiveEntryTypeEnum;
        uint32_t archiveEntryFlags;
        uint32_t archiveEntryHeaderEnum;
        uint32_t characterTitleIdReward;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextIdHeading = file.getUint(recordIndex, i++);
            for (int j = 0; j < 6; ++j)
                localizedTextIdText[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 6; ++j)
                localizedTextIdTextScientist[j] = file.getUint(recordIndex, i++);
            creature2IdPortrait = file.getUint(recordIndex, i++);
            iconAssetPath = file.getString(recordIndex, i++);
            inlineAssetPath = file.getString(recordIndex, i++);
            archiveEntryTypeEnum = file.getUint(recordIndex, i++);
            archiveEntryFlags = file.getUint(recordIndex, i++);
            archiveEntryHeaderEnum = file.getUint(recordIndex, i++);
            characterTitleIdReward = file.getUint(recordIndex, i++);
        }
    };
}
