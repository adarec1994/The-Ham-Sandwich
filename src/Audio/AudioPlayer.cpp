#include "AudioPlayer.h"
#include "wwise/WwiseVorbisDecoder.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <xaudio2.h>
#include <fstream>
#include <cstring>
#include <algorithm>

#pragma comment(lib, "xaudio2.lib")

#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_IMPLEMENTATION
#include "stb_vorbis.c"

namespace Audio {

static bool g_wwiseInitialized = false;

class AudioPlayer::VoiceCallback : public IXAudio2VoiceCallback {
public:
    VoiceCallback(AudioPlayer* player) : m_player(player) {}
    void STDMETHODCALLTYPE OnStreamEnd() override {
        m_player->m_state = PlaybackState::Stopped;
        if (m_player->m_callback) m_player->m_callback(PlaybackState::Stopped);
    }
    void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}
    void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32) override {}
    void STDMETHODCALLTYPE OnBufferEnd(void*) override {}
    void STDMETHODCALLTYPE OnBufferStart(void*) override {}
    void STDMETHODCALLTYPE OnLoopEnd(void*) override {}
    void STDMETHODCALLTYPE OnVoiceError(void*, HRESULT) override {}
private:
    AudioPlayer* m_player;
};

AudioPlayer::AudioPlayer() : m_voiceCallback(std::make_unique<VoiceCallback>(this)) {}
AudioPlayer::~AudioPlayer() { Shutdown(); }

bool AudioPlayer::Initialize() {
    if (m_initialized) return true;
    if (!InitXAudio2()) return false;

    if (!g_wwiseInitialized) {
        if (WwiseInitEmbedded()) {
            g_wwiseInitialized = true;
        }
    }

    m_initialized = true;
    return true;
}

void AudioPlayer::Shutdown() {
    Stop();
    DestroySourceVoice();
    CleanupXAudio2();
    m_initialized = false;
}

bool AudioPlayer::InitXAudio2() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) { m_lastError = "Failed to initialize COM"; return false; }
    hr = XAudio2Create(&m_xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr)) { m_lastError = "Failed to create XAudio2"; return false; }
    hr = m_xaudio2->CreateMasteringVoice(&m_masterVoice);
    if (FAILED(hr)) { m_lastError = "Failed to create mastering voice"; m_xaudio2->Release(); m_xaudio2 = nullptr; return false; }
    return true;
}

void AudioPlayer::CleanupXAudio2() {
    if (m_masterVoice) { m_masterVoice->DestroyVoice(); m_masterVoice = nullptr; }
    if (m_xaudio2) { m_xaudio2->Release(); m_xaudio2 = nullptr; }
}

bool AudioPlayer::CreateSourceVoice() {
    DestroySourceVoice();
    if (!m_xaudio2 || m_format.channels == 0) return false;

    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = m_format.channels;
    wfx.nSamplesPerSec = m_format.sampleRate;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    HRESULT hr = m_xaudio2->CreateSourceVoice(&m_sourceVoice, &wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, m_voiceCallback.get());
    if (FAILED(hr)) { m_lastError = "Failed to create source voice"; return false; }
    m_sourceVoice->SetVolume(m_volume);
    return true;
}

void AudioPlayer::DestroySourceVoice() {
    if (m_sourceVoice) { m_sourceVoice->Stop(); m_sourceVoice->FlushSourceBuffers(); m_sourceVoice->DestroyVoice(); m_sourceVoice = nullptr; }
}

bool AudioPlayer::LoadWEM(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) { m_lastError = "Failed to open: " + filepath; return false; }
    size_t sz = static_cast<size_t>(file.tellg());
    file.seekg(0);
    m_rawData.resize(sz);
    file.read(reinterpret_cast<char*>(m_rawData.data()), sz);
    return LoadWEM(m_rawData.data(), m_rawData.size());
}

bool AudioPlayer::LoadWEM(const std::vector<uint8_t>& data) { return LoadWEM(data.data(), data.size()); }

bool AudioPlayer::LoadWEM(const uint8_t* data, size_t size) {
    if (!m_initialized && !Initialize()) return false;
    Stop();
    m_audioData.clear();
    m_seekOffset = 0;
    m_rawData.assign(data, data + size);
    return DecodeWEM(data, size);
}

bool AudioPlayer::DecodeWEM(const uint8_t* data, size_t size) {
    if (size < 12) { m_lastError = "File too small"; return false; }

    bool isRIFF = memcmp(data, "RIFF", 4) == 0;
    bool isRIFX = memcmp(data, "RIFX", 4) == 0;

    if (!isRIFF && !isRIFX) { m_lastError = "Not RIFF/RIFX"; return false; }
    if (memcmp(data + 8, "WAVE", 4) != 0) { m_lastError = "Not WAVE"; return false; }

    bool bigEndian = isRIFX;
    auto RU16 = [bigEndian](const uint8_t* d) -> uint16_t {
        return bigEndian ? ((d[0] << 8) | d[1]) : (d[0] | (d[1] << 8));
    };
    auto RU32 = [bigEndian](const uint8_t* d) -> uint32_t {
        return bigEndian ? ((d[0] << 24) | (d[1] << 16) | (d[2] << 8) | d[3]) : (d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24));
    };

    size_t offset = 12;
    uint16_t formatTag = 0, channels = 0, blockAlign = 0, bitsPerSample = 0;
    uint32_t sampleRate = 0;
    const uint8_t* audioData = nullptr;
    size_t audioSize = 0;

    while (offset + 8 <= size) {
        char chunkId[5] = {0};
        memcpy(chunkId, data + offset, 4);
        uint32_t chunkSize = RU32(data + offset + 4);

        if (offset + 8 + chunkSize > size) break;

        if (memcmp(chunkId, "fmt ", 4) == 0) {
            const uint8_t* fmt = data + offset + 8;
            formatTag = RU16(fmt);
            channels = RU16(fmt + 2);
            sampleRate = RU32(fmt + 4);
            blockAlign = RU16(fmt + 12);
            bitsPerSample = RU16(fmt + 14);
        } else if (memcmp(chunkId, "data", 4) == 0) {
            audioData = data + offset + 8;
            audioSize = chunkSize;
        }

        offset += 8 + chunkSize;
        if (offset % 2) offset++;
    }

    if (!audioData || audioSize == 0) { m_lastError = "No audio data"; return false; }

    m_format.formatTag = formatTag;
    m_format.channels = channels;
    m_format.sampleRate = sampleRate;
    m_format.blockAlign = blockAlign;
    m_format.bitsPerSample = bitsPerSample;
    m_format.dataSize = (uint32_t)audioSize;

    if (formatTag == 0x0001) {
        if (bitsPerSample == 16) {
            m_audioData.assign(audioData, audioData + audioSize);
        } else {
            m_lastError = "Unsupported PCM bits: " + std::to_string(bitsPerSample);
            return false;
        }
    } else if (formatTag == 0x0002) {
        static const int AdaptTable[] = { 230, 230, 230, 230, 307, 409, 512, 614, 768, 614, 512, 409, 307, 230, 230, 230 };
        static const int Coef1[] = { 256, 512, 0, 192, 240, 460, 392 };
        static const int Coef2[] = { 0, -256, 0, 64, 0, -208, -232 };

        if (blockAlign == 0 || channels == 0) { m_lastError = "Invalid ADPCM params"; return false; }

        size_t numBlocks = audioSize / blockAlign;
        size_t samplesPerBlock = (blockAlign - 7 * channels) * 2 / channels + 2;

        std::vector<int16_t> out;
        out.reserve(numBlocks * samplesPerBlock * channels);

        for (size_t blk = 0; blk < numBlocks; blk++) {
            const uint8_t* bd = audioData + blk * blockAlign;
            size_t pos = 0;

            std::vector<int> c1(channels), c2(channels), delta(channels), s1(channels), s2(channels);

            for (int ch = 0; ch < channels; ch++) {
                int pred = bd[pos++];
                if (pred > 6) pred = 6;
                c1[ch] = Coef1[pred];
                c2[ch] = Coef2[pred];
            }
            for (int ch = 0; ch < channels; ch++) { delta[ch] = (int16_t)(bd[pos] | (bd[pos+1] << 8)); pos += 2; }
            for (int ch = 0; ch < channels; ch++) { s1[ch] = (int16_t)(bd[pos] | (bd[pos+1] << 8)); pos += 2; }
            for (int ch = 0; ch < channels; ch++) { s2[ch] = (int16_t)(bd[pos] | (bd[pos+1] << 8)); pos += 2; }

            if (channels == 1) {
                out.push_back((int16_t)s2[0]);
                out.push_back((int16_t)s1[0]);
            } else {
                out.push_back((int16_t)s2[0]);
                out.push_back((int16_t)s2[1]);
                out.push_back((int16_t)s1[0]);
                out.push_back((int16_t)s1[1]);
            }

            while (pos < blockAlign) {
                for (int ch = 0; ch < channels && pos < blockAlign; ch++) {
                    uint8_t byte = bd[pos++];
                    for (int n = 0; n < 2; n++) {
                        int nib = (n == 0) ? ((byte >> 4) & 0xF) : (byte & 0xF);
                        int snib = (nib >= 8) ? (nib - 16) : nib;
                        int pred = ((s1[ch] * c1[ch]) + (s2[ch] * c2[ch])) / 256 + snib * delta[ch];
                        if (pred > 32767) pred = 32767;
                        if (pred < -32768) pred = -32768;
                        s2[ch] = s1[ch];
                        s1[ch] = pred;
                        delta[ch] = (AdaptTable[nib] * delta[ch]) / 256;
                        if (delta[ch] < 16) delta[ch] = 16;
                        out.push_back((int16_t)pred);
                    }
                }
            }
        }

        m_audioData.resize(out.size() * 2);
        memcpy(m_audioData.data(), out.data(), m_audioData.size());
    } else if (formatTag == 0xFFFF || formatTag == 0xFFFE) {
        if (!g_wwiseInitialized) {
            m_lastError = "Wwise codebooks not loaded";
            return false;
        }

        WwiseVorbisDecoder decoder;
        if (!decoder.ParseWEM(data, size)) {
            m_lastError = "WEM parse failed: " + decoder.GetLastError();
            return false;
        }

        std::vector<uint8_t> oggData;
        if (!decoder.ConvertToOgg(oggData)) {
            m_lastError = "OGG conversion failed: " + decoder.GetLastError();
            return false;
        }

        int stbError = 0;
        stb_vorbis* vorbis = stb_vorbis_open_memory(oggData.data(), (int)oggData.size(), &stbError, nullptr);

        if (!vorbis) {
            const char* errNames[] = {"none","need_more_data","invalid_api_mixing","outofmem","feature_not_supported",
                "too_many_channels","file_open_failure","seek_without_length","unexpected_eof","seek_invalid",
                "invalid_setup","invalid_stream","missing_capture_pattern","invalid_stream_structure_version",
                "continued_packet_flag_invalid","incorrect_stream_serial","invalid_first_page","bad_packet_type",
                "cant_find_last_page","seek_failed","ogg_skeleton_not_supported"};
            const char* errName = (stbError >= 0 && stbError <= 20) ? errNames[stbError] : "unknown";
            m_lastError = "Vorbis decode failed: " + std::string(errName);
            return false;
        }

        stb_vorbis_info info = stb_vorbis_get_info(vorbis);
        int decChannels = info.channels;
        int decSampleRate = info.sample_rate;

        std::vector<short> pcmData;
        pcmData.reserve(1024 * 1024);

        short tempBuf[4096];
        int samplesRead;
        while ((samplesRead = stb_vorbis_get_samples_short_interleaved(vorbis, decChannels, tempBuf, 4096)) > 0) {
            pcmData.insert(pcmData.end(), tempBuf, tempBuf + samplesRead * decChannels);
        }
        stb_vorbis_close(vorbis);

        int decoded = (int)(pcmData.size() / decChannels);
        if (decoded <= 0) {
            m_lastError = "Vorbis decode failed: no samples";
            return false;
        }

        m_format.formatTag = 0x0001;
        m_format.channels = decChannels;
        m_format.sampleRate = decSampleRate;
        m_format.bitsPerSample = 16;
        m_format.blockAlign = decChannels * 2;
        m_format.totalSamples = decoded;

        m_audioData.resize(decoded * decChannels * 2);
        memcpy(m_audioData.data(), pcmData.data(), m_audioData.size());
    } else {
        m_lastError = "Unsupported format: " + std::to_string(formatTag);
        return false;
    }

    return CreateSourceVoice();
}

bool AudioPlayer::Play() {
    if (!m_sourceVoice || m_audioData.empty()) { m_lastError = "No audio loaded"; return false; }
    if (m_state == PlaybackState::Playing) return true;
    if (m_state == PlaybackState::Paused) { m_sourceVoice->Start(); m_state = PlaybackState::Playing; return true; }

    XAUDIO2_BUFFER buf = {};
    buf.AudioBytes = (UINT32)m_audioData.size();
    buf.pAudioData = m_audioData.data();
    buf.Flags = XAUDIO2_END_OF_STREAM;

    m_seekOffset = 0;
    if (FAILED(m_sourceVoice->SubmitSourceBuffer(&buf))) { m_lastError = "Failed to submit buffer"; return false; }
    if (FAILED(m_sourceVoice->Start())) { m_lastError = "Failed to start"; return false; }

    m_state = PlaybackState::Playing;
    if (m_callback) m_callback(PlaybackState::Playing);
    return true;
}

bool AudioPlayer::Pause() {
    if (!m_sourceVoice || m_state != PlaybackState::Playing) return false;
    m_sourceVoice->Stop();
    m_state = PlaybackState::Paused;
    if (m_callback) m_callback(PlaybackState::Paused);
    return true;
}

bool AudioPlayer::Stop() {
    if (!m_sourceVoice) return false;
    m_sourceVoice->Stop();
    m_sourceVoice->FlushSourceBuffers();
    m_seekOffset = 0;
    m_state = PlaybackState::Stopped;
    if (m_callback) m_callback(PlaybackState::Stopped);
    return true;
}

bool AudioPlayer::Seek(float seconds) {
    if (!m_sourceVoice || m_audioData.empty() || m_format.sampleRate == 0) return false;

    bool wasPlaying = (m_state == PlaybackState::Playing);

    m_sourceVoice->Stop();
    m_sourceVoice->FlushSourceBuffers();

    uint32_t sampleOffset = (uint32_t)(seconds * m_format.sampleRate);
    uint32_t totalSamples = (uint32_t)(m_audioData.size() / (m_format.channels * 2));
    if (sampleOffset >= totalSamples) sampleOffset = totalSamples > 0 ? totalSamples - 1 : 0;

    uint32_t byteOffset = sampleOffset * m_format.channels * 2;
    m_seekOffset = sampleOffset;

    XAUDIO2_BUFFER buf = {};
    buf.AudioBytes = (UINT32)(m_audioData.size() - byteOffset);
    buf.pAudioData = m_audioData.data() + byteOffset;
    buf.Flags = XAUDIO2_END_OF_STREAM;

    if (FAILED(m_sourceVoice->SubmitSourceBuffer(&buf))) return false;

    if (wasPlaying) {
        m_sourceVoice->Start();
        m_state = PlaybackState::Playing;
    } else {
        m_state = PlaybackState::Paused;
    }

    return true;
}

bool AudioPlayer::IsPlaying() const { return m_state == PlaybackState::Playing; }
bool AudioPlayer::IsPaused() const { return m_state == PlaybackState::Paused; }
PlaybackState AudioPlayer::GetState() const { return m_state; }

void AudioPlayer::SetVolume(float v) { m_volume = std::max(0.f, std::min(1.f, v)); if (m_sourceVoice) m_sourceVoice->SetVolume(m_volume); }
float AudioPlayer::GetVolume() const { return m_volume; }

float AudioPlayer::GetPosition() const {
    if (!m_sourceVoice || m_format.sampleRate == 0) return 0.f;
    XAUDIO2_VOICE_STATE st; m_sourceVoice->GetState(&st);
    return (float)(st.SamplesPlayed + m_seekOffset) / m_format.sampleRate;
}

float AudioPlayer::GetDuration() const {
    if (m_format.sampleRate == 0 || m_format.channels == 0) return 0.f;
    return (float)(m_audioData.size() / (2 * m_format.channels)) / m_format.sampleRate;
}

void AudioPlayer::SetCallback(PlaybackCallback cb) { m_callback = cb; }

bool AudioPlayer::IsWEMFile(const uint8_t* data, size_t size) {
    return size >= 12 && (memcmp(data, "RIFF", 4) == 0 || memcmp(data, "RIFX", 4) == 0) && memcmp(data + 8, "WAVE", 4) == 0;
}

bool AudioPlayer::IsWEMFile(const std::string& filepath) {
    std::ifstream f(filepath, std::ios::binary);
    uint8_t h[12]; f.read((char*)h, 12);
    return f.gcount() == 12 && IsWEMFile(h, 12);
}

AudioManager& AudioManager::Get() { static AudioManager inst; return inst; }

bool AudioManager::Initialize() {
    if (m_initialized) return true;
    m_player = std::make_unique<AudioPlayer>();
    if (!m_player->Initialize()) return false;
    m_initialized = true;
    return true;
}

void AudioManager::Shutdown() {
    if (m_player) { m_player->Shutdown(); m_player.reset(); }
    WwiseShutdown();
    g_wwiseInitialized = false;
    m_initialized = false;
}

bool AudioManager::PlayWEM(const std::string& filepath) {
    if (!m_initialized && !Initialize()) return false;
    if (!m_player->LoadWEM(filepath)) return false;
    return m_player->Play();
}

bool AudioManager::PlayWEM(const std::vector<uint8_t>& data) {
    if (!m_initialized && !Initialize()) return false;
    if (!m_player->LoadWEM(data)) return false;
    return m_player->Play();
}

void AudioManager::StopAll() { if (m_player) m_player->Stop(); }

}