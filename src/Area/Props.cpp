#include "Props.h"
#include <cstring>

constexpr size_t PROP_ENTRY_SIZE = 104;

std::string ReadWideString(const uint8_t* data, size_t offset, size_t maxLen)
{
    std::string result;
    result.reserve(256);

    const uint16_t* wptr = reinterpret_cast<const uint16_t*>(data + offset);
    for (size_t i = 0; i < maxLen; i++)
    {
        uint16_t wc = wptr[i];
        if (wc == 0) break;

        if (wc < 128)
            result += static_cast<char>(wc);
        else
            result += '?';
    }

    return result;
}

void ParsePropsChunk(const uint8_t* data, size_t size, std::vector<Prop>& outProps, std::unordered_map<uint32_t, size_t>& outLookup)
{
    if (!data || size < 4) return;

    uint32_t propCount = static_cast<uint32_t>(data[0]) |
                        (static_cast<uint32_t>(data[1]) << 8) |
                        (static_cast<uint32_t>(data[2]) << 16) |
                        (static_cast<uint32_t>(data[3]) << 24);

    outProps.clear();
    outLookup.clear();
    outProps.reserve(propCount);

    const uint8_t* ptr = data + 4;

    for (uint32_t i = 0; i < propCount; i++)
    {
        if (4 + i * PROP_ENTRY_SIZE + PROP_ENTRY_SIZE > size)
            break;

        const uint8_t* propPtr = ptr + i * PROP_ENTRY_SIZE;
        Prop prop;

        size_t offset = 0;

        std::memcpy(&prop.uniqueID, propPtr + offset, 4); offset += 4;
        std::memcpy(&prop.someID, propPtr + offset, 4); offset += 4;
        std::memcpy(&prop.unk0, propPtr + offset, 4); offset += 4;
        std::memcpy(&prop.unk1, propPtr + offset, 4); offset += 4;

        int32_t modelType;
        std::memcpy(&modelType, propPtr + offset, 4); offset += 4;
        prop.modelType = static_cast<PropModelType>(modelType);

        std::memcpy(&prop.nameOffset, propPtr + offset, 4); offset += 4;
        std::memcpy(&prop.unkOffset, propPtr + offset, 4); offset += 4;
        std::memcpy(&prop.scale, propPtr + offset, 4); offset += 4;

        float qx, qy, qz, qw;
        std::memcpy(&qx, propPtr + offset, 4); offset += 4;
        std::memcpy(&qy, propPtr + offset, 4); offset += 4;
        std::memcpy(&qz, propPtr + offset, 4); offset += 4;
        std::memcpy(&qw, propPtr + offset, 4); offset += 4;
        prop.rotation = glm::quat(qw, qx, qy, qz);

        float px, py, pz;
        std::memcpy(&px, propPtr + offset, 4); offset += 4;
        std::memcpy(&py, propPtr + offset, 4); offset += 4;
        std::memcpy(&pz, propPtr + offset, 4); offset += 4;
        prop.position = glm::vec3(px, py, pz);

        std::memcpy(&prop.placement.minX, propPtr + offset, 2); offset += 2;
        std::memcpy(&prop.placement.minY, propPtr + offset, 2); offset += 2;
        std::memcpy(&prop.placement.maxX, propPtr + offset, 2); offset += 2;
        std::memcpy(&prop.placement.maxY, propPtr + offset, 2); offset += 2;

        std::memcpy(&prop.unk7, propPtr + offset, 4); offset += 4;
        std::memcpy(&prop.unk8, propPtr + offset, 4); offset += 4;
        std::memcpy(&prop.unk9, propPtr + offset, 4); offset += 4;

        std::memcpy(&prop.color0, propPtr + offset, 4); offset += 4;
        std::memcpy(&prop.color1, propPtr + offset, 4); offset += 4;

        std::memcpy(&prop.unk10, propPtr + offset, 4); offset += 4;
        std::memcpy(&prop.unk11, propPtr + offset, 4); offset += 4;

        std::memcpy(&prop.color2, propPtr + offset, 4); offset += 4;
        std::memcpy(&prop.unk12, propPtr + offset, 4); offset += 4;

        if (prop.nameOffset != 0)
        {
            size_t stringOffset = static_cast<size_t>(prop.nameOffset);
            if (stringOffset < size)
            {
                prop.path = ReadWideString(data, stringOffset, 512);
            }
        }

        outLookup[prop.uniqueID] = outProps.size();
        outProps.push_back(std::move(prop));
    }
}

void ParseCurtsChunk(const uint8_t* data, size_t size, std::vector<CurtData>& outCurts)
{
    if (!data || size < 4) return;

    uint32_t curtCount = static_cast<uint32_t>(data[0]) |
                        (static_cast<uint32_t>(data[1]) << 8) |
                        (static_cast<uint32_t>(data[2]) << 16) |
                        (static_cast<uint32_t>(data[3]) << 24);

    outCurts.clear();
    outCurts.reserve(curtCount);

    const uint8_t* ptr = data + 4;
    const uint8_t* end = data + size;

    for (uint32_t i = 0; i < curtCount && ptr + 24 <= end; i++)
    {
        CurtData curt;

        std::memcpy(&curt.unk0, ptr, 4); ptr += 4;
        std::memcpy(&curt.positionCount, ptr, 2); ptr += 2;
        std::memcpy(&curt.placement.minX, ptr, 2); ptr += 2;
        std::memcpy(&curt.placement.minY, ptr, 2); ptr += 2;
        std::memcpy(&curt.placement.maxX, ptr, 2); ptr += 2;
        std::memcpy(&curt.placement.maxY, ptr, 2); ptr += 2;
        std::memcpy(&curt.unk5, ptr, 2); ptr += 2;
        std::memcpy(&curt.positionOffset, ptr, 4); ptr += 4;
        std::memcpy(&curt.unk6, ptr, 4); ptr += 4;

        if (curt.positionOffset != 0 && curt.positionCount > 0)
        {
            size_t posOffset = static_cast<size_t>(curt.positionOffset);
            if (posOffset + curt.positionCount * 12 <= size)
            {
                curt.positions.resize(curt.positionCount);
                const uint8_t* posPtr = data + posOffset;
                for (int j = 0; j < curt.positionCount; j++)
                {
                    float x, y, z;
                    std::memcpy(&x, posPtr, 4); posPtr += 4;
                    std::memcpy(&y, posPtr, 4); posPtr += 4;
                    std::memcpy(&z, posPtr, 4); posPtr += 4;
                    curt.positions[j] = glm::vec3(x, y, z);
                }
            }
        }

        outCurts.push_back(std::move(curt));
    }
}