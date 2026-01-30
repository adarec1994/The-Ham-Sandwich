#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace Bnk {

enum class HircType : uint8_t {
    Sound = 2,
    Action = 3,
    Event = 4,
    RandomContainer = 5,
    SwitchContainer = 6,
    ActorMixer = 7,
    BlendContainer = 9,
    MusicSegment = 10,
    MusicTrack = 11,
    MusicSwitchContainer = 12,
    MusicPlaylistContainer = 13,
    AudioDevice = 14,  // Also ActorMixer in some versions
};

struct SoundObject {
    uint32_t id;
    uint32_t sourceId;
    uint32_t fileId;
    uint32_t parentId;
    uint8_t streamType;
};

struct ActionObject {
    uint32_t id;
    uint8_t scope;
    uint8_t type;
    uint32_t targetId;
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

    std::unordered_set<uint32_t> getSourceIdsForEvent(uint32_t eventId) const;
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> buildSourceToEventMap() const;

    const std::unordered_map<uint32_t, SoundObject>& getSounds() const { return mSounds; }
    const std::unordered_map<uint32_t, ActionObject>& getActions() const { return mActions; }
    const std::unordered_map<uint32_t, EventObject>& getEvents() const { return mEvents; }
    const std::unordered_map<uint32_t, ContainerObject>& getContainers() const { return mContainers; }

private:
    void collectSourceIds(uint32_t objectId, std::unordered_set<uint32_t>& sourceIds,
                          std::unordered_set<uint32_t>& visited) const;

    std::unordered_map<uint32_t, SoundObject> mSounds;
    std::unordered_map<uint32_t, ActionObject> mActions;
    std::unordered_map<uint32_t, EventObject> mEvents;
    std::unordered_map<uint32_t, ContainerObject> mContainers;
};

class WemNameResolver {
public:
    bool loadSoundEventTable(const uint8_t* data, size_t size);
    bool loadSoundEventCsv(const uint8_t* data, size_t size);
    bool loadSoundBankTable(const uint8_t* data, size_t size);
    bool loadEventsBnk(const uint8_t* data, size_t size);
    bool loadStructureBnk(const uint8_t* data, size_t size);
    bool loadAudioBnk(const uint8_t* data, size_t size);
    bool loadAudioBnk(const uint8_t* data, size_t size, const std::string& bankName);
    void finalize();
    void clear();

    std::string resolve(uint32_t sourceMediaId) const;

    bool isLoaded() const { return mLoaded; }
    size_t getEventCount() const { return mEventIdToName.size(); }
    size_t getSoundCount() const { return mSourceIdToSoundId.size(); }

    static uint32_t fnv1Hash(const std::string& name);

private:
    bool mLoaded = false;
    std::unordered_map<uint32_t, std::wstring> mEventIdToName;
    std::unordered_map<uint32_t, uint32_t> mSourceIdToSoundId;
    std::unordered_map<uint32_t, uint32_t> mSoundIdToEventId;
    std::unordered_map<uint32_t, std::vector<uint32_t>> mContainerToChildren;
    std::unordered_map<uint32_t, uint32_t> mActionToTarget;
    std::unordered_map<uint32_t, std::vector<uint32_t>> mEventToActions;
    HircParser mEventsParser;
    HircParser mStructureParser;
    std::unordered_map<uint32_t, uint32_t> mChildToParent;  // child -> parent container

    std::unordered_map<uint32_t, uint32_t> mWemIdToBankId;
    std::unordered_map<uint32_t, std::string> mBankIdToName;
};

}