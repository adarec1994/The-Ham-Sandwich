#include "DataTable.h"
#include <iostream>

DataTable::DataTable(FileEntryPtr file, ArchivePtr archive) 
    : mArchive(archive)
{
    if (mArchive && file) {
        mArchive->getFileData(file, mContent);
        mStream = std::make_shared<BinStream>(mContent);
    }
}

bool DataTable::initialLoad() {
    if (!mStream || mContent.empty()) return false;

    mStream->seek(0);
    mHeader = mStream->read<DTBLHeader>();

    if (mHeader.magic != 0x4C425444) { // 'DTBL'
        return false; 
    }

    mStream->seek(0x60 + mHeader.ofsFieldDesc);
    mFieldDescs.resize((uint32)mHeader.numRows);

    // Read field descriptors
    mStream->read(mFieldDescs.data(), sizeof(FieldDescEntry) * mHeader.numRows);

    // Read table name
    if (mHeader.lenTableName > 0) {
        std::vector<wchar_t> tableName((uint32)mHeader.lenTableName);
        mStream->seek(0x60);
        mStream->read(tableName.data(), tableName.size() * sizeof(wchar_t));
        tableName.push_back((wchar_t) 0);
        mTableName = tableName.data();
    }

    return true;
}

bool DataTable::initialLoadIDs() {
    if (!initialLoad()) {
        return false;
    }

    mStream->seek(mHeader.ofsEntries + 0x60);
    // Iterate all entries to build ID lookup map
    for (uint32 i = 0; i < mHeader.numEntries; ++i) {
        mStream->seek(mHeader.ofsEntries + 0x60 + i * mHeader.recordSize);
        uint32 id = mStream->read<uint32>();
        mRows[id] = i;
    }

    return true;
}