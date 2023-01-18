#pragma once

#include "../InputCommandConverter.h"

#include "../context/InputDeviceDirectory.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <platform/Filesystem.h>
#include <string>

namespace ad {
namespace snacgame {
namespace component {

struct Context
{
    Context(InputDeviceDirectory aDeviceDirectory) :
        mInputDeviceDirectory{aDeviceDirectory},
        mGamepadMapping("/home/franz/gamedev/snac-assets/settings/gamepad_mapping.json"),
        mKeyboardMapping("/home/franz/gamedev/snac-assets/settings/keyboard_mapping.json")
    {
    }

    InputDeviceDirectory mInputDeviceDirectory;
    GamepadMapping mGamepadMapping;
    KeyboardMapping mKeyboardMapping;

    void drawUi()
    {
        ImGui::Begin("Keyboard mappings");
        for (auto & pair : mKeyboardMapping.mKeymaps)
        {
            std::string key(1, static_cast<char>(pair.first));
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

            struct CallbackData {
                int & mCurrentKey;
            };
            
            CallbackData cbData{pair.first};

            ImGui::InputText(moveName.c_str(), (char*)key.c_str(), key.capacity() + 1, 0, [](ImGuiInputTextCallbackData * data) -> int
                    {
                        std::string newText(data->Buf, data->BufTextLen);
                        CallbackData * myData = (CallbackData *)data->UserData;
                        for (auto character : newText)
                        {
                            if (character != myData->mCurrentKey)
                            {
                                myData->mCurrentKey = character;
                            }
                        }

                        return 0;
                    }, &cbData);
            if (key.size() != 0)
            {
                pair.first = key.at(0);
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
