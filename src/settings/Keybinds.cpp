#include "Keybinds.h"
#include <fstream>
#include <sstream>

namespace Keybinds
{
    static std::unordered_map<Action, Keybind> sBindings;
    static const char* CONFIG_FILE = "keybinds.cfg";

    static const char* sActionNames[] = {
        "Move Forward",
        "Move Backward",
        "Move Left",
        "Move Right",
        "Move Up",
        "Move Down",
        "Delete Prop",
        "Hide Prop",
        "Toggle Content Browser"
    };

    void Init()
    {
        sBindings[Action::MoveForward] = { ImGuiKey_W, false, false, false, "Move Forward" };
        sBindings[Action::MoveBackward] = { ImGuiKey_S, false, false, false, "Move Backward" };
        sBindings[Action::MoveLeft] = { ImGuiKey_A, false, false, false, "Move Left" };
        sBindings[Action::MoveRight] = { ImGuiKey_D, false, false, false, "Move Right" };
        sBindings[Action::MoveUp] = { ImGuiKey_E, false, false, false, "Move Up" };
        sBindings[Action::MoveDown] = { ImGuiKey_Q, false, false, false, "Move Down" };
        sBindings[Action::DeleteProp] = { ImGuiKey_Delete, false, false, false, "Delete Prop" };
        sBindings[Action::HideProp] = { ImGuiKey_H, false, false, false, "Hide Prop" };
        sBindings[Action::ToggleContentBrowser] = { ImGuiKey_Space, true, false, false, "Toggle Content Browser" };

        Load();
    }

    void Load()
    {
        std::ifstream file(CONFIG_FILE);
        if (!file.is_open()) return;

        std::string line;
        while (std::getline(file, line))
        {
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;

            std::string actionStr = line.substr(0, eq);
            std::string valueStr = line.substr(eq + 1);

            int keyVal = 0;
            int ctrl = 0, shift = 0, alt = 0;
            sscanf(valueStr.c_str(), "%d,%d,%d,%d", &keyVal, &ctrl, &shift, &alt);

            for (int i = 0; i < static_cast<int>(Action::COUNT); i++)
            {
                Action action = static_cast<Action>(i);
                if (sBindings[action].displayName == actionStr)
                {
                    sBindings[action].key = static_cast<ImGuiKey>(keyVal);
                    sBindings[action].ctrl = ctrl != 0;
                    sBindings[action].shift = shift != 0;
                    sBindings[action].alt = alt != 0;
                    break;
                }
            }
        }
    }

    void Save()
    {
        std::ofstream file(CONFIG_FILE);
        if (!file.is_open()) return;

        for (const auto& [action, bind] : sBindings)
        {
            file << bind.displayName << "="
                 << static_cast<int>(bind.key) << ","
                 << (bind.ctrl ? 1 : 0) << ","
                 << (bind.shift ? 1 : 0) << ","
                 << (bind.alt ? 1 : 0) << "\n";
        }
    }

    ImGuiKey GetKey(Action action)
    {
        auto it = sBindings.find(action);
        if (it != sBindings.end())
            return it->second.key;
        return ImGuiKey_None;
    }

    void SetKey(Action action, ImGuiKey key, bool ctrl, bool shift, bool alt)
    {
        sBindings[action].key = key;
        sBindings[action].ctrl = ctrl;
        sBindings[action].shift = shift;
        sBindings[action].alt = alt;
    }

    const char* GetActionName(Action action)
    {
        int idx = static_cast<int>(action);
        if (idx >= 0 && idx < static_cast<int>(Action::COUNT))
            return sActionNames[idx];
        return "Unknown";
    }

    const char* GetKeyName(ImGuiKey key)
    {
        return ImGui::GetKeyName(key);
    }

    std::string GetKeybindString(Action action)
    {
        auto it = sBindings.find(action);
        if (it == sBindings.end())
            return "None";

        const auto& bind = it->second;
        std::string result;

        if (bind.ctrl) result += "Ctrl+";
        if (bind.shift) result += "Shift+";
        if (bind.alt) result += "Alt+";
        result += GetKeyName(bind.key);

        return result;
    }

    bool IsPressed(Action action)
    {
        auto it = sBindings.find(action);
        if (it == sBindings.end()) return false;

        const auto& bind = it->second;
        if (bind.key == ImGuiKey_None) return false;

        bool ctrlHeld = ImGui::GetIO().KeyCtrl;
        bool shiftHeld = ImGui::GetIO().KeyShift;
        bool altHeld = ImGui::GetIO().KeyAlt;

        if (bind.ctrl != ctrlHeld) return false;
        if (bind.shift != shiftHeld) return false;
        if (bind.alt != altHeld) return false;

        return ImGui::IsKeyPressed(bind.key, false);
    }

    bool IsDown(Action action)
    {
        auto it = sBindings.find(action);
        if (it == sBindings.end()) return false;

        const auto& bind = it->second;
        if (bind.key == ImGuiKey_None) return false;

        bool ctrlHeld = ImGui::GetIO().KeyCtrl;
        bool shiftHeld = ImGui::GetIO().KeyShift;
        bool altHeld = ImGui::GetIO().KeyAlt;

        if (bind.ctrl != ctrlHeld) return false;
        if (bind.shift != shiftHeld) return false;
        if (bind.alt != altHeld) return false;

        return ImGui::IsKeyDown(bind.key);
    }

    std::unordered_map<Action, Keybind>& GetAllBindings()
    {
        return sBindings;
    }

    std::unordered_map<Action, Keybind> GetBindingsCopy()
    {
        return sBindings;
    }

    void ApplyBindings(const std::unordered_map<Action, Keybind>& bindings)
    {
        sBindings = bindings;
    }
}