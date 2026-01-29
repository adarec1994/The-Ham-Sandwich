#include "BNK_hirc.h"
#include "../Database/Tbl.h"
#include <cstring>
#include <cwctype>
#include <algorithm>
#include <functional>

namespace Bnk {

static inline uint8_t rd_u8(const uint8_t* p) { return *p; }
static inline uint32_t rd_u32(const uint8_t* p) { uint32_t v; std::memcpy(&v, p, 4); return v; }

bool HircParser::parse(const uint8_t* data, size_t size)
{
    mSounds.clear();
    mActions.clear();
    mEvents.clear();
    mContainers.clear();

    size_t pos = 0;
    const uint8_t* hircData = nullptr;
    size_t hircSize = 0;
    const uint8_t* didxData = nullptr;
    size_t didxSize = 0;

    while (pos + 8 <= size) {
        uint32_t sectionSize = rd_u32(data + pos + 4);
        if (std::memcmp(data + pos, "HIRC", 4) == 0) {
            hircData = data + pos + 8;
            hircSize = sectionSize;
        } else if (std::memcmp(data + pos, "DIDX", 4) == 0) {
            didxData = data + pos + 8;
            didxSize = sectionSize;
        }
        pos += 8 + sectionSize;
    }

    if (didxData && didxSize >= 12 && !hircData) {
        size_t numEntries = didxSize / 12;
        for (size_t i = 0; i < numEntries; i++) {
            uint32_t mediaId = rd_u32(didxData + i * 12);
            SoundObject sound;
            sound.id = mediaId;
            sound.sourceId = mediaId;
            sound.fileId = mediaId;
            sound.streamType = 0;
            mSounds[mediaId] = sound;
        }
        return true;
    }

    if (!hircData || hircSize < 4) return false;

    uint32_t objectCount = rd_u32(hircData);
    size_t offset = 4;

    for (uint32_t i = 0; i < objectCount && offset + 9 <= hircSize; ++i) {
        uint8_t type = rd_u8(hircData + offset);
        uint32_t objSize = rd_u32(hircData + offset + 1);
        uint32_t objId = rd_u32(hircData + offset + 5);

        if (offset + 5 + objSize > hircSize) break;

        const uint8_t* objData = hircData + offset + 9;
        size_t objDataSize = objSize > 4 ? objSize - 4 : 0;

        switch (static_cast<HircType>(type)) {
        case HircType::Sound:
            if (objDataSize >= 12) {
                SoundObject sound;
                sound.id = objId;
                sound.sourceId = rd_u32(objData + 8);
                sound.fileId = 0;
                sound.streamType = 0;
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
            if (objDataSize >= 4) {
                EventObject event;
                event.id = objId;
                uint32_t actionCount = rd_u32(objData);
                for (uint32_t j = 0; j < actionCount && 4 + j * 4 + 4 <= objDataSize; ++j) {
                    event.actionIds.push_back(rd_u32(objData + 4 + j * 4));
                }
                mEvents[objId] = event;
            }
            break;

        case HircType::RandomContainer:
        case HircType::SwitchContainer:
        case HircType::ActorMixer:
        case HircType::BlendContainer:
        case HircType::MusicSegment:
        case HircType::MusicTrack:
        case HircType::MusicSwitchContainer:
        case HircType::MusicPlaylistContainer:
            if (objDataSize >= 20) {
                ContainerObject container;
                container.id = objId;

                for (size_t scanPos = 4; scanPos + 4 < objDataSize; ++scanPos) {
                    uint32_t maybeCount = rd_u32(objData + scanPos);
                    if (maybeCount > 0 && maybeCount < 1000 &&
                        scanPos + 4 + maybeCount * 4 <= objDataSize) {
                        bool validIds = true;
                        std::vector<uint32_t> children;
                        for (uint32_t k = 0; k < maybeCount && validIds; ++k) {
                            uint32_t childId = rd_u32(objData + scanPos + 4 + k * 4);
                            if (childId == 0) validIds = false;
                            else children.push_back(childId);
                        }
                        if (validIds && !children.empty()) {
                            container.childIds = std::move(children);
                            break;
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

    auto soundIt = mSounds.find(objectId);
    if (soundIt != mSounds.end()) {
        if (soundIt->second.sourceId != 0) {
            sourceIds.insert(soundIt->second.sourceId);
        }
        return;
    }

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

static std::string wstring_to_utf8_local(const std::wstring& wstr)
{
    if (wstr.empty()) return "";
    std::string result;
    result.reserve(wstr.size() * 2);
    for (wchar_t wc : wstr) {
        if (wc < 0x80) {
            result.push_back(static_cast<char>(wc));
        } else if (wc < 0x800) {
            result.push_back(static_cast<char>(0xC0 | ((wc >> 6) & 0x1F)));
            result.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
        } else {
            result.push_back(static_cast<char>(0xE0 | ((wc >> 12) & 0x0F)));
            result.push_back(static_cast<char>(0x80 | ((wc >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
        }
    }
    return result;
}

bool WemNameResolver::loadSoundEventTable(const uint8_t* data, size_t size)
{
    Tbl::File tblFile;
    if (!tblFile.load(data, size)) {
        return false;
    }

    const auto& cols = tblFile.getColumns();

    int idCol = -1, nameCol = -1;
    for (size_t i = 0; i < cols.size(); ++i) {
        std::string colNameUtf8 = wstring_to_utf8_local(cols[i].name);
        std::string lower = colNameUtf8;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower == "id" || lower == "eventid" || lower == "event_id")
            idCol = static_cast<int>(i);
        else if (lower == "name" || lower == "eventname" || lower == "event_name")
            nameCol = static_cast<int>(i);
    }

    if (idCol < 0 || nameCol < 0) {
        return false;
    }

    for (uint32_t row = 0; row < tblFile.getRecordCount(); ++row) {
        uint32_t id = tblFile.getUint(row, idCol);
        std::wstring name = tblFile.getString(row, nameCol);
        if (id != 0 && !name.empty()) {
            mEventIdToName[id] = name;
        }
    }

    return true;
}

bool WemNameResolver::loadEventsBnk(const uint8_t* data, size_t size)
{
    if (!mEventsParser.parse(data, size)) {
        return false;
    }
    return true;
}

bool WemNameResolver::loadAudioBnk(const uint8_t* data, size_t size)
{
    HircParser parser;
    if (!parser.parse(data, size)) {
        return false;
    }

    for (const auto& [id, sound] : parser.getSounds()) {
        if (sound.sourceId != 0) {
            mSourceIdToSoundId[sound.sourceId] = id;
        }
    }

    for (const auto& [id, container] : parser.getContainers()) {
        mContainerToChildren[id] = container.childIds;
    }

    for (const auto& [id, action] : parser.getActions()) {
        mActionToTarget[id] = action.targetId;
    }

    for (const auto& [id, event] : parser.getEvents()) {
        mEventToActions[id] = event.actionIds;
    }

    return true;
}

void WemNameResolver::finalize()
{
    for (const auto& [id, action] : mEventsParser.getActions()) {
        mActionToTarget[id] = action.targetId;
    }

    for (const auto& [id, event] : mEventsParser.getEvents()) {
        mEventToActions[id] = event.actionIds;
    }

    for (const auto& [id, container] : mEventsParser.getContainers()) {
        if (mContainerToChildren.find(id) == mContainerToChildren.end()) {
            mContainerToChildren[id] = container.childIds;
        }
    }

    for (const auto& [eventId, actionIds] : mEventToActions) {
        for (uint32_t actionId : actionIds) {
            auto it = mActionToTarget.find(actionId);
            if (it != mActionToTarget.end()) {
                mSoundIdToEventId[it->second] = eventId;
            }
        }
    }

    std::function<void(uint32_t, uint32_t)> propagateDown;
    propagateDown = [&](uint32_t nodeId, uint32_t eventId) {
        if (mSoundIdToEventId.find(nodeId) == mSoundIdToEventId.end()) {
            mSoundIdToEventId[nodeId] = eventId;
        }
        auto it = mContainerToChildren.find(nodeId);
        if (it != mContainerToChildren.end()) {
            for (uint32_t childId : it->second) {
                propagateDown(childId, eventId);
            }
        }
    };

    auto directMappings = mSoundIdToEventId;
    for (const auto& [soundId, eventId] : directMappings) {
        propagateDown(soundId, eventId);
    }

    std::unordered_map<uint32_t, uint32_t> childToParent;
    for (const auto& [parentId, children] : mContainerToChildren) {
        for (uint32_t childId : children) {
            childToParent[childId] = parentId;
        }
    }

    std::function<uint32_t(uint32_t, int)> findEventViaParent;
    findEventViaParent = [&](uint32_t nodeId, int depth) -> uint32_t {
        if (depth > 20) return 0;
        auto evtIt = mSoundIdToEventId.find(nodeId);
        if (evtIt != mSoundIdToEventId.end()) return evtIt->second;
        auto parentIt = childToParent.find(nodeId);
        if (parentIt != childToParent.end()) {
            return findEventViaParent(parentIt->second, depth + 1);
        }
        return 0;
    };

    for (const auto& [sourceId, soundId] : mSourceIdToSoundId) {
        if (mSoundIdToEventId.find(soundId) == mSoundIdToEventId.end()) {
            uint32_t eventId = findEventViaParent(soundId, 0);
            if (eventId != 0) {
                mSoundIdToEventId[soundId] = eventId;
            }
        }
    }

    mLoaded = !mEventIdToName.empty() || !mSourceIdToSoundId.empty();
}

std::string WemNameResolver::resolve(uint32_t sourceMediaId) const
{
    auto srcIt = mSourceIdToSoundId.find(sourceMediaId);
    if (srcIt != mSourceIdToSoundId.end()) {
        uint32_t soundId = srcIt->second;

        auto evtIt = mSoundIdToEventId.find(soundId);
        if (evtIt != mSoundIdToEventId.end()) {
            uint32_t eventId = evtIt->second;

            auto nameIt = mEventIdToName.find(eventId);
            if (nameIt != mEventIdToName.end()) {
                return wstring_to_utf8_local(nameIt->second);
            }
        }
    }

    return std::to_string(sourceMediaId);
}

void WemNameResolver::clear()
{
    mLoaded = false;
    mEventIdToName.clear();
    mSourceIdToSoundId.clear();
    mSoundIdToEventId.clear();
    mContainerToChildren.clear();
    mActionToTarget.clear();
    mEventToActions.clear();
    mEventsParser = HircParser();
}

}