#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace Audio
{
    enum class HIRCObjectType : uint8_t
    {
        State = 1,
        Sound = 2,
        Action = 3,
        Event = 4,
        RandomSequenceContainer = 5,
        SwitchContainer = 6,
        ActorMixer = 7,
        AudioBus = 8,
        BlendContainer = 9,
        MusicSegment = 10,
        MusicTrack = 11,
        MusicSwitchContainer = 12,
        MusicPlaylistContainer = 13,
        Attenuation = 14,
        DialogueEvent = 15,
        MotionBus = 16,
        MotionFX = 17,
        Effect = 18,
        Unknown = 19,
        AuxBus = 20
    };

    struct WEMDescriptor
    {
        uint32_t id = 0;
        uint32_t offset = 0;
        uint32_t size = 0;
    };

    struct HIRCObject
    {
        HIRCObjectType type = HIRCObjectType::Unknown;
        uint32_t size = 0;
        uint32_t id = 0;
        std::vector<uint8_t> data;
    };

    struct SoundObject : HIRCObject
    {
        uint32_t pluginId = 0;
        uint8_t streamType = 0;
        uint32_t sourceId = 0;
        uint32_t fileId = 0;
        uint32_t fileOffset = 0;
        uint32_t fileSize = 0;
    };

    struct EventObject : HIRCObject
    {
        std::vector<uint32_t> actionIds;
    };

    struct ActionObject : HIRCObject
    {
        uint8_t scope = 0;
        uint8_t actionType = 0;
        uint32_t targetId = 0;
    };

    struct MusicTrackObject : HIRCObject
    {
        std::vector<uint32_t> sourceIds;
        uint32_t numSubTracks = 0;
    };

    struct BankHeader
    {
        uint32_t version = 0;
        uint32_t bankId = 0;
        uint32_t languageId = 0;
        uint32_t feedback = 0;
        uint32_t projectId = 0;
    };

    struct StringEntry
    {
        uint32_t id = 0;
        std::string name;
    };

    class SoundBank
    {
    public:
        SoundBank() = default;
        ~SoundBank() = default;

        bool Load(const std::string& filepath);
        bool LoadFromMemory(const uint8_t* data, size_t size);

        const BankHeader& GetHeader() const { return m_header; }
        const std::string& GetBankName() const { return m_bankName; }
        uint32_t GetVersion() const { return m_header.version; }

        size_t GetEmbeddedWEMCount() const { return m_wemDescriptors.size(); }
        const WEMDescriptor* GetWEMDescriptor(uint32_t id) const;
        const std::vector<WEMDescriptor>& GetWEMDescriptors() const { return m_wemDescriptors; }
        bool ExtractWEM(uint32_t id, std::vector<uint8_t>& outData) const;
        bool ExtractWEMByIndex(size_t index, std::vector<uint8_t>& outData) const;

        size_t GetHIRCObjectCount() const { return m_hircObjects.size(); }
        const std::vector<std::unique_ptr<HIRCObject>>& GetHIRCObjects() const { return m_hircObjects; }
        const HIRCObject* GetHIRCObject(uint32_t id) const;
        std::vector<const HIRCObject*> GetObjectsByType(HIRCObjectType type) const;

        std::vector<const EventObject*> GetEvents() const;
        std::vector<const SoundObject*> GetSounds() const;

        const std::string* GetStringById(uint32_t id) const;
        const std::unordered_map<uint32_t, std::string>& GetStringMap() const { return m_stringIds; }

        const std::string& GetLastError() const { return m_lastError; }

    private:
        bool ParseChunks(const uint8_t* data, size_t size);
        bool ParseBKHD(const uint8_t* data, size_t size);
        bool ParseDIDX(const uint8_t* data, size_t size);
        bool ParseDATA(const uint8_t* data, size_t size);
        bool ParseHIRC(const uint8_t* data, size_t size);
        bool ParseSTID(const uint8_t* data, size_t size);
        bool ParseSTMG(const uint8_t* data, size_t size);

        std::unique_ptr<HIRCObject> ParseHIRCObject(const uint8_t* data, size_t maxSize, size_t& bytesRead);
        void ParseSoundObject(SoundObject* obj, const uint8_t* data, size_t size);
        void ParseEventObject(EventObject* obj, const uint8_t* data, size_t size);
        void ParseActionObject(ActionObject* obj, const uint8_t* data, size_t size);
        void ParseMusicTrackObject(MusicTrackObject* obj, const uint8_t* data, size_t size);

        BankHeader m_header;
        std::string m_bankName;
        std::string m_filepath;
        std::string m_lastError;

        std::vector<WEMDescriptor> m_wemDescriptors;
        std::vector<uint8_t> m_dataSection;
        size_t m_dataOffset = 0;

        std::vector<std::unique_ptr<HIRCObject>> m_hircObjects;
        std::unordered_map<uint32_t, HIRCObject*> m_hircById;
        std::unordered_map<uint32_t, std::string> m_stringIds;

        std::vector<uint8_t> m_fileData;
    };

    const char* GetHIRCTypeName(HIRCObjectType type);
    std::string GetVersionString(uint32_t version);

}