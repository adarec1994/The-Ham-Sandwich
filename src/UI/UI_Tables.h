// src/UI/UI_Tables.h
#pragma once
#include "UI.h"
#include "../Database/Tbl.h"
#include <memory>

struct TblViewState
{
    bool show_window = false;
    std::string tableName;
    std::unique_ptr<Tbl::File> file;
};

namespace UI_Tables
{
    void OpenTblFile(AppState& state, const ArchivePtr& arc, const std::shared_ptr<FileEntry>& fileEntry);
    void Draw(AppState& state);
}