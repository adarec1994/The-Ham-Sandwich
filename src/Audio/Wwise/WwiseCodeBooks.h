#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace Audio
{

    class WwiseCodebooks
    {
    public:
        static WwiseCodebooks& Get();

        bool LoadFromFile(const std::string& path);
        bool IsLoaded() const;

        size_t GetCodebookCount() const;
        const uint8_t* GetCodebook(uint32_t id, size_t& outSize) const;

    private:
        WwiseCodebooks() = default;
    };

}