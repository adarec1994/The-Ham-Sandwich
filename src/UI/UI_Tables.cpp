// src/UI/UI_Tables.cpp
#include "UI_Tables.h"
#include "UI_Utils.h"
#include "../Archive.h"

#include <imgui.h>
#include <iostream>

static TblViewState gTblState;

namespace UI_Tables
{
    void OpenTblFile(AppState& state, const ArchivePtr& arc, const std::shared_ptr<FileEntry>& fileEntry)
    {
        if (!arc || !fileEntry) return;

        std::string name = wstring_to_utf8(fileEntry->getEntryName());
        std::cout << "Loading TBL: " << name << std::endl;

        std::vector<uint8_t> data;
        if (!arc->getFileData(fileEntry, data) || data.empty())
        {
            std::cout << "Failed to read TBL file data." << std::endl;
            return;
        }

        auto tblFile = std::make_unique<Tbl::File>();
        if (!tblFile->load(data.data(), data.size()))
        {
            std::cout << "Failed to parse TBL file." << std::endl;
            return;
        }

        gTblState.file = std::move(tblFile);
        gTblState.tableName = name;
        gTblState.show_window = true;

        std::cout << "TBL loaded. Records: " << gTblState.file->getRecordCount()
                  << ", Columns: " << gTblState.file->getColumns().size() << std::endl;
    }

    static const char* DataTypeToString(Tbl::DataType type)
    {
        switch (type)
        {
            case Tbl::DataType::Uint:   return "Uint";
            case Tbl::DataType::Float:  return "Float";
            case Tbl::DataType::Flags:  return "Flags";
            case Tbl::DataType::Ulong:  return "Ulong";
            case Tbl::DataType::String: return "String";
            default:                    return "Unknown";
        }
    }

    void Draw(AppState& state)
    {
        if (!gTblState.show_window || !gTblState.file) return;

        ImGuiViewport* viewport = ImGui::GetMainViewport();

        float strip_width = 70.0f;
        float left_offset = strip_width + state.sidebar_current_width;
        float available_width = viewport->Size.x - left_offset;
        float available_height = viewport->Size.y;

        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + left_offset, viewport->Pos.y));
        ImGui::SetNextWindowSize(ImVec2(available_width, available_height));

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove
                                      | ImGuiWindowFlags_NoResize
                                      | ImGuiWindowFlags_NoCollapse;

        if (ImGui::Begin(gTblState.tableName.c_str(), &gTblState.show_window, window_flags))
        {
            const auto& header = gTblState.file->getHeader();
            const auto& columns = gTblState.file->getColumns();

            if (ImGui::CollapsingHeader("Header Info"))
            {
                ImGui::Text("Magic: 0x%08X", header.magic);
                ImGui::Text("Version: %d", header.version);
                ImGui::Text("Table Name: %s", wstring_to_utf8(gTblState.file->getTableName()).c_str());
                ImGui::Text("Record Size: %u bytes", header.recordSize);
                ImGui::Text("Record Count: %u", header.recordCount);
                ImGui::Text("Field Count: %lld", header.fieldCount);
                ImGui::Text("Max ID: %lld", header.maxID);
            }

            if (ImGui::CollapsingHeader("Columns"))
            {
                if (ImGui::BeginTable("ColumnsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
                {
                    ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                    ImGui::TableHeadersRow();

                    for (size_t i = 0; i < columns.size(); ++i)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("%zu", i);
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(wstring_to_utf8(columns[i].name).c_str());
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(DataTypeToString(columns[i].dataType));
                    }

                    ImGui::EndTable();
                }
            }

            if (ImGui::CollapsingHeader("Data", ImGuiTreeNodeFlags_DefaultOpen))
            {
                int columnCount = static_cast<int>(columns.size());
                if (columnCount > 0)
                {
                    ImGuiTableFlags flags = ImGuiTableFlags_Borders
                                          | ImGuiTableFlags_RowBg
                                          | ImGuiTableFlags_ScrollX
                                          | ImGuiTableFlags_ScrollY
                                          | ImGuiTableFlags_Resizable;

                    ImVec2 tableSize = ImVec2(0, ImGui::GetContentRegionAvail().y - 10);

                    if (ImGui::BeginTable("DataTable", columnCount, flags, tableSize))
                    {
                        for (size_t i = 0; i < columns.size(); ++i)
                        {
                            std::string colName = wstring_to_utf8(columns[i].name);
                            float width = (columns[i].dataType == Tbl::DataType::String) ? 200.0f : 80.0f;
                            ImGui::TableSetupColumn(colName.c_str(), ImGuiTableColumnFlags_WidthFixed, width);
                        }
                        ImGui::TableSetupScrollFreeze(1, 1);
                        ImGui::TableHeadersRow();

                        ImGuiListClipper clipper;
                        clipper.Begin(static_cast<int>(header.recordCount));

                        while (clipper.Step())
                        {
                            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
                            {
                                ImGui::TableNextRow();

                                for (size_t col = 0; col < columns.size(); ++col)
                                {
                                    ImGui::TableNextColumn();

                                    switch (columns[col].dataType)
                                    {
                                        case Tbl::DataType::Uint:
                                        case Tbl::DataType::Flags:
                                            ImGui::Text("%u", gTblState.file->getUint(row, col));
                                            break;

                                        case Tbl::DataType::Float:
                                            ImGui::Text("%.4f", gTblState.file->getFloat(row, col));
                                            break;

                                        case Tbl::DataType::Ulong:
                                            ImGui::Text("%lld", gTblState.file->getInt64(row, col));
                                            break;

                                        case Tbl::DataType::String:
                                        {
                                            std::wstring ws = gTblState.file->getString(row, col);
                                            ImGui::TextUnformatted(wstring_to_utf8(ws).c_str());
                                            break;
                                        }

                                        default:
                                            ImGui::TextUnformatted("?");
                                            break;
                                    }
                                }
                            }
                        }

                        ImGui::EndTable();
                    }
                }
                else
                {
                    ImGui::TextUnformatted("No columns defined.");
                }
            }
        }
        ImGui::End();
    }
}