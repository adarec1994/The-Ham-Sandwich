#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace Bnk {

// HIRC object types we care about
enum class HircType : uint8_t {
    Sound = 2,
    Action = 3,
    Event = 4,
    RandomContainer = 5,
    SwitchContainer = 6,
    ActorMixer = 7,
    MusicSegment = 10,
    MusicTrack = 11,
    MusicSwitchContainer = 12,
    MusicPlaylistContainer = 13,
};

struct SoundObject {
    uint32_t id;
    uint32_t sourceId;      // The source media ID (WEM ID)
    uint32_t fileId;        // For streamed audio
    uint8_t streamType;     // 0=embedded, 1=streamed, 2=prefetch
};

struct ActionObject {
    uint32_t id;
    uint8_t scope;
    uint8_t type;
    uint32_t targetId;      // Object this action targets
};

struct EventObject {
    uint32_t id;
    std::vector<uint32_t> actionIds;
};

struct ContainerObject {
    uint32_t id;
    std::vector<uint32_t> childIds;
};

class HircParser {
public:
    bool parse(const uint8_t* data, size_t size);
    
    // Get all source media IDs that an event ultimately references
    std::unordered_set<uint32_t> getSourceIdsForEvent(uint32_t eventId) const;
    
    // Build reverse map: sourceMediaId -> set of eventIds that use it
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> buildSourceToEventMap() const;
    
    const std::unordered_map<uint32_t, SoundObject>& getSounds() const { return mSounds; }
    const std::unordered_map<uint32_t, ActionObject>& getActions() const { return mActions; }
    const std::unordered_map<uint32_t, EventObject>& getEvents() const { return mEvents; }
    
private:
    void collectSourceIds(uint32_t objectId, std::unordered_set<uint32_t>& sourceIds, 
                          std::unordered_set<uint32_t>& visited) const;
    
    std::unordered_map<uint32_t, SoundObject> mSounds;
    std::unordered_map<uint32_t, ActionObject> mActions;
    std::unordered_map<uint32_t, EventObject> mEvents;
    std::unordered_map<uint32_t, ContainerObject> mContainers;
};

// Main lookup class that combines everything
class WemNameResolver {
public:
    // Load SoundEvent.tbl to get eventHash -> eventName
    bool loadSoundEventTable(const uint8_t* data, size_t size);
    
    // Load Events.bnk to parse HIRC and build sourceId -> eventIds
    bool loadEventsBnk(const uint8_t* data, size_t size);
    
    // Resolve a source media ID to an event name
    std::string resolve(uint32_t sourceMediaId) const;
    
    // Check if loaded
    bool isLoaded() const { return mLoaded; }
    size_t getEventCount() const { return mEventIdToName.size(); }
    size_t getSourceMappingCount() const { return mSourceToEventIds.size(); }
    
private:
    bool mLoaded = false;
    std::unordered_map<uint32_t, std::wstring> mEventIdToName;  // eventHash -> eventName
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> mSourceToEventIds;  // sourceId -> eventIds
    HircParser mHircParser;
};

} // namespace Bnk