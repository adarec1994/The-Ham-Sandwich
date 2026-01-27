#include "WwiseCodebooks.h"
#include "WwiseCodebookData.h"

namespace Audio
{

    static constexpr size_t wvc_list_count_standard = sizeof(wvc_list_standard) / sizeof(wvc_list_standard[0]);
    static constexpr size_t wvc_list_count_aotuv603 = sizeof(wvc_list_aotuv603) / sizeof(wvc_list_aotuv603[0]);

    WwiseCodebooks& WwiseCodebooks::Get()
    {
        static WwiseCodebooks instance;
        return instance;
    }

    bool WwiseCodebooks::LoadFromFile(const std::string& path)
    {
        (void)path;
        return true;
    }

    bool WwiseCodebooks::IsLoaded() const
    {
        return true;
    }

    size_t WwiseCodebooks::GetCodebookCount() const
    {
        return wvc_list_count_aotuv603;
    }

    const uint8_t* WwiseCodebooks::GetCodebook(uint32_t id, size_t& outSize) const
    {
        for (size_t i = 0; i < wvc_list_count_aotuv603; i++)
        {
            if (wvc_list_aotuv603[i].id == id)
            {
                outSize = wvc_list_aotuv603[i].size;
                return wvc_list_aotuv603[i].codebook;
            }
        }

        for (size_t i = 0; i < wvc_list_count_standard; i++)
        {
            if (wvc_list_standard[i].id == id)
            {
                outSize = wvc_list_standard[i].size;
                return wvc_list_standard[i].codebook;
            }
        }

        outSize = 0;
        return nullptr;
    }

}