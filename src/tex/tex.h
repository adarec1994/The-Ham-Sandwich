#pragma once
#include <cstdint>
#include <vector>
#include <array>
#include <string>
#include <memory>
#include <glad/glad.h>

struct AppState;
class Archive;
class FileEntry;

using ArchivePtr = std::shared_ptr<Archive>;
using FileEntryPtr = std::shared_ptr<FileEntry>;

namespace Tex
{
    enum class TextureType : uint32_t
    {
        Unknown,
        Jpeg1,
        Jpeg2,
        Jpeg3,
        Argb1,
        Argb2,
        Argb16,
        Rgb,
        Grayscale,
        Garbage,
        DXT1,
        DXT3,
        DXT5,
    };

    struct LayerInfo
    {
        int8_t quality = 0;
        int8_t hasReplacement = 0;
        int8_t replacement = 0;
    };

    struct Header
    {
        uint32_t version = 0;
        int32_t  width = 0;
        int32_t  height = 0;
        uint32_t depth = 0;
        uint32_t sides = 0;
        int32_t  mipCount = 0;
        uint32_t format = 0;

        uint32_t isCompressed = 0;
        uint32_t compressionFormat = 0;
        std::array<LayerInfo, 4> layerInfos{};
        uint32_t imageSizesCount = 0;
        std::array<uint32_t, 13> imageSizes{};
        int32_t unk = 0;

        TextureType textureType = TextureType::Unknown;

        bool read(const uint8_t* data, size_t size, size_t& offset);
    };

    struct ImageRGBA
    {
        int width = 0;
        int height = 0;
        std::vector<uint8_t> rgba;
    };

    class File
    {
    public:
        bool failedReading = false;
        Header header{};
        std::vector<std::vector<uint8_t>> mipData;

        bool readFromMemory(const uint8_t* data, size_t size);
        bool decodeLargestMipToRGBA(ImageRGBA& out) const;

    private:
        static std::vector<int> calculateDXTSizes(int mipLevels, int width, int height, int blockSize);
        static bool decodeDXT1(const uint8_t* src, int width, int height, std::vector<uint8_t>& outRGBA);
        static bool decodeDXT3(const uint8_t* src, int width, int height, std::vector<uint8_t>& outRGBA);
        static bool decodeDXT5(const uint8_t* src, int width, int height, std::vector<uint8_t>& outRGBA);
        static bool decodeArgb16(const uint8_t* src, int width, int height, std::vector<uint8_t>& outRGBA);
        static bool decodeGrayscale(const uint8_t* src, int width, int height, std::vector<uint8_t>& outRGBA);
        static bool decodeGarbage(const uint8_t* src, int width, int height, std::vector<uint8_t>& outRGBA);
    };

    struct PreviewState
    {
        bool open = false;
        std::string title;
        GLuint texture = 0;
        int texW = 0;
        int texH = 0;
        bool hasTexture = false;

        bool showR = true;
        bool showG = true;
        bool showB = true;
        bool showA = true;

        bool opaquePreview = false;

        void clearGL();
    };

    bool OpenTexPreviewFromEntry(AppState& state, const ArchivePtr& arc, const FileEntryPtr& fileEntry);
    void RenderTexPreviewWindow(PreviewState& ps);
}