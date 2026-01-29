#include "BNK_hirc.h"
#include "../Database/Tbl.h"
#include <cstring>
#include <cwctype>
#include <algorithm>
#include <functional>
#include <sstream>
#include <fstream>

namespace Bnk {

static std::ofstream& getDebugLog() {
    static std::ofstream log("wem_resolver_debug.txt", std::ios::out | std::ios::trunc);
    return log;
}

#define WEM_DEBUG(x) do { getDebugLog() << x << std::endl; getDebugLog().flush(); } while(0)

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
                sound.sourceId = 0;
                sound.fileId = 0;
                sound.parentId = 0;
                sound.streamType = 0;

                uint32_t srcId = rd_u32(objData + 8);
                if (srcId == 0 && objDataSize >= 9) {
                    srcId = rd_u32(objData + 5);
                }
                sound.sourceId = srcId;
                sound.fileId = srcId;

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

static std::wstring utf8_to_wstring_local(const std::string& str)
{
    std::wstring result;
    result.reserve(str.size());
    size_t i = 0;
    while (i < str.size()) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        if (c < 0x80) {
            result.push_back(static_cast<wchar_t>(c));
            i++;
        } else if ((c & 0xE0) == 0xC0 && i + 1 < str.size()) {
            wchar_t wc = ((c & 0x1F) << 6) | (str[i + 1] & 0x3F);
            result.push_back(wc);
            i += 2;
        } else if ((c & 0xF0) == 0xE0 && i + 2 < str.size()) {
            wchar_t wc = ((c & 0x0F) << 12) | ((str[i + 1] & 0x3F) << 6) | (str[i + 2] & 0x3F);
            result.push_back(wc);
            i += 3;
        } else {
            result.push_back(L'?');
            i++;
        }
    }
    return result;
}

static std::vector<std::string> parseCsvLine(const std::string& line, char delimiter)
{
    std::vector<std::string> fields;
    std::string field;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                field += '"';
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == delimiter && !inQuotes) {
            fields.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    fields.push_back(field);
    return fields;
}

bool WemNameResolver::loadSoundEventCsv(const uint8_t* data, size_t size)
{
    WEM_DEBUG("=== loadSoundEventCsv called, size=" << size);

    std::string content(reinterpret_cast<const char*>(data), size);
    std::istringstream stream(content);
    std::string line;

    if (!std::getline(stream, line)) {
        WEM_DEBUG("  Failed to read header line");
        return false;
    }

    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
        line.pop_back();
    }

    WEM_DEBUG("  Header line: " << line.substr(0, 200));

    char delimiter = ';';
    if (line.find(';') == std::string::npos && line.find(',') != std::string::npos) {
        delimiter = ',';
    }
    WEM_DEBUG("  Using delimiter: '" << delimiter << "'");

    std::vector<std::string> headers = parseCsvLine(line, delimiter);
    WEM_DEBUG("  Found " << headers.size() << " columns");

    int nameCol = -1, hashCol = -1;
    for (size_t i = 0; i < headers.size(); ++i) {
        std::string h = headers[i];
        std::string hLower = h;
        std::transform(hLower.begin(), hLower.end(), hLower.begin(), ::tolower);
        WEM_DEBUG("    Col " << i << ": '" << h << "' (lower: '" << hLower << "')");
        if (hLower == "name" || hLower == "eventname" || hLower == "event_name") {
            nameCol = static_cast<int>(i);
        } else if (hLower == "hash" || hLower == "eventhash" || hLower == "event_hash" || hLower == "wwiseid" || hLower == "wwise_id") {
            hashCol = static_cast<int>(i);
        }
    }

    WEM_DEBUG("  nameCol=" << nameCol << " hashCol=" << hashCol);

    if (nameCol < 0 || hashCol < 0) {
        WEM_DEBUG("  FAILED: Missing required columns");
        return false;
    }

    int rowCount = 0;
    int loadedCount = 0;
    while (std::getline(stream, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
            line.pop_back();
        }
        if (line.empty()) continue;

        rowCount++;
        std::vector<std::string> fields = parseCsvLine(line, delimiter);
        if (fields.size() <= static_cast<size_t>(std::max(nameCol, hashCol))) {
            continue;
        }

        std::string nameStr = fields[nameCol];
        std::string hashStr = fields[hashCol];

        if (nameStr.empty() || hashStr.empty()) continue;

        uint32_t hash = 0;
        try {
            hash = static_cast<uint32_t>(std::stoul(hashStr));
        } catch (...) {
            continue;
        }

        if (hash != 0) {
            mEventIdToName[hash] = utf8_to_wstring_local(nameStr);
            loadedCount++;
            if (loadedCount <= 10) {
                WEM_DEBUG("  Sample: hash=" << hash << " name=" << nameStr);
            }
        }
    }

    WEM_DEBUG("  Processed " << rowCount << " rows, loaded " << loadedCount << " event names");
    WEM_DEBUG("  mEventIdToName.size() = " << mEventIdToName.size());

    return !mEventIdToName.empty();
}

bool WemNameResolver::loadSoundEventTable(const uint8_t* data, size_t size)
{
    WEM_DEBUG("=== loadSoundEventTable called, size=" << size);

    if (size >= 4) {
        bool looksLikeCsv = false;
        for (size_t i = 0; i < std::min(size, static_cast<size_t>(256)); ++i) {
            if (data[i] == ';' || data[i] == ',') {
                looksLikeCsv = true;
                break;
            }
        }
        if (looksLikeCsv) {
            WEM_DEBUG("  Detected CSV format, delegating to loadSoundEventCsv");
            return loadSoundEventCsv(data, size);
        }
    }

    WEM_DEBUG("  Attempting to parse as Tbl format");
    Tbl::File tblFile;
    if (!tblFile.load(data, size)) {
        WEM_DEBUG("  FAILED: Tbl::File::load() returned false");
        return false;
    }

    const auto& cols = tblFile.getColumns();
    WEM_DEBUG("  Tbl loaded, " << cols.size() << " columns, " << tblFile.getRecordCount() << " records");

    int hashCol = -1, nameCol = -1;
    for (size_t i = 0; i < cols.size(); ++i) {
        std::string colNameUtf8 = wstring_to_utf8_local(cols[i].name);
        std::string lower = colNameUtf8;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        WEM_DEBUG("    Col " << i << ": '" << colNameUtf8 << "' type=" << static_cast<int>(cols[i].dataType));
        if (lower == "hash" || lower == "eventhash" || lower == "event_hash" || lower == "wwiseid" || lower == "wwise_id")
            hashCol = static_cast<int>(i);
        else if (lower == "name" || lower == "eventname" || lower == "event_name")
            nameCol = static_cast<int>(i);
    }

    WEM_DEBUG("  hashCol=" << hashCol << " nameCol=" << nameCol);

    if (hashCol < 0 || nameCol < 0) {
        WEM_DEBUG("  FAILED: Missing required columns (hash or name)");
        return false;
    }

    bool hashIsUlong = (hashCol < static_cast<int>(cols.size()) &&
                        cols[hashCol].dataType == Tbl::DataType::Ulong);
    WEM_DEBUG("  hashIsUlong=" << hashIsUlong);

    int loadedCount = 0;
    for (uint32_t row = 0; row < tblFile.getRecordCount(); ++row) {
        uint32_t hash;
        if (hashIsUlong) {
            int64_t hashVal = tblFile.getInt64(row, hashCol);
            hash = static_cast<uint32_t>(hashVal);
        } else {
            hash = tblFile.getUint(row, hashCol);
        }
        std::wstring name = tblFile.getString(row, nameCol);
        if (hash != 0 && !name.empty()) {
            mEventIdToName[hash] = name;
            loadedCount++;
            if (loadedCount <= 10) {
                WEM_DEBUG("  Sample: hash=" << hash << " name=" << wstring_to_utf8_local(name));
            }
        }
    }

    WEM_DEBUG("  Loaded " << loadedCount << " event names from Tbl");
    WEM_DEBUG("  mEventIdToName.size() = " << mEventIdToName.size());

    return !mEventIdToName.empty();
}

bool WemNameResolver::loadEventsBnk(const uint8_t* data, size_t size)
{
    WEM_DEBUG("=== loadEventsBnk called, size=" << size);
    if (!mEventsParser.parse(data, size)) {
        WEM_DEBUG("  FAILED: mEventsParser.parse() returned false");
        return false;
    }
    WEM_DEBUG("  Parsed Events.bnk: " << mEventsParser.getEvents().size() << " events, "
              << mEventsParser.getActions().size() << " actions, "
              << mEventsParser.getSounds().size() << " sounds, "
              << mEventsParser.getContainers().size() << " containers");
    return true;
}

bool WemNameResolver::loadAudioBnk(const uint8_t* data, size_t size)
{
    WEM_DEBUG("=== loadAudioBnk called, size=" << size);

    size_t pos = 0;
    while (pos + 8 <= size) {
        uint32_t sectionSize = rd_u32(data + pos + 4);
        if (std::memcmp(data + pos, "STID", 4) == 0 && sectionSize >= 8) {
            const uint8_t* stid = data + pos + 8;
            uint32_t stringType = rd_u32(stid);
            uint32_t numStrings = rd_u32(stid + 4);
            WEM_DEBUG("  Found STID section: " << numStrings << " strings");
            size_t strOffset = 8;
            for (uint32_t i = 0; i < numStrings && strOffset + 5 <= sectionSize; i++) {
                uint32_t id = rd_u32(stid + strOffset);
                uint8_t strLen = rd_u8(stid + strOffset + 4);
                if (strOffset + 5 + strLen <= sectionSize) {
                    std::string name(reinterpret_cast<const char*>(stid + strOffset + 5), strLen);
                    if (!name.empty() && id != 0) {
                        mEventIdToName[id] = utf8_to_wstring_local(name);
                        if (i < 5) {
                            WEM_DEBUG("    STID: id=" << id << " name=" << name);
                        }
                    }
                }
                strOffset += 5 + strLen;
            }
        }
        pos += 8 + sectionSize;
    }

    HircParser parser;
    if (!parser.parse(data, size)) {
        WEM_DEBUG("  FAILED: parser.parse() returned false");
        return false;
    }

    WEM_DEBUG("  Parsed audio BNK: " << parser.getSounds().size() << " sounds, "
              << parser.getActions().size() << " actions, "
              << parser.getEvents().size() << " events, "
              << parser.getContainers().size() << " containers");

    std::unordered_set<uint32_t> soundSourceIds;
    for (const auto& [id, sound] : parser.getSounds()) {
        if (sound.sourceId != 0) {
            soundSourceIds.insert(sound.sourceId);
        }
    }
    WEM_DEBUG("  Unique sourceIds from Sound objects: " << soundSourceIds.size());

    pos = 0;
    std::unordered_set<uint32_t> didxIds;
    while (pos + 8 <= size) {
        uint32_t sectionSize = rd_u32(data + pos + 4);
        if (std::memcmp(data + pos, "DIDX", 4) == 0) {
            const uint8_t* didx = data + pos + 8;
            size_t numEntries = sectionSize / 12;
            for (size_t i = 0; i < numEntries; i++) {
                uint32_t mediaId = rd_u32(didx + i * 12);
                didxIds.insert(mediaId);
            }
        }
        pos += 8 + sectionSize;
    }

    if (!didxIds.empty()) {
        WEM_DEBUG("  DIDX entries (embedded wems): " << didxIds.size());
    }

    int newSources = 0;
    for (const auto& [id, sound] : parser.getSounds()) {
        if (sound.sourceId != 0) {
            if (mSourceIdToSoundId.find(sound.sourceId) == mSourceIdToSoundId.end()) {
                newSources++;
            }
            mSourceIdToSoundId[sound.sourceId] = id;
        }
    }
    WEM_DEBUG("  Added " << newSources << " new source->sound mappings, total=" << mSourceIdToSoundId.size());

    for (const auto& [id, container] : parser.getContainers()) {
        for (uint32_t childId : container.childIds) {
            mContainerToChildren[id].push_back(childId);
        }
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
    WEM_DEBUG("=== finalize called");
    WEM_DEBUG("  Before finalize:");
    WEM_DEBUG("    mEventIdToName.size() = " << mEventIdToName.size());
    WEM_DEBUG("    mSourceIdToSoundId.size() = " << mSourceIdToSoundId.size());
    WEM_DEBUG("    mSoundIdToEventId.size() = " << mSoundIdToEventId.size());
    WEM_DEBUG("    mContainerToChildren.size() = " << mContainerToChildren.size());
    WEM_DEBUG("    mActionToTarget.size() = " << mActionToTarget.size());
    WEM_DEBUG("    mEventToActions.size() = " << mEventToActions.size());

    for (const auto& [id, action] : mEventsParser.getActions()) {
        mActionToTarget[id] = action.targetId;
    }

    for (const auto& [id, event] : mEventsParser.getEvents()) {
        mEventToActions[id] = event.actionIds;
    }

    for (const auto& [id, container] : mEventsParser.getContainers()) {
        for (uint32_t childId : container.childIds) {
            mContainerToChildren[id].push_back(childId);
        }
    }

    WEM_DEBUG("  After merging Events.bnk data:");
    WEM_DEBUG("    mContainerToChildren.size() = " << mContainerToChildren.size());

    std::unordered_map<uint32_t, uint32_t> childToParent;
    for (const auto& [parentId, children] : mContainerToChildren) {
        for (uint32_t childId : children) {
            childToParent[childId] = parentId;
        }
    }
    WEM_DEBUG("  childToParent.size() = " << childToParent.size());

    std::unordered_set<uint32_t> allActionTargets;
    for (const auto& [eventId, actionIds] : mEventToActions) {
        for (uint32_t actionId : actionIds) {
            auto it = mActionToTarget.find(actionId);
            if (it != mActionToTarget.end()) {
                allActionTargets.insert(it->second);
                mSoundIdToEventId[it->second] = eventId;
            }
        }
    }
    WEM_DEBUG("  Direct action targets: " << allActionTargets.size());

    std::function<void(uint32_t, uint32_t, std::unordered_set<uint32_t>&)> collectAllDescendants;
    collectAllDescendants = [&](uint32_t nodeId, uint32_t eventId, std::unordered_set<uint32_t>& visited) {
        if (visited.count(nodeId)) return;
        visited.insert(nodeId);
        mSoundIdToEventId[nodeId] = eventId;
        auto it = mContainerToChildren.find(nodeId);
        if (it != mContainerToChildren.end()) {
            for (uint32_t childId : it->second) {
                collectAllDescendants(childId, eventId, visited);
            }
        }
    };

    for (uint32_t target : allActionTargets) {
        uint32_t eventId = mSoundIdToEventId[target];
        std::unordered_set<uint32_t> visited;
        collectAllDescendants(target, eventId, visited);
    }
    WEM_DEBUG("  After propagateDown: mSoundIdToEventId.size() = " << mSoundIdToEventId.size());

    std::function<uint32_t(uint32_t, std::unordered_set<uint32_t>&)> findEventViaAncestors;
    findEventViaAncestors = [&](uint32_t nodeId, std::unordered_set<uint32_t>& visited) -> uint32_t {
        if (visited.count(nodeId)) return 0;
        visited.insert(nodeId);
        auto evtIt = mSoundIdToEventId.find(nodeId);
        if (evtIt != mSoundIdToEventId.end()) return evtIt->second;
        auto parentIt = childToParent.find(nodeId);
        if (parentIt != childToParent.end()) {
            return findEventViaAncestors(parentIt->second, visited);
        }
        return 0;
    };

    int parentMappings = 0;
    int noParentFound = 0;
    int hasParentNoEvent = 0;
    for (const auto& [sourceId, soundId] : mSourceIdToSoundId) {
        if (mSoundIdToEventId.find(soundId) == mSoundIdToEventId.end()) {
            std::unordered_set<uint32_t> visited;
            uint32_t eventId = findEventViaAncestors(soundId, visited);
            if (eventId != 0) {
                mSoundIdToEventId[soundId] = eventId;
                parentMappings++;
            } else {
                auto parentIt = childToParent.find(soundId);
                if (parentIt == childToParent.end()) {
                    noParentFound++;
                } else {
                    hasParentNoEvent++;
                }
            }
        }
    }
    WEM_DEBUG("  Found " << parentMappings << " via parent traversal");
    WEM_DEBUG("  No parent in hierarchy: " << noParentFound);
    WEM_DEBUG("  Has parent but no event found: " << hasParentNoEvent);
    WEM_DEBUG("  Final mSoundIdToEventId.size() = " << mSoundIdToEventId.size());

    int resolvable = 0;
    int noEventMapping = 0;
    int noNameMapping = 0;
    for (const auto& [sourceId, soundId] : mSourceIdToSoundId) {
        auto evtIt = mSoundIdToEventId.find(soundId);
        if (evtIt != mSoundIdToEventId.end()) {
            auto nameIt = mEventIdToName.find(evtIt->second);
            if (nameIt != mEventIdToName.end()) {
                resolvable++;
            } else {
                noNameMapping++;
            }
        } else {
            noEventMapping++;
        }
    }
    WEM_DEBUG("  Resolution breakdown:");
    WEM_DEBUG("    Fully resolvable: " << resolvable);
    WEM_DEBUG("    Has event but no name: " << noNameMapping);
    WEM_DEBUG("    No event mapping: " << noEventMapping);

    mLoaded = !mEventIdToName.empty() || !mSourceIdToSoundId.empty();
    WEM_DEBUG("  mLoaded = " << mLoaded);
}

std::string WemNameResolver::resolve(uint32_t sourceMediaId) const
{
    static int resolveCallCount = 0;
    static int resolvedCount = 0;
    static int unresolvedCount = 0;
    resolveCallCount++;

    auto srcIt = mSourceIdToSoundId.find(sourceMediaId);
    if (srcIt != mSourceIdToSoundId.end()) {
        uint32_t soundId = srcIt->second;

        auto evtIt = mSoundIdToEventId.find(soundId);
        if (evtIt != mSoundIdToEventId.end()) {
            uint32_t eventId = evtIt->second;

            auto nameIt = mEventIdToName.find(eventId);
            if (nameIt != mEventIdToName.end()) {
                resolvedCount++;
                return wstring_to_utf8_local(nameIt->second);
            }
        }
    }

    auto evtDirectIt = mSoundIdToEventId.find(sourceMediaId);
    if (evtDirectIt != mSoundIdToEventId.end()) {
        auto nameIt = mEventIdToName.find(evtDirectIt->second);
        if (nameIt != mEventIdToName.end()) {
            resolvedCount++;
            return wstring_to_utf8_local(nameIt->second);
        }
    }

    auto directNameIt = mEventIdToName.find(sourceMediaId);
    if (directNameIt != mEventIdToName.end()) {
        resolvedCount++;
        return wstring_to_utf8_local(directNameIt->second);
    }

    unresolvedCount++;
    if (unresolvedCount <= 50) {
        WEM_DEBUG("resolve(" << sourceMediaId << "): UNRESOLVED");
    }
    if (resolveCallCount % 1000 == 0) {
        WEM_DEBUG("resolve stats: calls=" << resolveCallCount << " resolved=" << resolvedCount << " unresolved=" << unresolvedCount);
    }

    return std::to_string(sourceMediaId);
}

void WemNameResolver::clear()
{
    WEM_DEBUG("=== clear() called");
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