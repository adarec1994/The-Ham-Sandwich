#include "BNK_hirc.h"
#include "../Database/Tbl.h"
#include <cstring>
#include <cwctype>
#include <algorithm>
#include <functional>
#include <sstream>
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
            sound.parentId = 0;
            mSounds[mediaId] = sound;
        }
        return true;
    }
    if (!hircData || hircSize < 4) return false;
    uint32_t objectCount = rd_u32(hircData);
    size_t offset = 4;
    std::unordered_set<uint32_t> allObjectIds;
    std::vector<std::tuple<uint8_t, uint32_t, const uint8_t*, size_t>> containerData;
    for (uint32_t i = 0; i < objectCount && offset + 9 <= hircSize; ++i) {
        uint8_t type = rd_u8(hircData + offset);
        uint32_t objSize = rd_u32(hircData + offset + 1);
        uint32_t objId = rd_u32(hircData + offset + 5);
        if (offset + 5 + objSize > hircSize) break;
        allObjectIds.insert(objId);
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
                if (objDataSize >= 27) {
                    sound.parentId = rd_u32(objData + 23);
                }
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
        case HircType::MusicTrack:
            if (objDataSize >= 16) {
                SoundObject sound;
                sound.id = objId;
                sound.sourceId = 0;
                sound.fileId = 0;
                sound.parentId = 0;
                sound.streamType = 0;
                uint32_t srcId = rd_u32(objData + 12);
                if (srcId != 0) {
                    sound.sourceId = srcId;
                    sound.fileId = srcId;
                    mSounds[objId] = sound;
                }
            }
            containerData.push_back({type, objId, objData, objDataSize});
            break;
        case HircType::RandomContainer:
        case HircType::SwitchContainer:
        case HircType::ActorMixer:
        case HircType::BlendContainer:
        case HircType::MusicSegment:
        case HircType::MusicSwitchContainer:
        case HircType::MusicPlaylistContainer:
            containerData.push_back({type, objId, objData, objDataSize});
            break;
        default:
            break;
        }
        offset += 5 + objSize;
    }
    for (const auto& [type, objId, objData, objDataSize] : containerData) {
        if (objDataSize >= 8) {
            ContainerObject container;
            container.id = objId;
            for (size_t scanPos = 0; scanPos + 4 <= objDataSize; ++scanPos) {
                uint32_t maybeCount = rd_u32(objData + scanPos);
                if (maybeCount > 0 && maybeCount <= 500 &&
                    scanPos + 4 + maybeCount * 4 <= objDataSize) {
                    std::vector<uint32_t> children;
                    bool allValid = true;
                    for (uint32_t k = 0; k < maybeCount; ++k) {
                        uint32_t childId = rd_u32(objData + scanPos + 4 + k * 4);
                        if (allObjectIds.count(childId) == 0) {
                            allValid = false;
                            break;
                        }
                        children.push_back(childId);
                    }
                    if (allValid && children.size() == maybeCount) {
                        container.childIds = std::move(children);
                        break;
                    }
                }
            }
            if (!container.childIds.empty()) {
                mContainers[objId] = container;
            }
        }
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
uint32_t WemNameResolver::fnv1Hash(const std::string& name)
{
    uint32_t hash = 2166136261u;
    for (char c : name) {
        char lower = (c >= 'A' && c <= 'Z') ? (c + 32) : c;
        hash = (hash * 16777619u) ^ static_cast<uint8_t>(lower);
    }
    return hash;
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
    std::string content(reinterpret_cast<const char*>(data), size);
    std::istringstream stream(content);
    std::string line;
    if (!std::getline(stream, line)) {
        return false;
    }
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
        line.pop_back();
    }
    char delimiter = ';';
    if (line.find(';') == std::string::npos && line.find(',') != std::string::npos) {
        delimiter = ',';
    }
    std::vector<std::string> headers = parseCsvLine(line, delimiter);
    int nameCol = -1, hashCol = -1;
    for (size_t i = 0; i < headers.size(); ++i) {
        std::string h = headers[i];
        std::string hLower = h;
        std::transform(hLower.begin(), hLower.end(), hLower.begin(), ::tolower);
        if (hLower == "name" || hLower == "eventname" || hLower == "event_name") {
            nameCol = static_cast<int>(i);
        } else if (hLower == "hash" || hLower == "eventhash" || hLower == "event_hash" || hLower == "wwiseid" || hLower == "wwise_id") {
            hashCol = static_cast<int>(i);
        }
    }
    if (nameCol < 0 || hashCol < 0) {
        return false;
    }
    while (std::getline(stream, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
            line.pop_back();
        }
        if (line.empty()) continue;
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
        }
    }
    return !mEventIdToName.empty();
}
bool WemNameResolver::loadSoundEventTable(const uint8_t* data, size_t size)
{
    if (size >= 4) {
        bool looksLikeCsv = false;
        for (size_t i = 0; i < std::min(size, static_cast<size_t>(256)); ++i) {
            if (data[i] == ';' || data[i] == ',') {
                looksLikeCsv = true;
                break;
            }
        }
        if (looksLikeCsv) {
            return loadSoundEventCsv(data, size);
        }
    }
    Tbl::File tblFile;
    if (!tblFile.load(data, size)) {
        return false;
    }
    const auto& cols = tblFile.getColumns();
    int hashCol = -1, nameCol = -1;
    for (size_t i = 0; i < cols.size(); ++i) {
        std::string colNameUtf8 = wstring_to_utf8_local(cols[i].name);
        std::string lower = colNameUtf8;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower == "hash" || lower == "eventhash" || lower == "event_hash" || lower == "wwiseid" || lower == "wwise_id")
            hashCol = static_cast<int>(i);
        else if (lower == "name" || lower == "eventname" || lower == "event_name")
            nameCol = static_cast<int>(i);
    }
    if (hashCol < 0 || nameCol < 0) {
        return false;
    }
    bool hashIsUlong = (hashCol < static_cast<int>(cols.size()) &&
                        cols[hashCol].dataType == Tbl::DataType::Ulong);
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
        }
    }
    return !mEventIdToName.empty();
}
bool WemNameResolver::loadEventsBnk(const uint8_t* data, size_t size)
{
    if (!mEventsParser.parse(data, size)) {
        return false;
    }
    return true;
}
bool WemNameResolver::loadStructureBnk(const uint8_t* data, size_t size)
{
    if (!mStructureParser.parse(data, size)) {
        return false;
    }
    std::unordered_set<uint32_t> allObjIds;
    for (const auto& [id, _] : mStructureParser.getSounds()) {
        allObjIds.insert(id);
    }
    for (const auto& [id, _] : mStructureParser.getContainers()) {
        allObjIds.insert(id);
    }
    for (const auto& [containerId, container] : mStructureParser.getContainers()) {
        for (uint32_t childId : container.childIds) {
            if (allObjIds.count(childId)) {
                mChildToParent[childId] = containerId;
            }
        }
    }
    return true;
}
bool WemNameResolver::loadSoundBankTable(const uint8_t* data, size_t size)
{
    if (size < 4 || std::memcmp(data, "LBTD", 4) != 0) {
        return false;
    }
    std::vector<std::string> bankNames;
    size_t pos = 0;
    while (pos + 2 < size) {
        if (data[pos] >= 'A' && data[pos] <= 'z' && data[pos+1] == 0) {
            size_t end = pos;
            while (end + 1 < size && end - pos < 200) {
                if (data[end] == 0 && data[end+1] == 0) break;
                end += 2;
            }
            if (end > pos + 4) {
                std::wstring ws;
                for (size_t i = pos; i < end; i += 2) {
                    wchar_t wc = data[i] | (data[i+1] << 8);
                    ws.push_back(wc);
                }
                std::string name = wstring_to_utf8_local(ws);
                if (name.length() >= 3 && name[0] >= 'A' && name[0] <= 'Z') {
                    bool isDuplicate = false;
                    for (const auto& existing : bankNames) {
                        if (existing.find(name) != std::string::npos || name.find(existing) != std::string::npos) {
                            if (existing != name) isDuplicate = true;
                        }
                    }
                    if (!isDuplicate) {
                        bankNames.push_back(name);
                    }
                }
                pos = end;
            }
        }
        pos++;
    }
    int mapped = 0;
    for (const auto& name : bankNames) {
        uint32_t hash = fnv1Hash(name);
        if (mBankIdToName.find(hash) == mBankIdToName.end()) {
            mBankIdToName[hash] = name;
            mapped++;
        }
    }
    return mapped > 0;
}
bool WemNameResolver::loadAudioBnk(const uint8_t* data, size_t size)
{
    return loadAudioBnk(data, size, "");
}
bool WemNameResolver::loadAudioBnk(const uint8_t* data, size_t size, const std::string& bankName)
{
    bool hasHirc = false;
    bool hasDidx = false;
    size_t checkPos = 0;
    while (checkPos + 8 <= size) {
        uint32_t secSize = rd_u32(data + checkPos + 4);
        if (std::memcmp(data + checkPos, "HIRC", 4) == 0) hasHirc = true;
        if (std::memcmp(data + checkPos, "DIDX", 4) == 0) hasDidx = true;
        checkPos += 8 + secSize;
    }
    if (hasHirc && !hasDidx) {
        HircParser testParser;
        if (testParser.parse(data, size) &&
            (testParser.getSounds().size() > 500 || testParser.getContainers().size() > 500)) {
            for (const auto& [id, sound] : testParser.getSounds()) {
                if (sound.sourceId != 0) {
                    mSourceIdToSoundId[sound.sourceId] = id;
                }
            }
            for (const auto& [containerId, container] : testParser.getContainers()) {
                for (uint32_t childId : container.childIds) {
                    mContainerToChildren[containerId].push_back(childId);
                }
            }
            std::unordered_set<uint32_t> allObjIds;
            for (const auto& [id, _] : testParser.getSounds()) {
                allObjIds.insert(id);
            }
            for (const auto& [id, _] : testParser.getContainers()) {
                allObjIds.insert(id);
            }
            for (const auto& [containerId, container] : testParser.getContainers()) {
                for (uint32_t childId : container.childIds) {
                    if (allObjIds.count(childId)) {
                        mChildToParent[childId] = containerId;
                    }
                }
            }
            for (const auto& [soundId, sound] : testParser.getSounds()) {
                if (sound.parentId != 0 && allObjIds.count(sound.parentId)) {
                    if (mChildToParent.find(soundId) == mChildToParent.end()) {
                        mChildToParent[soundId] = sound.parentId;
                    }
                }
            }
            return true;
        }
    }
    uint32_t bankId = 0;
    std::string resolvedBankName = bankName;
    if (size >= 16 && std::memcmp(data, "BKHD", 4) == 0) {
        bankId = rd_u32(data + 12);
        if (resolvedBankName.empty()) {
            auto it = mBankIdToName.find(bankId);
            if (it != mBankIdToName.end()) {
                resolvedBankName = it->second;
            }
        }
        if (!resolvedBankName.empty() && mBankIdToName.find(bankId) == mBankIdToName.end()) {
            mBankIdToName[bankId] = resolvedBankName;
        }
    }
    size_t pos = 0;
    while (pos + 8 <= size) {
        uint32_t sectionSize = rd_u32(data + pos + 4);
        if (std::memcmp(data + pos, "STID", 4) == 0 && sectionSize >= 8) {
            const uint8_t* stid = data + pos + 8;
            uint32_t stringType = rd_u32(stid);
            uint32_t numStrings = rd_u32(stid + 4);
            size_t strOffset = 8;
            for (uint32_t i = 0; i < numStrings && strOffset + 5 <= sectionSize; i++) {
                uint32_t id = rd_u32(stid + strOffset);
                uint8_t strLen = rd_u8(stid + strOffset + 4);
                if (strOffset + 5 + strLen <= sectionSize) {
                    std::string name(reinterpret_cast<const char*>(stid + strOffset + 5), strLen);
                    if (!name.empty() && id != 0) {
                        mEventIdToName[id] = utf8_to_wstring_local(name);
                        mBankIdToName[id] = name;
                    }
                }
                strOffset += 5 + strLen;
            }
        }
        pos += 8 + sectionSize;
    }
    pos = 0;
    std::vector<uint32_t> didxIds;
    while (pos + 8 <= size) {
        uint32_t sectionSize = rd_u32(data + pos + 4);
        if (std::memcmp(data + pos, "DIDX", 4) == 0) {
            const uint8_t* didx = data + pos + 8;
            size_t numEntries = sectionSize / 12;
            for (size_t i = 0; i < numEntries; i++) {
                uint32_t mediaId = rd_u32(didx + i * 12);
                didxIds.push_back(mediaId);
                if (bankId != 0) {
                    mWemIdToBankId[mediaId] = bankId;
                }
            }
        }
        pos += 8 + sectionSize;
    }
    HircParser parser;
    if (!parser.parse(data, size)) {
        return !didxIds.empty();
    }
    for (const auto& [id, sound] : parser.getSounds()) {
        if (sound.sourceId != 0) {
            auto existing = mSourceIdToSoundId.find(sound.sourceId);
            if (existing == mSourceIdToSoundId.end()) {
                mSourceIdToSoundId[sound.sourceId] = id;
            } else if (existing->second == sound.sourceId) {
                mSourceIdToSoundId[sound.sourceId] = id;
            }
            if (bankId != 0) {
                mWemIdToBankId[sound.sourceId] = bankId;
            }
        }
    }
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
    std::unordered_map<uint32_t, uint32_t> childToParent;
    for (const auto& [parentId, children] : mContainerToChildren) {
        for (uint32_t childId : children) {
            childToParent[childId] = parentId;
        }
    }
    for (const auto& [child, parent] : mChildToParent) {
        if (childToParent.find(child) == childToParent.end()) {
            childToParent[child] = parent;
        }
    }
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
    std::unordered_set<uint32_t> soundObjIds;
    for (const auto& [sourceId, soundId] : mSourceIdToSoundId) {
        soundObjIds.insert(soundId);
    }
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
    for (const auto& [sourceId, soundId] : mSourceIdToSoundId) {
        if (mSoundIdToEventId.find(soundId) == mSoundIdToEventId.end()) {
            std::unordered_set<uint32_t> visited;
            uint32_t eventId = findEventViaAncestors(soundId, visited);
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
    auto evtDirectIt = mSoundIdToEventId.find(sourceMediaId);
    if (evtDirectIt != mSoundIdToEventId.end()) {
        auto nameIt = mEventIdToName.find(evtDirectIt->second);
        if (nameIt != mEventIdToName.end()) {
            return wstring_to_utf8_local(nameIt->second);
        }
    }
    auto directNameIt = mEventIdToName.find(sourceMediaId);
    if (directNameIt != mEventIdToName.end()) {
        return wstring_to_utf8_local(directNameIt->second);
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
    mWemIdToBankId.clear();
    mBankIdToName.clear();
    mChildToParent.clear();
    mEventsParser = HircParser();
    mStructureParser = HircParser();
}
}