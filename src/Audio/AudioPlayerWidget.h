#pragma once
#include <string>

namespace Audio {

    class AudioPlayerWidget {
    public:
        static AudioPlayerWidget& Get();

        void Render();

        void PlayFile(const std::string& filepath);
        void PlayFile(const uint8_t* data, size_t size, const std::string& name = "Audio");

        void Show() { m_visible = true; }
        void Hide() { m_visible = false; }
        void Toggle() { m_visible = !m_visible; }
        bool IsVisible() const { return m_visible; }

    private:
        AudioPlayerWidget() = default;

        bool m_visible = false;
        std::string m_currentFile;
    };

}