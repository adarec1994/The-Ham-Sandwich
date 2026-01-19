#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct ArchiveLink
    {
        uint32_t ID;
        uint32_t archiveArticleIdParent;
        uint32_t archiveArticleIdChild;
        uint32_t archiveLinkFlags;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            archiveArticleIdParent = file.getUint(recordIndex, i++);
            archiveArticleIdChild = file.getUint(recordIndex, i++);
            archiveLinkFlags = file.getUint(recordIndex, i++);
        }
    };
}
