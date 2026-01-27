// WwiseCodebooks.cpp - Load Wwise Vorbis codebook data from packed file
// Supports aoTuV 603 format used by modern Wwise games (2012+)

#include "WwiseCodebooks.h"
#include <fstream>
#include <cstring>

namespace Audio
{

// Static storage for loaded codebook data
static std::vector<uint8_t> g_codebookData;
static std::vector<CodebookEntry> g_codebookEntries;
static bool g_initialized = false;
static std::string g_codebookPath;

static uint32_t ReadU32LE(const uint8_t* data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

bool InitializeCodebooks(const std::string& path)
{
    if (g_initialized && g_codebookPath == path)
        return true;

    g_codebookData.clear();
    g_codebookEntries.clear();
    g_initialized = false;
    g_codebookPath = path;

    // Read the entire file
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return false;

    size_t fileSize = (size_t)file.tellg();
    if (fileSize < 8)
    {
        file.close();
        return false;
    }

    file.seekg(0);
    g_codebookData.resize(fileSize);
    file.read(reinterpret_cast<char*>(g_codebookData.data()), fileSize);
    file.close();

    // Parse the offset table at the end of the file
    // Last 4 bytes = offset to start of table
    uint32_t tableStart = ReadU32LE(g_codebookData.data() + fileSize - 4);

    if (tableStart >= fileSize)
        return false;

    // Each entry in the table is a 4-byte offset
    // Number of codebooks = (fileSize - tableStart) / 4 - 1
    // (excluding the last entry which is tableStart itself)
    size_t tableSize = fileSize - tableStart;
    size_t numCodebooks = (tableSize / 4) - 1;

    g_codebookEntries.resize(numCodebooks);

    for (size_t i = 0; i < numCodebooks; i++)
    {
        uint32_t offset = ReadU32LE(g_codebookData.data() + tableStart + i * 4);
        uint32_t nextOffset = ReadU32LE(g_codebookData.data() + tableStart + (i + 1) * 4);

        g_codebookEntries[i].offset = offset;
        g_codebookEntries[i].size = nextOffset - offset;
    }

    g_initialized = true;
    return true;
}

bool IsCodebooksInitialized()
{
    return g_initialized;
}

const uint8_t* GetCodebookData(uint32_t codebookId, size_t& size)
{
    if (!g_initialized || codebookId >= g_codebookEntries.size())
    {
        size = 0;
        return nullptr;
    }

    const CodebookEntry& entry = g_codebookEntries[codebookId];
    size = entry.size;
    return g_codebookData.data() + entry.offset;
}

size_t GetCodebookCount()
{
    return g_codebookEntries.size();
}

bool IsValidCodebookId(uint32_t codebookId)
{
    return g_initialized && codebookId < g_codebookEntries.size();
}

} // namespace Audio