#pragma once
#include <string>
#include <unordered_map>
#include <imgui.h>

namespace Keybinds
{
    enum class Action
    {
        MoveForward,
        MoveBackward,
        MoveLeft,
        MoveRight,
        MoveUp,
        MoveDown,
        DeleteProp,
        HideProp,
        ToggleContentBrowser,
        COUNT
    };

    struct Keybind
    {
        ImGuiKey key = ImGuiKey_None;
        bool ctrl = false;
        bool shift = false;
        bool alt = false;
        std::string displayName;
    };

    void Init();
    void Load();
    void Save();

    ImGuiKey GetKey(Action action);
    void SetKey(Action action, ImGuiKey key, bool ctrl = false, bool shift = false, bool alt = false);
    const char* GetActionName(Action action);
    const char* GetKeyName(ImGuiKey key);
    std::string GetKeybindString(Action action);

    bool IsPressed(Action action);
    bool IsDown(Action action);

    std::unordered_map<Action, Keybind>& GetAllBindings();
    std::unordered_map<Action, Keybind> GetBindingsCopy();
    void ApplyBindings(const std::unordered_map<Action, Keybind>& bindings);
}