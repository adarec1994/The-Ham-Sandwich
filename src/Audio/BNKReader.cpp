#include "BNKReader.h"
#include <fstream>
#include <cstring>
#include <algorithm>

namespace Audio
{
    static inline uint8_t ReadU8(const uint8_t* data) { return data[0]; }

    static inline uint16_t ReadU16(const uint8_t* data)
    {
        return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
    }

    static inline uint32_t ReadU32(const uint8_t* data)
    {
        return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) |
               (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
    }

    static inline bool MatchMagic(const uint8_t* data, const char* magic)
    {
        return std::memcmp(data, magic, 4) == 0;
    }

    bool SoundBank::Load(const std::string& filepath)
    {
        m_filepath = filepath;
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            m_lastError = "Failed to open file: " + filepath;
            return false;
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        if (fileSize < 8)
        {
            m_lastError = "File too small";
            return false;
        }

        file.seekg(0, std::ios::beg);
        m_fileData.resize(fileSize);
        file.read(reinterpret_cast<char*>(m_fileData.data()), fileSize);
        file.close();

        return LoadFromMemory(m_fileData.data(), m_fileData.size());
    }

    bool SoundBank::LoadFromMemory(const uint8_t* data, size_t size)
    {
        if (!data || size < 8)
        {
            m_lastError = "Invalid data or size";
            return false;
        }

        if (m_fileData.empty())
        {
            m_fileData.assign(data, data + size);
            data = m_fileData.data();
        }

        return ParseChunks(data, size);
    }

    bool SoundBank::ParseChunks(const uint8_t* data, size_t size)
    {
        size_t offset = 0;

        while (offset + 8 <= size)
        {
            char magic[5] = {0};
            std::memcpy(magic, data + offset, 4);
            uint32_t chunkSize = ReadU32(data + offset + 4);
            offset += 8;

            if (offset + chunkSize > size)
            {
                m_lastError = "Chunk size exceeds file bounds";
                return false;
            }

            const uint8_t* chunkData = data + offset;

            if (MatchMagic(reinterpret_cast<const uint8_t*>(magic), "BKHD"))
            {
                if (!ParseBKHD(chunkData, chunkSize)) return false;
            }
            else if (MatchMagic(reinterpret_cast<const uint8_t*>(magic), "DIDX"))
            {
                if (!ParseDIDX(chunkData, chunkSize)) return false;
            }
            else if (MatchMagic(reinterpret_cast<const uint8_t*>(magic), "DATA"))
            {
                m_dataOffset = offset;
                if (!ParseDATA(chunkData, chunkSize)) return false;
            }
            else if (MatchMagic(reinterpret_cast<const uint8_t*>(magic), "HIRC"))
            {
                if (!ParseHIRC(chunkData, chunkSize)) return false;
            }
            else if (MatchMagic(reinterpret_cast<const uint8_t*>(magic), "STID"))
            {
                if (!ParseSTID(chunkData, chunkSize)) return false;
            }
            else if (MatchMagic(reinterpret_cast<const uint8_t*>(magic), "STMG"))
            {
                if (!ParseSTMG(chunkData, chunkSize)) return false;
            }

            offset += chunkSize;
        }

        return true;
    }

    bool SoundBank::ParseBKHD(const uint8_t* data, size_t size)
    {
        if (size < 8)
        {
            m_lastError = "BKHD section too small";
            return false;
        }

        m_header.version = ReadU32(data);
        m_header.bankId = ReadU32(data + 4);
        if (size >= 12) m_header.languageId = ReadU32(data + 8);
        if (size >= 16) m_header.feedback = ReadU32(data + 12);
        if (size >= 20) m_header.projectId = ReadU32(data + 16);

        return true;
    }

    bool SoundBank::ParseDIDX(const uint8_t* data, size_t size)
    {
        const size_t entrySize = 12;
        size_t count = size / entrySize;
        m_wemDescriptors.reserve(count);

        for (size_t i = 0; i < count; ++i)
        {
            const uint8_t* entry = data + (i * entrySize);
            WEMDescriptor desc;
            desc.id = ReadU32(entry);
            desc.offset = ReadU32(entry + 4);
            desc.size = ReadU32(entry + 8);
            m_wemDescriptors.push_back(desc);
        }

        return true;
    }

    bool SoundBank::ParseDATA(const uint8_t* data, size_t size)
    {
        m_dataSection.assign(data, data + size);
        return true;
    }

    bool SoundBank::ParseHIRC(const uint8_t* data, size_t size)
    {
        if (size < 4)
        {
            m_lastError = "HIRC section too small";
            return false;
        }

        uint32_t objectCount = ReadU32(data);
        size_t offset = 4;
        m_hircObjects.reserve(objectCount);

        for (uint32_t i = 0; i < objectCount && offset < size; ++i)
        {
            size_t bytesRead = 0;
            auto obj = ParseHIRCObject(data + offset, size - offset, bytesRead);
            if (obj)
            {
                m_hircById[obj->id] = obj.get();
                m_hircObjects.push_back(std::move(obj));
            }
            offset += bytesRead;
        }

        return true;
    }

    std::unique_ptr<HIRCObject> SoundBank::ParseHIRCObject(const uint8_t* data, size_t maxSize, size_t& bytesRead)
    {
        bytesRead = 0;
        if (maxSize < 5) return nullptr;

        uint8_t typeId = ReadU8(data);
        uint32_t objSize = ReadU32(data + 1);
        bytesRead = 5 + objSize;

        if (bytesRead > maxSize) return nullptr;

        const uint8_t* objData = data + 5;
        uint32_t objId = (objSize >= 4) ? ReadU32(objData) : 0;
        HIRCObjectType type = static_cast<HIRCObjectType>(typeId);

        std::unique_ptr<HIRCObject> obj;

        switch (type)
        {
            case HIRCObjectType::Sound:
            {
                auto sound = std::make_unique<SoundObject>();
                sound->type = type;
                sound->size = objSize;
                sound->id = objId;
                ParseSoundObject(sound.get(), objData, objSize);
                obj = std::move(sound);
                break;
            }
            case HIRCObjectType::Event:
            {
                auto event = std::make_unique<EventObject>();
                event->type = type;
                event->size = objSize;
                event->id = objId;
                ParseEventObject(event.get(), objData, objSize);
                obj = std::move(event);
                break;
            }
            case HIRCObjectType::Action:
            {
                auto action = std::make_unique<ActionObject>();
                action->type = type;
                action->size = objSize;
                action->id = objId;
                ParseActionObject(action.get(), objData, objSize);
                obj = std::move(action);
                break;
            }
            case HIRCObjectType::MusicTrack:
            {
                auto track = std::make_unique<MusicTrackObject>();
                track->type = type;
                track->size = objSize;
                track->id = objId;
                ParseMusicTrackObject(track.get(), objData, objSize);
                obj = std::move(track);
                break;
            }
            default:
            {
                obj = std::make_unique<HIRCObject>();
                obj->type = type;
                obj->size = objSize;
                obj->id = objId;
                break;
            }
        }

        if (obj && objSize > 0)
        {
            obj->data.assign(objData, objData + objSize);
        }

        return obj;
    }

    void SoundBank::ParseSoundObject(SoundObject* obj, const uint8_t* data, size_t size)
    {
        if (size < 14) return;

        size_t offset = 4;
        obj->pluginId = ReadU32(data + offset); offset += 4;
        obj->streamType = ReadU8(data + offset); offset += 1;
        obj->sourceId = ReadU32(data + offset); offset += 4;
        obj->fileId = ReadU32(data + offset); offset += 4;

        if ((obj->streamType == 0 || obj->streamType == 2) && offset + 8 <= size)
        {
            obj->fileOffset = ReadU32(data + offset); offset += 4;
            obj->fileSize = ReadU32(data + offset);
        }
    }

    void SoundBank::ParseEventObject(EventObject* obj, const uint8_t* data, size_t size)
    {
        if (size < 5) return;

        size_t offset = 4;
        uint32_t actionCount = 0;

        if (m_header.version >= 134)
        {
            actionCount = ReadU8(data + offset);
            offset += 1;
        }
        else
        {
            actionCount = ReadU32(data + offset);
            offset += 4;
        }

        obj->actionIds.reserve(actionCount);
        for (uint32_t i = 0; i < actionCount && offset + 4 <= size; ++i)
        {
            obj->actionIds.push_back(ReadU32(data + offset));
            offset += 4;
        }
    }

    void SoundBank::ParseActionObject(ActionObject* obj, const uint8_t* data, size_t size)
    {
        if (size < 7) return;

        size_t offset = 4;
        obj->scope = ReadU8(data + offset); offset += 1;
        obj->actionType = ReadU8(data + offset); offset += 1;
        obj->targetId = ReadU32(data + offset);
    }

    void SoundBank::ParseMusicTrackObject(MusicTrackObject* obj, const uint8_t* data, size_t size)
    {
        if (size < 8) return;

        size_t offset = 4;
        if (offset + 4 <= size)
        {
            uint32_t sourceCount = ReadU32(data + offset);
            offset += 4;
            obj->sourceIds.reserve(sourceCount);

            for (uint32_t i = 0; i < sourceCount && offset + 14 <= size; ++i)
            {
                uint32_t sourceId = ReadU32(data + offset + 4);
                obj->sourceIds.push_back(sourceId);
                offset += 14;
            }
        }
    }

    bool SoundBank::ParseSTID(const uint8_t* data, size_t size)
    {
        if (size < 8) return true;

        size_t offset = 4;
        uint32_t stringCount = ReadU32(data + offset);
        offset += 4;

        for (uint32_t i = 0; i < stringCount && offset < size; ++i)
        {
            if (offset + 5 > size) break;

            uint32_t id = ReadU32(data + offset);
            offset += 4;

            uint8_t strLen = ReadU8(data + offset);
            offset += 1;

            if (offset + strLen > size) break;

            std::string name(reinterpret_cast<const char*>(data + offset), strLen);
            offset += strLen;

            m_stringIds[id] = name;
            if (i == 0) m_bankName = name;
        }

        return true;
    }

    bool SoundBank::ParseSTMG(const uint8_t* data, size_t size)
    {
        return true;
    }

    const WEMDescriptor* SoundBank::GetWEMDescriptor(uint32_t id) const
    {
        for (const auto& desc : m_wemDescriptors)
        {
            if (desc.id == id) return &desc;
        }
        return nullptr;
    }

    bool SoundBank::ExtractWEM(uint32_t id, std::vector<uint8_t>& outData) const
    {
        const WEMDescriptor* desc = GetWEMDescriptor(id);
        if (!desc) return false;
        if (desc->offset + desc->size > m_dataSection.size()) return false;

        outData.assign(m_dataSection.begin() + desc->offset,
                       m_dataSection.begin() + desc->offset + desc->size);
        return true;
    }

    bool SoundBank::ExtractWEMByIndex(size_t index, std::vector<uint8_t>& outData) const
    {
        if (index >= m_wemDescriptors.size()) return false;
        return ExtractWEM(m_wemDescriptors[index].id, outData);
    }

    const HIRCObject* SoundBank::GetHIRCObject(uint32_t id) const
    {
        auto it = m_hircById.find(id);
        return (it != m_hircById.end()) ? it->second : nullptr;
    }

    std::vector<const HIRCObject*> SoundBank::GetObjectsByType(HIRCObjectType type) const
    {
        std::vector<const HIRCObject*> result;
        for (const auto& obj : m_hircObjects)
        {
            if (obj->type == type) result.push_back(obj.get());
        }
        return result;
    }

    std::vector<const EventObject*> SoundBank::GetEvents() const
    {
        std::vector<const EventObject*> result;
        for (const auto& obj : m_hircObjects)
        {
            if (obj->type == HIRCObjectType::Event)
                result.push_back(static_cast<const EventObject*>(obj.get()));
        }
        return result;
    }

    std::vector<const SoundObject*> SoundBank::GetSounds() const
    {
        std::vector<const SoundObject*> result;
        for (const auto& obj : m_hircObjects)
        {
            if (obj->type == HIRCObjectType::Sound)
                result.push_back(static_cast<const SoundObject*>(obj.get()));
        }
        return result;
    }

    const std::string* SoundBank::GetStringById(uint32_t id) const
    {
        auto it = m_stringIds.find(id);
        return (it != m_stringIds.end()) ? &it->second : nullptr;
    }

    const char* GetHIRCTypeName(HIRCObjectType type)
    {
        switch (type)
        {
            case HIRCObjectType::State: return "State";
            case HIRCObjectType::Sound: return "Sound";
            case HIRCObjectType::Action: return "Action";
            case HIRCObjectType::Event: return "Event";
            case HIRCObjectType::RandomSequenceContainer: return "RandomSequenceContainer";
            case HIRCObjectType::SwitchContainer: return "SwitchContainer";
            case HIRCObjectType::ActorMixer: return "ActorMixer";
            case HIRCObjectType::AudioBus: return "AudioBus";
            case HIRCObjectType::BlendContainer: return "BlendContainer";
            case HIRCObjectType::MusicSegment: return "MusicSegment";
            case HIRCObjectType::MusicTrack: return "MusicTrack";
            case HIRCObjectType::MusicSwitchContainer: return "MusicSwitchContainer";
            case HIRCObjectType::MusicPlaylistContainer: return "MusicPlaylistContainer";
            case HIRCObjectType::Attenuation: return "Attenuation";
            case HIRCObjectType::DialogueEvent: return "DialogueEvent";
            case HIRCObjectType::MotionBus: return "MotionBus";
            case HIRCObjectType::MotionFX: return "MotionFX";
            case HIRCObjectType::Effect: return "Effect";
            case HIRCObjectType::AuxBus: return "AuxBus";
            default: return "Unknown";
        }
    }

    std::string GetVersionString(uint32_t version)
    {
        if (version >= 150) return "2024+";
        if (version >= 145) return "2023+";
        if (version >= 140) return "2022+";
        if (version >= 135) return "2021+";
        if (version >= 134) return "2019.2+";
        if (version >= 132) return "2019.1+";
        if (version >= 128) return "2018+";
        if (version >= 120) return "2017+";
        if (version >= 113) return "2016+";
        if (version >= 88) return "2015+";
        if (version >= 72) return "2014+";
        if (version >= 65) return "2013+";
        return "Pre-2013";
    }

}