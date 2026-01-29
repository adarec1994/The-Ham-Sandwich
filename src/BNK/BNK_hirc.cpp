#include "BNK_hirc.h"
#include "../Database/Tbl.h"
#include <cstring>
#include <algorithm>

namespace Bnk {

static inline uint8_t rd_u8(const uint8_t* p) { return *p; }
static inline uint32_t rd_u32(const uint8_t* p) { uint32_t v; std::memcpy(&v, p, 4); return v; }

bool HircParser::parse(const uint8_t* data, size_t size)
{
    mSounds.clear();
    mActions.clear();
    mEvents.clear();
    mContainers.clear();
    
    // Find HIRC section
    size_t pos = 0;
    const uint8_t* hircData = nullptr;
    size_t hircSize = 0;
    
    while (pos + 8 <= size) {
        char sectionId[5] = {0};
        std::memcpy(sectionId, data + pos, 4);
        uint32_t sectionSize = rd_u32(data + pos + 4);
        
        if (std::memcmp(sectionId, "HIRC", 4) == 0) {
            hircData = data + pos + 8;
            hircSize = sectionSize;
            break;
        }
        pos += 8 + sectionSize;
    }
    
    if (!hircData) return false;
    
    // Parse HIRC objects
    // First 4 bytes = object count
    if (hircSize < 4) return false;
    
    uint32_t objectCount = rd_u32(hircData);
    size_t offset = 4;
    
    for (uint32_t i = 0; i < objectCount && offset + 9 <= hircSize; ++i) {
        uint8_t type = rd_u8(hircData + offset);
        uint32_t objSize = rd_u32(hircData + offset + 1);
        uint32_t objId = rd_u32(hircData + offset + 5);
        
        const uint8_t* objData = hircData + offset + 9;
        size_t objDataSize = objSize - 4; // objSize includes the ID
        
        if (offset + 5 + objSize > hircSize) break;
        
        switch (static_cast<HircType>(type)) {
        case HircType::Sound:
            if (objDataSize >= 14) {
                SoundObject sound;
                sound.id = objId;
                // Skip plugin info (4 bytes)
                sound.streamType = rd_u8(objData + 4);
                sound.sourceId = rd_u32(objData + 5);
                sound.fileId = rd_u32(objData + 9);
                mSounds[objId] = sound;
            }
            break;
            
        case HircType::Action:
            if (objDataSize >= 6) {
                ActionObject action;
                action.id = objId;
                action.scope = rd_u8(objData + 0);
                action.type = rd_u8(objData + 1);
                action.targetId = rd_u32(objData + 2);
                mActions[objId] = action;
            }
            break;
            
        case HircType::Event:
            if (objDataSize >= 1) {
                EventObject event;
                event.id = objId;
                uint8_t actionCount = rd_u8(objData + 0);
                size_t actionOffset = 1;
                for (uint8_t j = 0; j < actionCount && actionOffset + 4 <= objDataSize; ++j) {
                    event.actionIds.push_back(rd_u32(objData + actionOffset));
                    actionOffset += 4;
                }
                mEvents[objId] = event;
            }
            break;
            
        case HircType::RandomContainer:
        case HircType::SwitchContainer:
        case HircType::ActorMixer:
        case HircType::MusicSegment:
        case HircType::MusicTrack:
        case HircType::MusicSwitchContainer:
        case HircType::MusicPlaylistContainer:
            // These are containers that can have children
            // The structure varies but children are usually listed
            // For simplicity, we'll try to find child references
            {
                ContainerObject container;
                container.id = objId;
                
                // Try to find NodeBaseParams which contains child list
                // This is a simplified approach - actual parsing is more complex
                // Look for a count followed by IDs in the data
                if (objDataSize > 20) {
                    // Skip NodeBaseParams header, look for children count
                    // This is approximate - structure varies by type
                    for (size_t scanPos = 0; scanPos + 8 < objDataSize; ++scanPos) {
                        uint32_t maybeCount = rd_u32(objData + scanPos);
                        if (maybeCount > 0 && maybeCount < 100 && scanPos + 4 + maybeCount * 4 <= objDataSize) {
                            // Check if following values look like valid IDs
                            bool validIds = true;
                            for (uint32_t k = 0; k < maybeCount && validIds; ++k) {
                                uint32_t maybeId = rd_u32(objData + scanPos + 4 + k * 4);
                                if (maybeId == 0) validIds = false;
                            }
                            if (validIds && maybeCount <= 50) {
                                for (uint32_t k = 0; k < maybeCount; ++k) {
                                    container.childIds.push_back(rd_u32(objData + scanPos + 4 + k * 4));
                                }
                                break;
                            }
                        }
                    }
                }
                if (!container.childIds.empty()) {
                    mContainers[objId] = container;
                }
            }
            break;
            
        default:
            break;
        }
        
        offset += 5 + objSize;
    }
    
    return true;
}

void HircParser::collectSourceIds(uint32_t objectId, std::unordered_set<uint32_t>& sourceIds,
                                   std::unordered_set<uint32_t>& visited) const
{
    if (visited.count(objectId)) return;
    visited.insert(objectId);
    
    // Check if it's a Sound object
    auto soundIt = mSounds.find(objectId);
    if (soundIt != mSounds.end()) {
        if (soundIt->second.sourceId != 0) {
            sourceIds.insert(soundIt->second.sourceId);
        }
        return;
    }
    
    // Check if it's a Container
    auto containerIt = mContainers.find(objectId);
    if (containerIt != mContainers.end()) {
        for (uint32_t childId : containerIt->second.childIds) {
            collectSourceIds(childId, sourceIds, visited);
        }
    }
}

std::unordered_set<uint32_t> HircParser::getSourceIdsForEvent(uint32_t eventId) const
{
    std::unordered_set<uint32_t> sourceIds;
    std::unordered_set<uint32_t> visited;
    
    auto eventIt = mEvents.find(eventId);
    if (eventIt == mEvents.end()) return sourceIds;
    
    for (uint32_t actionId : eventIt->second.actionIds) {
        auto actionIt = mActions.find(actionId);
        if (actionIt != mActions.end()) {
            collectSourceIds(actionIt->second.targetId, sourceIds, visited);
        }
    }
    
    return sourceIds;
}

std::unordered_map<uint32_t, std::unordered_set<uint32_t>> HircParser::buildSourceToEventMap() const
{
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> result;
    
    for (const auto& [eventId, event] : mEvents) {
        auto sourceIds = getSourceIdsForEvent(eventId);
        for (uint32_t sourceId : sourceIds) {
            result[sourceId].insert(eventId);
        }
    }
    
    return result;
}

// WemNameResolver implementation

bool WemNameResolver::loadSoundEventTable(const uint8_t* data, size_t size)
{
    Tbl::File tblFile;
    if (!tblFile.load(data, size)) return false;
    
    // Find column indices for 'hash' and 'name'
    const auto& cols = tblFile.getColumns();
    int hashCol = -1, nameCol = -1;
    
    for (size_t i = 0; i < cols.size(); ++i) {
        if (cols[i].name == L"hash") hashCol = static_cast<int>(i);
        else if (cols[i].name == L"name") nameCol = static_cast<int>(i);
    }
    
    if (hashCol < 0 || nameCol < 0) return false;
    
    mEventIdToName.clear();
    for (uint32_t row = 0; row < tblFile.getRecordCount(); ++row) {
        uint32_t hash = tblFile.getUint(row, hashCol);
        std::wstring name = tblFile.getString(row, nameCol);
        if (hash != 0 && !name.empty()) {
            mEventIdToName[hash] = name;
        }
    }
    
    printf("WemNameResolver: Loaded %zu event names from SoundEvent.tbl\n", mEventIdToName.size());
    return true;
}

bool WemNameResolver::loadEventsBnk(const uint8_t* data, size_t size)
{
    if (!mHircParser.parse(data, size)) {
        printf("WemNameResolver: Failed to parse Events.bnk HIRC\n");
        return false;
    }
    
    mSourceToEventIds = mHircParser.buildSourceToEventMap();
    
    printf("WemNameResolver: Parsed Events.bnk - %zu sounds, %zu actions, %zu events, %zu source mappings\n",
           mHircParser.getSounds().size(),
           mHircParser.getActions().size(),
           mHircParser.getEvents().size(),
           mSourceToEventIds.size());
    
    mLoaded = !mEventIdToName.empty() && !mSourceToEventIds.empty();
    return true;
}

static std::string wstring_to_utf8_local(const std::wstring& wstr)
{
    std::string result;
    for (wchar_t c : wstr) {
        if (c < 0x80) {
            result += static_cast<char>(c);
        } else if (c < 0x800) {
            result += static_cast<char>(0xC0 | (c >> 6));
            result += static_cast<char>(0x80 | (c & 0x3F));
        } else {
            result += static_cast<char>(0xE0 | (c >> 12));
            result += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (c & 0x3F));
        }
    }
    return result;
}

std::string WemNameResolver::resolve(uint32_t sourceMediaId) const
{
    if (!mLoaded) {
        return std::to_string(sourceMediaId);
    }
    
    auto sourceIt = mSourceToEventIds.find(sourceMediaId);
    if (sourceIt == mSourceToEventIds.end() || sourceIt->second.empty()) {
        return std::to_string(sourceMediaId);
    }
    
    // Get the first event ID that references this source
    uint32_t eventId = *sourceIt->second.begin();
    
    auto nameIt = mEventIdToName.find(eventId);
    if (nameIt == mEventIdToName.end()) {
        return std::to_string(sourceMediaId);
    }
    
    return wstring_to_utf8_local(nameIt->second);
}

} // namespace Bnk