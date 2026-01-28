#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>

struct IXAudio2;
struct IXAudio2MasteringVoice;
struct IXAudio2SourceVoice;

namespace Audio {

struct AudioFormat {
    uint16_t formatTag = 0;
    uint16_t channels = 0;
    uint32_t sampleRate = 0;
    uint32_t avgBytesPerSec = 0;
    uint16_t blockAlign = 0;
    uint16_t bitsPerSample = 0;
    uint32_t dataSize = 0;
    uint32_t totalSamples = 0;
};

enum class PlaybackState { Stopped, Playing, Paused };

using PlaybackCallback = std::function<void(PlaybackState state)>;

class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();

    bool Initialize();
    void Shutdown();
    bool IsInitialized() const { return m_initialized; }

    bool LoadWEM(const std::string& filepath);
    bool LoadWEM(const uint8_t* data, size_t size);
    bool LoadWEM(const std::vector<uint8_t>& data);

    bool Play();
    bool Pause();
    bool Stop();
    bool Seek(float seconds);
    bool IsPlaying() const;
    bool IsPaused() const;
    PlaybackState GetState() const;

    void SetVolume(float volume);
    float GetVolume() const;

    float GetPosition() const;
    float GetDuration() const;

    void SetCallback(PlaybackCallback callback);

    const AudioFormat& GetFormat() const { return m_format; }
    const std::string& GetLastError() const { return m_lastError; }

    static bool IsWEMFile(const uint8_t* data, size_t size);
    static bool IsWEMFile(const std::string& filepath);

private:
    bool InitXAudio2();
    void CleanupXAudio2();
    bool CreateSourceVoice();
    void DestroySourceVoice();
    bool DecodeWEM(const uint8_t* data, size_t size);

    IXAudio2* m_xaudio2 = nullptr;
    IXAudio2MasteringVoice* m_masterVoice = nullptr;
    IXAudio2SourceVoice* m_sourceVoice = nullptr;

    bool m_initialized = false;
    std::atomic<PlaybackState> m_state{PlaybackState::Stopped};
    float m_volume = 1.0f;
    uint32_t m_seekOffset = 0;

    AudioFormat m_format;
    std::vector<uint8_t> m_audioData;
    std::vector<uint8_t> m_rawData;

    PlaybackCallback m_callback;
    std::string m_lastError;

    class VoiceCallback;
    std::unique_ptr<VoiceCallback> m_voiceCallback;
};

class AudioManager {
public:
    static AudioManager& Get();

    bool Initialize();
    void Shutdown();

    bool PlayWEM(const std::string& filepath);
    bool PlayWEM(const std::vector<uint8_t>& data);

    AudioPlayer* GetPlayer() { return m_player.get(); }
    void StopAll();

private:
    AudioManager() = default;
    ~AudioManager() = default;
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    std::unique_ptr<AudioPlayer> m_player;
    bool m_initialized = false;
};

}