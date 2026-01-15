#pragma once
#include "M3Common.h"
#include "../Archive.h"
#include <memory>

class M3Loader {
public:
    static M3ModelData Load(const std::vector<uint8_t>& buffer);
    static M3ModelData LoadFromFile(const ArchivePtr& arc, const std::shared_ptr<FileEntry>& entry);

private:
    static float HalfToFloat(uint16_t h);
    static glm::vec3 ReadVertexV3(const uint8_t* data, uint8_t type, int& offset);
    static glm::vec2 ReadVertexV2(const uint8_t* data, uint8_t type, int& offset);
};
