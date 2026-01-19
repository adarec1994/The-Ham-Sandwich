#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct ArchiveArticle
    {
        uint32_t ID;
        uint32_t creature2IdPortrait;
        std::wstring iconAssetPath;
        uint32_t localizedTextIdTitle;
        uint32_t localizedTextIdText[6];
        uint32_t archiveEntryId[16];
        uint32_t archiveArticleFlags;
        uint32_t archiveCategoryId[3];
        uint32_t localizedTextIdToolTip;
        uint32_t worldZoneId;
        uint32_t characterTitleIdReward;
        std::wstring linkName;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            creature2IdPortrait = file.getUint(recordIndex, i++);
            iconAssetPath = file.getString(recordIndex, i++);
            localizedTextIdTitle = file.getUint(recordIndex, i++);
            for (int j = 0; j < 6; ++j)
                localizedTextIdText[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 16; ++j)
                archiveEntryId[j] = file.getUint(recordIndex, i++);
            archiveArticleFlags = file.getUint(recordIndex, i++);
            for (int j = 0; j < 3; ++j)
                archiveCategoryId[j] = file.getUint(recordIndex, i++);
            localizedTextIdToolTip = file.getUint(recordIndex, i++);
            worldZoneId = file.getUint(recordIndex, i++);
            characterTitleIdReward = file.getUint(recordIndex, i++);
            linkName = file.getString(recordIndex, i++);
        }
    };
}
