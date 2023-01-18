#pragma once

#include "snacman/Input.h"
#include "../InputCommandConverter.h"

#include "../context/InputDeviceDirectory.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <limits>
#include <platform/Filesystem.h>
#include <string>

namespace ad {
namespace snacgame {
namespace component {

struct Context
{
    Context(InputDeviceDirectory aDeviceDirectory) :
        mInputDeviceDirectory{aDeviceDirectory},
        mGamepadMapping("d:/projects/gamedev/2/snac-assets/settings/gamepad_mapping.json"),
        mKeyboardMapping("d:/projects/gamedev/2/snac-assets/settings/keyboard_mapping.json")
        //mGamepadMapping("/home/franz/gamedev/snac-assets/settings/gamepad_mapping.json"),
        //mKeyboardMapping("/home/franz/gamedev/snac-assets/settings/keyboard_mapping.json")
    {
    }

    InputDeviceDirectory mInputDeviceDirectory;
    GamepadMapping mGamepadMapping;
    KeyboardMapping mKeyboardMapping;

    void drawUi(const RawInput & aInput)
    {
        ImGui::Begin("Keyboard mappings");
        static int selected = -1;
        for (auto & pair : mKeyboardMapping.mKeymaps)
        {
            int move = pair.second;
            std::string moveName{""};
            switch(move)
            {
                case gPlayerMoveFlagUp:
                    moveName = "Up";
                    break;
                case gPlayerMoveFlagDown:
                    moveName = "Down";
                    break;
                case gPlayerMoveFlagLeft:
                    moveName = "Left";
                    break;
                case gPlayerMoveFlagRight:
                    moveName = "Right";
                    break;
                case gQuitCommand:
                    moveName = "Escape";
                    break;
            }

            std::string key(1, static_cast<char>(pair.first));

            if (pair.first > std::numeric_limits<char>::max() || key.at(0) == 0)
            {
                key = gKeyboardMappingDictionnary.reverseLookup(pair.first);
            }

            ImGui::Text("%s:", moveName.c_str());
            ImGui::SameLine();
            if (ImGui::Selectable(key.c_str(), selected == pair.second))
            {
                selected = pair.second;

            }
            if (selected == pair.second)
            {
                int index = 0;
                for (const InputState & aState : aInput.mKeyboard.mKeyState)
                {
                    if (static_cast<ButtonStatus>(aState) == ButtonStatus::PositiveEdge)
                    {
                        pair.first = index;
                        selected = -1;
                        break;
                    }
                    index++;
                }
            }
        }
        ImGui::End();

        ImGui::Begin("Gamepad mappings");
        for (auto & pair : mGamepadMapping.mKeymaps)
        {
            int key = static_cast<int>(pair.first);
            int move = pair.second;
            std::string moveName{""};
            switch(move)
            {
                case gPlayerMoveFlagUp:
                    moveName = "Up";
                    break;
                case gPlayerMoveFlagDown:
                    moveName = "Down";
                    break;
                case gPlayerMoveFlagLeft:
                    moveName = "Left";
                    break;
                case gPlayerMoveFlagRight:
                    moveName = "Right";
                    break;
                case gQuitCommand:
                    moveName = "Escape";
                    break;
            }
            ImGui::InputInt(moveName.c_str(), &key);
            pair.first = static_cast<GamepadAtomicInput>(key);
        }
        ImGui::End();
    }
};

}
}
}
