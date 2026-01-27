#pragma once
// WwiseCodebooks.h - Load Wwise Vorbis codebook data
// Supports packed_codebooks_aoTuV_603.bin format

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace Audio
{

    // Codebook entry info
    struct CodebookEntry
    {
        uint32_t offset;
        uint32_t size;
    };

    // Initialize codebook database from packed file
    // Returns true if successful
    // Path should point to packed_codebooks_aoTuV_603.bin
    bool InitializeCodebooks(const std::string& path);

    // Check if codebooks are loaded
    bool IsCodebooksInitialized();

    // Get codebook data for a given ID
    // Returns pointer to codebook data and sets size, or nullptr if not found
    const uint8_t* GetCodebookData(uint32_t codebookId, size_t& size);

    // Get total number of available codebooks
    size_t GetCodebookCount();

    // Check if a codebook ID is valid
    bool IsValidCodebookId(uint32_t codebookId);

} // namespace Audio