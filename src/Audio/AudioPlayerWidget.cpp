#include "AudioPlayerWidget.h"
#include "AudioPlayer.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>

namespace Audio {

AudioPlayerWidget& AudioPlayerWidget::Get() {
    static AudioPlayerWidget instance;
    return instance;
}

void AudioPlayerWidget::PlayFile(const std::string& filepath) {
    auto& manager = AudioManager::Get();
    if (manager.PlayWEM(filepath)) {
        size_t lastSlash = filepath.find_last_of("/\\");
        m_currentFile = (lastSlash != std::string::npos) ? filepath.substr(lastSlash + 1) : filepath;
        m_visible = true;
    }
}

void AudioPlayerWidget::PlayFile(const uint8_t* data, size_t size, const std::string& name) {
    auto& manager = AudioManager::Get();
    std::vector<uint8_t> vec(data, data + size);
    if (manager.PlayWEM(vec)) {
        m_currentFile = name;
        m_visible = true;
    } else {
        m_currentFile = name;
        m_visible = true;
    }
}

static std::string FormatTime(float seconds) {
    if (seconds < 0) seconds = 0;
    int mins = (int)(seconds / 60);
    int secs = (int)(seconds) % 60;
    char buf[32];
    snprintf(buf, sizeof(buf), "%d:%02d", mins, secs);
    return buf;
}

void AudioPlayerWidget::Render() {
    if (!m_visible) return;

    auto& manager = AudioManager::Get();
    auto* player = manager.GetPlayer();
    if (!player) return;

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::SetNextWindowSize(ImVec2(350, 0), ImGuiCond_FirstUseEver);

    bool wasVisible = m_visible;

    if (ImGui::Begin("Audio Player", &m_visible, flags)) {
        if (!m_currentFile.empty()) {
            ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s", m_currentFile.c_str());
            ImGui::Separator();
        }

        float duration = player->GetDuration();
        float position = player->GetPosition();
        bool isPlaying = player->IsPlaying();
        bool isPaused = player->IsPaused();
        bool hasAudio = duration > 0;

        ImGui::BeginDisabled(!hasAudio);

        float buttonWidth = 80.0f;
        if (isPlaying) {
            if (ImGui::Button("Pause", ImVec2(buttonWidth, 0))) {
                player->Pause();
            }
        } else {
            if (ImGui::Button(isPaused ? "Resume" : "Play", ImVec2(buttonWidth, 0))) {
                player->Play();
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Stop", ImVec2(buttonWidth, 0))) {
            player->Stop();
        }

        ImGui::SameLine();

        ImGui::SetNextItemWidth(80);
        float volume = player->GetVolume();
        if (ImGui::SliderFloat("##vol", &volume, 0.0f, 1.0f, "")) {
            player->SetVolume(volume);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Volume: %.0f%%", volume * 100);
        }
        ImGui::SameLine();
        ImGui::Text("Vol");

        ImGui::Spacing();

        std::string timeStr = FormatTime(position) + " / " + FormatTime(duration);

        ImVec2 progressSize(ImGui::GetContentRegionAvail().x, 20);
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();

        float progress = (duration > 0) ? (position / duration) : 0.0f;
        progress = std::clamp(progress, 0.0f, 1.0f);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImU32 bgColor = IM_COL32(50, 50, 50, 255);
        ImU32 fillColor = IM_COL32(80, 160, 255, 255);
        ImU32 hoverColor = IM_COL32(100, 180, 255, 255);

        ImVec2 barEnd(cursorPos.x + progressSize.x, cursorPos.y + progressSize.y);
        drawList->AddRectFilled(cursorPos, barEnd, bgColor, 4.0f);

        ImVec2 fillEnd(cursorPos.x + progressSize.x * progress, cursorPos.y + progressSize.y);
        drawList->AddRectFilled(cursorPos, fillEnd, fillColor, 4.0f);

        ImGui::InvisibleButton("##progress", progressSize);

        bool hovered = ImGui::IsItemHovered();
        bool clicked = ImGui::IsItemClicked();

        if (hovered) {
            drawList->AddRect(cursorPos, barEnd, hoverColor, 4.0f, 0, 2.0f);

            ImVec2 mousePos = ImGui::GetMousePos();
            float seekRatio = (mousePos.x - cursorPos.x) / progressSize.x;
            seekRatio = std::clamp(seekRatio, 0.0f, 1.0f);
            float seekTime = seekRatio * duration;

            ImGui::SetTooltip("%s", FormatTime(seekTime).c_str());

            if (clicked && hasAudio) {
                player->Seek(seekTime);
            }
        }

        ImVec2 textSize = ImGui::CalcTextSize(timeStr.c_str());
        ImVec2 textPos(
            cursorPos.x + (progressSize.x - textSize.x) * 0.5f,
            cursorPos.y + (progressSize.y - textSize.y) * 0.5f
        );
        drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), timeStr.c_str());

        ImGui::EndDisabled();

        if (!hasAudio) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No audio loaded");
        } else {
            const char* stateStr = isPlaying ? "Playing" : (isPaused ? "Paused" : "Stopped");
            ImVec4 stateColor = isPlaying ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f) :
                                isPaused ? ImVec4(1.0f, 0.8f, 0.3f, 1.0f) :
                                           ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            ImGui::TextColored(stateColor, "%s", stateStr);

            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "| %d Hz, %d ch",
                player->GetFormat().sampleRate, player->GetFormat().channels);
        }

        const std::string& error = player->GetLastError();
        if (!error.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error: %s", error.c_str());
        }
    }
    ImGui::End();

    if (wasVisible && !m_visible) {
        manager.StopAll();
    }
}

}