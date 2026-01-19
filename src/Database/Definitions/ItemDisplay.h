#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct ItemDisplay
    {
        uint32_t ID;
        std::wstring description;
        uint32_t item2TypeId;
        std::wstring objectModel;
        std::wstring objectModelL;
        std::wstring objectTexture0;
        std::wstring objectTexture1;
        uint32_t modelTextureIdObject00;
        uint32_t modelTextureIdObject01;
        std::wstring skinnedModel;
        std::wstring skinnedModelL;
        std::wstring skinnedTexture0;
        std::wstring skinnedTexture1;
        uint32_t modelTextureIdSkinned00;
        uint32_t modelTextureIdSkinned01;
        std::wstring attachedModel;
        uint32_t modelAttachmentIdAttached;
        std::wstring attachedTexture0;
        std::wstring attachedTexture1;
        uint32_t modelTextureIdAttached00;
        uint32_t modelTextureIdAttached01;
        uint32_t componentRegionFlags;
        uint32_t componentPriority;
        std::wstring skinMaskMap;
        std::wstring skinColorMap;
        std::wstring skinNormalMap;
        std::wstring skinDyeMap;
        std::wstring armorMaskMap;
        std::wstring armorColorMap;
        std::wstring armorNormalMap;
        std::wstring armorDyeMap;
        uint32_t modelMeshId00;
        uint32_t modelMeshId01;
        uint32_t modelMeshId02;
        uint32_t modelMeshId03;
        uint32_t soundImpactDescriptionId;
        uint32_t ItemVisualTypeId;
        uint32_t soundReplaceDescriptionId;
        uint32_t itemColorSetId;
        uint32_t dyeChannelFlags;
        uint32_t modelPoseId;
        float modelPoseBlend;
        uint32_t shaderPreset00;
        uint32_t shaderPreset01;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            description = file.getString(recordIndex, i++);
            item2TypeId = file.getUint(recordIndex, i++);
            objectModel = file.getString(recordIndex, i++);
            objectModelL = file.getString(recordIndex, i++);
            objectTexture0 = file.getString(recordIndex, i++);
            objectTexture1 = file.getString(recordIndex, i++);
            modelTextureIdObject00 = file.getUint(recordIndex, i++);
            modelTextureIdObject01 = file.getUint(recordIndex, i++);
            skinnedModel = file.getString(recordIndex, i++);
            skinnedModelL = file.getString(recordIndex, i++);
            skinnedTexture0 = file.getString(recordIndex, i++);
            skinnedTexture1 = file.getString(recordIndex, i++);
            modelTextureIdSkinned00 = file.getUint(recordIndex, i++);
            modelTextureIdSkinned01 = file.getUint(recordIndex, i++);
            attachedModel = file.getString(recordIndex, i++);
            modelAttachmentIdAttached = file.getUint(recordIndex, i++);
            attachedTexture0 = file.getString(recordIndex, i++);
            attachedTexture1 = file.getString(recordIndex, i++);
            modelTextureIdAttached00 = file.getUint(recordIndex, i++);
            modelTextureIdAttached01 = file.getUint(recordIndex, i++);
            componentRegionFlags = file.getUint(recordIndex, i++);
            componentPriority = file.getUint(recordIndex, i++);
            skinMaskMap = file.getString(recordIndex, i++);
            skinColorMap = file.getString(recordIndex, i++);
            skinNormalMap = file.getString(recordIndex, i++);
            skinDyeMap = file.getString(recordIndex, i++);
            armorMaskMap = file.getString(recordIndex, i++);
            armorColorMap = file.getString(recordIndex, i++);
            armorNormalMap = file.getString(recordIndex, i++);
            armorDyeMap = file.getString(recordIndex, i++);
            modelMeshId00 = file.getUint(recordIndex, i++);
            modelMeshId01 = file.getUint(recordIndex, i++);
            modelMeshId02 = file.getUint(recordIndex, i++);
            modelMeshId03 = file.getUint(recordIndex, i++);
            soundImpactDescriptionId = file.getUint(recordIndex, i++);
            ItemVisualTypeId = file.getUint(recordIndex, i++);
            soundReplaceDescriptionId = file.getUint(recordIndex, i++);
            itemColorSetId = file.getUint(recordIndex, i++);
            dyeChannelFlags = file.getUint(recordIndex, i++);
            modelPoseId = file.getUint(recordIndex, i++);
            modelPoseBlend = file.getFloat(recordIndex, i++);
            shaderPreset00 = file.getUint(recordIndex, i++);
            shaderPreset01 = file.getUint(recordIndex, i++);
        }
    };
}
