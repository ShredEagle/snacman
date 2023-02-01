#pragma once

#include "snacman/Input.h"
#include "../InputCommandConverter.h"

#include <platform/Filesystem.h>

#include <resource/ResourceFinder.h>

#include <GLFW/glfw3.h>

#include <imgui.h>

#include <limits>
#include <string>


namespace ad {
namespace snacgame {

//const inline filesystem::path gAssetRoot{"/home/franz/utilise-le-resource-finder/gamedev/snac-assets"};
const inline filesystem::path gAssetRoot{"d:/projects/gamedev/2/snac-assets"};

namespace component {

struct MappingContext
{
    MappingContext(const resource::ResourceFinder aResourceFinder) :
        mGamepadMapping(*aResourceFinder.find("settings/gamepad_mapping.json")),
        mKeyboardMapping(*aResourceFinder.find("settings/keyboard_mapping.json"))
    {}

    void drawUi(const RawInput & aInput)
    {
        ImGui::Begin("Keyboard mappings");
        static int selected = -1;
        for (auto & [groupName, mappings] : mKeyboardMapping.mKeymaps)
        {
            if (ImGui::CollapsingHeader(groupName.c_str()))
            {
                for (auto & pair : mappings)
                {
                    int move = pair.second;
                    std::string moveName{gCommandFlags.reverseLookup(move)};

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
            }
        }
        ImGui::End();

        ImGui::Begin("Gamepad mappings");
        for (auto & [groupName, mapping] : mGamepadMapping.mKeymaps)
        {
            if (ImGui::CollapsingHeader(groupName.c_str()))
            {
                for (auto & pair : mapping)
                {
                    int key = static_cast<int>(pair.first);
                    int move = pair.second;
                    std::string moveName{gCommandFlags.reverseLookup(move)};
                    ImGui::InputInt(moveName.c_str(), &key);
                    pair.first = static_cast<GamepadAtomicInput>(key);
                }
            }
        }
        ImGui::End();
    }

    GamepadMapping mGamepadMapping;
    KeyboardMapping mKeyboardMapping;
};

}
}
}
