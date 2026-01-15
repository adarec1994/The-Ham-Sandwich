#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct RealmDataCenter
    {
        uint32_t ID;
        uint32_t flags;
        uint32_t deploymentRegionEnum;
        uint32_t deploymentTypeEnum;
        uint32_t localizedTextId;
        std::wstring authServer;
        std::wstring ncClientAuthServer;
        std::wstring ncRedirectUrlTemplate;
        std::wstring ncRedirectUrlTemplateSignature;
        std::wstring ncAppGroupCode;
        uint32_t ncProgramAuth;
        std::wstring steamSignatureUrlTemplate;
        std::wstring steamNCoinUrlTemplate;
        std::wstring storeBannerDataUrlTemplate;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            flags = f.getUint(row, i++);
            deploymentRegionEnum = f.getUint(row, i++);
            deploymentTypeEnum = f.getUint(row, i++);
            localizedTextId = f.getUint(row, i++);
            authServer = f.getString(row, i++);
            ncClientAuthServer = f.getString(row, i++);
            ncRedirectUrlTemplate = f.getString(row, i++);
            ncRedirectUrlTemplateSignature = f.getString(row, i++);
            ncAppGroupCode = f.getString(row, i++);
            ncProgramAuth = f.getUint(row, i++);
            steamSignatureUrlTemplate = f.getString(row, i++);
            steamNCoinUrlTemplate = f.getString(row, i++);
            storeBannerDataUrlTemplate = f.getString(row, i++);
        }
    };
}
