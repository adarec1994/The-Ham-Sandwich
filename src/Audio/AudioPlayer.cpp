#include "AudioPlayer.h"
#include "wwise/WwiseVorbisDecoder.h"
#include "wwise/WwiseCodebooks.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <xaudio2.h>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "xaudio2.lib")

// Try to include stb_vorbis for OGG decoding
#if __has_include("wwise/stb_vorbis.c")
#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_IMPLEMENTATION
#include "wwise/stb_vorbis.c"
#define HAS_STB_VORBIS 1
#else
#define HAS_STB_VORBIS 0
#endif

static std::ofstream g_log;

static void Log(const std::string& msg)
{
    if (!g_log.is_open())
        g_log.open("audio_debug.txt", std::ios::out | std::ios::trunc);
    if (g_log.is_open())
    {
        g_log << msg << "\n";
        g_log.flush();
    }
}

static std::string Hex(const uint8_t* data, size_t len)
{
    std::ostringstream ss;
    for (size_t i = 0; i < len; i++)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i] << " ";
    return ss.str();
}

namespace Audio
{
    static inline uint16_t ReadU16LE(const uint8_t* data)
    {
        return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
    }

    static inline uint32_t ReadU32LE(const uint8_t* data)
    {
        return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) |
               (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
    }

    class AudioPlayer::VoiceCallback : public IXAudio2VoiceCallback
    {
    public:
        VoiceCallback(AudioPlayer* player) : m_player(player) {}
        void STDMETHODCALLTYPE OnStreamEnd() override
        {
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

    bool AudioPlayer::Initialize()
    {
        if (m_initialized) return true;
        if (!InitXAudio2()) return false;

        // Try to initialize Wwise codebooks
        // Look in several locations
        std::vector<std::string> codebookPaths = {
            "packed_codebooks_aoTuV_603.bin",
            "data/packed_codebooks_aoTuV_603.bin",
            "audio/packed_codebooks_aoTuV_603.bin"
        };

        // Also check next to executable
        char exePath[MAX_PATH];
        if (GetModuleFileNameA(nullptr, exePath, MAX_PATH))
        {
            std::string dir(exePath);
            size_t lastSlash = dir.find_last_of("\\/");
            if (lastSlash != std::string::npos)
            {
                dir = dir.substr(0, lastSlash + 1);
                codebookPaths.push_back(dir + "packed_codebooks_aoTuV_603.bin");
                codebookPaths.push_back(dir + "data/packed_codebooks_aoTuV_603.bin");
            }
        }

        for (const auto& path : codebookPaths)
        {
            if (InitializeCodebooks(path))
            {
                Log("Loaded codebooks from: " + path);
                break;
            }
        }

        if (!IsCodebooksInitialized())
        {
            Log("WARNING: Could not load packed_codebooks_aoTuV_603.bin");
            Log("Wwise Vorbis audio will fall back to vgmstream-cli");
        }

        m_initialized = true;
        return true;
    }

    void AudioPlayer::Shutdown()
    {
        Stop();
        DestroySourceVoice();
        CleanupXAudio2();
        m_initialized = false;
    }

    bool AudioPlayer::InitXAudio2()
    {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) { m_lastError = "Failed to initialize COM"; return false; }
        hr = XAudio2Create(&m_xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
        if (FAILED(hr)) { m_lastError = "Failed to create XAudio2"; return false; }
        hr = m_xaudio2->CreateMasteringVoice(&m_masterVoice);
        if (FAILED(hr)) { m_lastError = "Failed to create mastering voice"; m_xaudio2->Release(); m_xaudio2 = nullptr; return false; }
        return true;
    }

    void AudioPlayer::CleanupXAudio2()
    {
        if (m_masterVoice) { m_masterVoice->DestroyVoice(); m_masterVoice = nullptr; }
        if (m_xaudio2) { m_xaudio2->Release(); m_xaudio2 = nullptr; }
    }

    bool AudioPlayer::CreateSourceVoice()
    {
        DestroySourceVoice();
        if (!m_xaudio2 || m_format.channels == 0) return false;

        WAVEFORMATEX wfx = {};
        wfx.wFormatTag = WAVE_FORMAT_PCM;
        wfx.nChannels = m_format.channels;
        wfx.nSamplesPerSec = m_format.sampleRate;
        wfx.wBitsPerSample = 16;
        wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) / 8;
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

        Log("CreateSourceVoice: " + std::to_string(wfx.nChannels) + "ch, " + std::to_string(wfx.nSamplesPerSec) + "Hz");

        HRESULT hr = m_xaudio2->CreateSourceVoice(&m_sourceVoice, &wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, m_voiceCallback.get());
        if (FAILED(hr)) { m_lastError = "Failed to create source voice"; return false; }
        m_sourceVoice->SetVolume(m_volume);
        return true;
    }

    void AudioPlayer::DestroySourceVoice()
    {
        if (m_sourceVoice) { m_sourceVoice->Stop(); m_sourceVoice->FlushSourceBuffers(); m_sourceVoice->DestroyVoice(); m_sourceVoice = nullptr; }
    }

    bool AudioPlayer::LoadWEM(const std::string& filepath)
    {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) { m_lastError = "Failed to open: " + filepath; return false; }
        size_t sz = static_cast<size_t>(file.tellg());
        file.seekg(0);
        m_rawData.resize(sz);
        file.read(reinterpret_cast<char*>(m_rawData.data()), sz);
        return LoadWEM(m_rawData.data(), m_rawData.size());
    }

    bool AudioPlayer::LoadWEM(const std::vector<uint8_t>& data) { return LoadWEM(data.data(), data.size()); }

    bool AudioPlayer::LoadWEM(const uint8_t* data, size_t size)
    {
        if (!m_initialized && !Initialize()) return false;
        Stop();
        m_audioData.clear();
        m_rawData.assign(data, data + size);
        return DecodeWEM(data, size);
    }

    bool AudioPlayer::DecodeWEM(const uint8_t* data, size_t size)
    {
        Log("========== WEM DEBUG ==========");
        Log("File size: " + std::to_string(size));

        if (size < 12) { m_lastError = "File too small"; Log("ERROR: File too small"); return false; }

        Log("First 64 bytes: " + Hex(data, std::min(size, (size_t)64)));

        bool isRIFF = memcmp(data, "RIFF", 4) == 0;
        bool isRIFX = memcmp(data, "RIFX", 4) == 0;
        Log("Is RIFF: " + std::string(isRIFF ? "YES" : "NO"));
        Log("Is RIFX: " + std::string(isRIFX ? "YES" : "NO"));

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

        while (offset + 8 <= size)
        {
            char chunkId[5] = {0};
            memcpy(chunkId, data + offset, 4);
            uint32_t chunkSize = RU32(data + offset + 4);
            Log("Chunk '" + std::string(chunkId) + "' at " + std::to_string(offset) + ", size " + std::to_string(chunkSize));

            if (offset + 8 + chunkSize > size) break;

            if (memcmp(chunkId, "fmt ", 4) == 0)
            {
                const uint8_t* fmt = data + offset + 8;
                formatTag = RU16(fmt);
                channels = RU16(fmt + 2);
                sampleRate = RU32(fmt + 4);
                blockAlign = RU16(fmt + 12);
                bitsPerSample = RU16(fmt + 14);

                {
                    std::ostringstream oss;
                    oss << "fmt: tag=0x" << std::hex << formatTag << std::dec
                        << " ch=" << channels
                        << " rate=" << sampleRate
                        << " block=" << blockAlign
                        << " bits=" << bitsPerSample;
                    Log(oss.str());
                }

                std::string fmtName;
                switch(formatTag) {
                    case 0x0001: fmtName = "PCM"; break;
                    case 0x0002: fmtName = "MS ADPCM"; break;
                    case 0x0003: fmtName = "IEEE Float"; break;
                    case 0x0011: fmtName = "IMA ADPCM"; break;
                    case 0x0069: fmtName = "Wwise IMA ADPCM"; break;
                    case 0xFFFE: fmtName = "EXTENSIBLE"; break;
                    case 0xFFFF: fmtName = "Wwise Vorbis"; break;
                    default: fmtName = "Unknown"; break;
                }
                Log("Format name: " + fmtName);

                if (chunkSize > 16)
                    Log("Extra fmt bytes: " + Hex(fmt + 16, std::min((size_t)(chunkSize - 16), (size_t)32)));
            }
            else if (memcmp(chunkId, "data", 4) == 0)
            {
                audioData = data + offset + 8;
                audioSize = chunkSize;
                Log("data chunk: " + std::to_string(audioSize) + " bytes");
                Log("First 32 bytes of audio: " + Hex(audioData, std::min(audioSize, (size_t)32)));
            }

            offset += 8 + chunkSize;
            if (offset % 2) offset++;
        }

        if (!audioData || audioSize == 0) { m_lastError = "No audio data"; Log("ERROR: No audio data"); return false; }

        m_format.formatTag = formatTag;
        m_format.channels = channels;
        m_format.sampleRate = sampleRate;
        m_format.blockAlign = blockAlign;
        m_format.bitsPerSample = bitsPerSample;
        m_format.dataSize = (uint32_t)audioSize;

        if (formatTag == 0x0001) // PCM
        {
            Log("Decoding PCM...");
            if (bitsPerSample == 16) {
                m_audioData.assign(audioData, audioData + audioSize);
            } else {
                m_lastError = "Unsupported PCM bits: " + std::to_string(bitsPerSample);
                Log("ERROR: " + m_lastError);
                return false;
            }
        }
        else if (formatTag == 0x0002) // MS ADPCM
        {
            Log("========== MS ADPCM DECODE ==========");

            static const int AdaptTable[] = { 230, 230, 230, 230, 307, 409, 512, 614, 768, 614, 512, 409, 307, 230, 230, 230 };
            static const int Coef1[] = { 256, 512, 0, 192, 240, 460, 392 };
            static const int Coef2[] = { 0, -256, 0, 64, 0, -208, -232 };

            if (blockAlign == 0 || channels == 0) { m_lastError = "Invalid ADPCM params"; return false; }

            size_t numBlocks = audioSize / blockAlign;
            size_t samplesPerBlock = (blockAlign - 7 * channels) * 2 / channels + 2;
            Log("Blocks: " + std::to_string(numBlocks) + ", SamplesPerBlock: " + std::to_string(samplesPerBlock));
            Log("First block header: " + Hex(audioData, std::min((size_t)32, audioSize)));

            std::vector<int16_t> out;
            out.reserve(numBlocks * samplesPerBlock * channels);

            for (size_t blk = 0; blk < numBlocks; blk++)
            {
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

                if (blk == 0) {
                    for (int ch = 0; ch < channels; ch++)
                        Log("Ch" + std::to_string(ch) + ": c1=" + std::to_string(c1[ch]) + " c2=" + std::to_string(c2[ch]) +
                            " delta=" + std::to_string(delta[ch]) + " s1=" + std::to_string(s1[ch]) + " s2=" + std::to_string(s2[ch]));
                }

                // Output initial samples
                if (channels == 1) {
                    out.push_back((int16_t)s2[0]);
                    out.push_back((int16_t)s1[0]);
                } else {
                    out.push_back((int16_t)s2[0]);
                    out.push_back((int16_t)s2[1]);
                    out.push_back((int16_t)s1[0]);
                    out.push_back((int16_t)s1[1]);
                }

                while (pos < blockAlign)
                {
                    for (int ch = 0; ch < channels && pos < blockAlign; ch++)
                    {
                        uint8_t byte = bd[pos++];
                        for (int n = 0; n < 2; n++)
                        {
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
            Log("Decoded " + std::to_string(out.size()) + " samples, " + std::to_string(m_audioData.size()) + " bytes");

            // Log first samples
            std::ostringstream ss;
            ss << "First 16 samples: ";
            for (size_t i = 0; i < std::min(out.size(), (size_t)16); i++) ss << out[i] << " ";
            Log(ss.str());
        }
        else if (formatTag == 0xFFFF || formatTag == 0xFFFE)
        {
            Log("Wwise Vorbis detected, attempting decode...");

            // Try native Wwise Vorbis decoder first
            if (IsCodebooksInitialized())
            {
                Log("Using native Wwise Vorbis decoder...");
                WwiseVorbisDecoder decoder;

                if (decoder.ParseWEM(data, size))
                {
                    Log("WEM parsed: " + std::to_string(decoder.GetChannels()) + " ch, " +
                        std::to_string(decoder.GetSampleRate()) + " Hz");

                    // Convert to OGG
                    std::vector<uint8_t> oggData;
                    if (decoder.ConvertToOgg(oggData))
                    {
                        Log("Converted to OGG: " + std::to_string(oggData.size()) + " bytes");

#if HAS_STB_VORBIS
                        // Decode OGG to PCM using stb_vorbis
                        int decChannels = 0, decSampleRate = 0;
                        short* output = nullptr;
                        int samples = stb_vorbis_decode_memory(
                            oggData.data(), (int)oggData.size(),
                            &decChannels, &decSampleRate, &output);

                        if (samples > 0 && output)
                        {
                            Log("Decoded " + std::to_string(samples) + " samples");

                            m_format.formatTag = 0x0001; // PCM
                            m_format.channels = decChannels;
                            m_format.sampleRate = decSampleRate;
                            m_format.bitsPerSample = 16;
                            m_format.blockAlign = decChannels * 2;
                            m_format.totalSamples = samples;

                            m_audioData.resize(samples * decChannels * 2);
                            memcpy(m_audioData.data(), output, m_audioData.size());
                            free(output);

                            return CreateSourceVoice();
                        }
                        else
                        {
                            Log("stb_vorbis decode failed");
                        }
#else
                        // No stb_vorbis - save OGG to temp file and try vgmstream
                        Log("No built-in OGG decoder, saving converted OGG...");

                        char tempPath[MAX_PATH];
                        char tempOgg[MAX_PATH];
                        GetTempPathA(MAX_PATH, tempPath);
                        snprintf(tempOgg, MAX_PATH, "%swem_converted_%u.ogg", tempPath, GetCurrentProcessId());

                        std::ofstream oggFile(tempOgg, std::ios::binary);
                        if (oggFile.is_open())
                        {
                            oggFile.write(reinterpret_cast<const char*>(oggData.data()), oggData.size());
                            oggFile.close();
                            Log("Saved OGG to: " + std::string(tempOgg));

                            // Try to decode the OGG with vgmstream
                            // (It can decode standard OGG too)
                        }
#endif
                    }
                    else
                    {
                        Log("OGG conversion failed: " + decoder.GetLastError());
                    }
                }
                else
                {
                    Log("WEM parse failed: " + decoder.GetLastError());
                }
            }

            // Fall back to vgmstream-cli if available
            if (DecodeWithVgmstream(data, size))
            {
                Log("Decoded with vgmstream successfully");
                return CreateSourceVoice();
            }

            // Fallback: explain the issue
            m_lastError = "Wwise Vorbis decode failed. Place packed_codebooks_aoTuV_603.bin or vgmstream-cli.exe in app directory";
            Log("ERROR: No Wwise Vorbis decoder available");
            Log("Option 1: Place packed_codebooks_aoTuV_603.bin in the application directory");
            Log("Option 2: Download vgmstream from: https://github.com/vgmstream/vgmstream/releases");
            Log("          Place vgmstream-cli.exe in the same directory as the application");
            return false;
        }
        else
        {
            std::ostringstream oss;
            oss << std::hex << formatTag;
            m_lastError = "Unsupported format: 0x" + oss.str();
            Log("ERROR: " + m_lastError);
            return false;
        }

        Log("================================");
        return CreateSourceVoice();
    }

    bool AudioPlayer::DecodeWithVgmstream(const uint8_t* data, size_t size)
    {
        // Write WEM data to temp file
        char tempPath[MAX_PATH];
        char tempWem[MAX_PATH];
        char tempWav[MAX_PATH];

        GetTempPathA(MAX_PATH, tempPath);
        snprintf(tempWem, MAX_PATH, "%swem_temp_%u.wem", tempPath, GetCurrentProcessId());
        snprintf(tempWav, MAX_PATH, "%swem_temp_%u.wav", tempPath, GetCurrentProcessId());

        // Write WEM file
        {
            std::ofstream wemFile(tempWem, std::ios::binary);
            if (!wemFile.is_open())
            {
                Log("Failed to create temp WEM file");
                return false;
            }
            wemFile.write(reinterpret_cast<const char*>(data), size);
        }

        // Try to find vgmstream-cli.exe
        std::string vgmstreamPath;

        // Check current directory
        if (GetFileAttributesA("vgmstream-cli.exe") != INVALID_FILE_ATTRIBUTES)
            vgmstreamPath = "vgmstream-cli.exe";
        // Check application directory
        else
        {
            char exePath[MAX_PATH];
            GetModuleFileNameA(nullptr, exePath, MAX_PATH);
            std::string dir(exePath);
            size_t lastSlash = dir.find_last_of("\\/");
            if (lastSlash != std::string::npos)
                dir = dir.substr(0, lastSlash + 1);
            vgmstreamPath = dir + "vgmstream-cli.exe";

            if (GetFileAttributesA(vgmstreamPath.c_str()) == INVALID_FILE_ATTRIBUTES)
            {
                // Also check for vgmstream123.exe
                vgmstreamPath = dir + "vgmstream123.exe";
                if (GetFileAttributesA(vgmstreamPath.c_str()) == INVALID_FILE_ATTRIBUTES)
                {
                    DeleteFileA(tempWem);
                    Log("vgmstream-cli.exe not found");
                    return false;
                }
            }
        }

        // Build command line: vgmstream-cli -o output.wav input.wem
        std::string cmdLine = "\"" + vgmstreamPath + "\" -o \"" + tempWav + "\" \"" + tempWem + "\"";

        Log("Running: " + cmdLine);

        // Run vgmstream
        STARTUPINFOA si = {};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        PROCESS_INFORMATION pi = {};

        if (!CreateProcessA(nullptr, const_cast<char*>(cmdLine.c_str()),
                           nullptr, nullptr, FALSE,
                           CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
        {
            DeleteFileA(tempWem);
            DWORD err = ::GetLastError();
            Log("Failed to run vgmstream: error " + std::to_string(err));
            return false;
        }

        // Wait for process to complete (max 30 seconds)
        WaitForSingleObject(pi.hProcess, 30000);

        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        DeleteFileA(tempWem);

        if (exitCode != 0)
        {
            DeleteFileA(tempWav);
            Log("vgmstream failed with exit code: " + std::to_string(static_cast<unsigned long>(exitCode)));
            return false;
        }

        // Read the WAV file
        std::ifstream wavFile(tempWav, std::ios::binary | std::ios::ate);
        if (!wavFile.is_open())
        {
            DeleteFileA(tempWav);
            Log("Failed to open converted WAV file");
            return false;
        }

        size_t wavSize = static_cast<size_t>(wavFile.tellg());
        wavFile.seekg(0);

        std::vector<uint8_t> wavData(wavSize);
        wavFile.read(reinterpret_cast<char*>(wavData.data()), wavSize);
        wavFile.close();

        DeleteFileA(tempWav);

        // Parse the WAV file to get PCM data
        if (wavSize < 44)
        {
            Log("WAV file too small");
            return false;
        }

        // Simple WAV parser
        if (memcmp(wavData.data(), "RIFF", 4) != 0 || memcmp(wavData.data() + 8, "WAVE", 4) != 0)
        {
            Log("Invalid WAV file from vgmstream");
            return false;
        }

        // Find fmt and data chunks
        size_t pos = 12;
        while (pos + 8 <= wavSize)
        {
            uint32_t chunkSize = ReadU32LE(wavData.data() + pos + 4);

            if (memcmp(wavData.data() + pos, "fmt ", 4) == 0)
            {
                const uint8_t* fmt = wavData.data() + pos + 8;
                m_format.formatTag = ReadU16LE(fmt);
                m_format.channels = ReadU16LE(fmt + 2);
                m_format.sampleRate = ReadU32LE(fmt + 4);
                m_format.bitsPerSample = ReadU16LE(fmt + 14);

                Log("WAV format: " + std::to_string(m_format.channels) + "ch, " +
                    std::to_string(m_format.sampleRate) + "Hz, " +
                    std::to_string(m_format.bitsPerSample) + "bit");
            }
            else if (memcmp(wavData.data() + pos, "data", 4) == 0)
            {
                m_audioData.assign(wavData.data() + pos + 8, wavData.data() + pos + 8 + chunkSize);
                m_format.dataSize = chunkSize;
                Log("PCM data: " + std::to_string(chunkSize) + " bytes");
                return true;
            }

            pos += 8 + chunkSize;
            if (pos % 2) pos++;
        }

        Log("No data chunk in WAV file");
        return false;
    }

    bool AudioPlayer::Play()
    {
        if (!m_sourceVoice || m_audioData.empty()) { m_lastError = "No audio loaded"; return false; }
        if (m_state == PlaybackState::Playing) return true;
        if (m_state == PlaybackState::Paused) { m_sourceVoice->Start(); m_state = PlaybackState::Playing; return true; }

        XAUDIO2_BUFFER buf = {};
        buf.AudioBytes = (UINT32)m_audioData.size();
        buf.pAudioData = m_audioData.data();
        buf.Flags = XAUDIO2_END_OF_STREAM;

        if (FAILED(m_sourceVoice->SubmitSourceBuffer(&buf))) { m_lastError = "Failed to submit buffer"; return false; }
        if (FAILED(m_sourceVoice->Start())) { m_lastError = "Failed to start"; return false; }

        m_state = PlaybackState::Playing;
        if (m_callback) m_callback(PlaybackState::Playing);
        return true;
    }

    bool AudioPlayer::Pause()
    {
        if (!m_sourceVoice || m_state != PlaybackState::Playing) return false;
        m_sourceVoice->Stop();
        m_state = PlaybackState::Paused;
        if (m_callback) m_callback(PlaybackState::Paused);
        return true;
    }

    bool AudioPlayer::Stop()
    {
        if (!m_sourceVoice) return false;
        m_sourceVoice->Stop();
        m_sourceVoice->FlushSourceBuffers();
        m_state = PlaybackState::Stopped;
        if (m_callback) m_callback(PlaybackState::Stopped);
        return true;
    }

    bool AudioPlayer::IsPlaying() const { return m_state == PlaybackState::Playing; }
    bool AudioPlayer::IsPaused() const { return m_state == PlaybackState::Paused; }
    PlaybackState AudioPlayer::GetState() const { return m_state; }

    void AudioPlayer::SetVolume(float v) { m_volume = std::max(0.f, std::min(1.f, v)); if (m_sourceVoice) m_sourceVoice->SetVolume(m_volume); }
    float AudioPlayer::GetVolume() const { return m_volume; }

    float AudioPlayer::GetPosition() const
    {
        if (!m_sourceVoice || m_format.sampleRate == 0) return 0.f;
        XAUDIO2_VOICE_STATE st; m_sourceVoice->GetState(&st);
        return (float)st.SamplesPlayed / m_format.sampleRate;
    }

    float AudioPlayer::GetDuration() const
    {
        if (m_format.sampleRate == 0 || m_format.channels == 0) return 0.f;
        return (float)(m_audioData.size() / (2 * m_format.channels)) / m_format.sampleRate;
    }

    void AudioPlayer::SetCallback(PlaybackCallback cb) { m_callback = cb; }

    bool AudioPlayer::IsWEMFile(const uint8_t* data, size_t size)
    {
        return size >= 12 && (memcmp(data, "RIFF", 4) == 0 || memcmp(data, "RIFX", 4) == 0) && memcmp(data + 8, "WAVE", 4) == 0;
    }

    bool AudioPlayer::IsWEMFile(const std::string& filepath)
    {
        std::ifstream f(filepath, std::ios::binary);
        uint8_t h[12]; f.read((char*)h, 12);
        return f.gcount() == 12 && IsWEMFile(h, 12);
    }

    AudioManager& AudioManager::Get() { static AudioManager inst; return inst; }

    bool AudioManager::Initialize()
    {
        if (m_initialized) return true;
        m_player = std::make_unique<AudioPlayer>();
        if (!m_player->Initialize()) return false;
        m_initialized = true;
        return true;
    }

    void AudioManager::Shutdown()
    {
        if (m_player) { m_player->Shutdown(); m_player.reset(); }
        m_initialized = false;
    }

    bool AudioManager::PlayWEM(const std::string& filepath)
    {
        if (!m_initialized && !Initialize()) return false;
        if (!m_player->LoadWEM(filepath)) return false;
        return m_player->Play();
    }

    bool AudioManager::PlayWEM(const std::vector<uint8_t>& data)
    {
        if (!m_initialized && !Initialize()) return false;
        if (!m_player->LoadWEM(data)) return false;
        return m_player->Play();
    }

    void AudioManager::StopAll() { if (m_player) m_player->Stop(); }
}