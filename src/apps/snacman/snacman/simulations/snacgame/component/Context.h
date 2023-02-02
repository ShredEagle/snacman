#pragma once

#include "snacman/Input.h"

#include "../InputCommandConverter.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <limits>
#include <platform/Filesystem.h>
#include <resource/ResourceFinder.h>
#include <string>

namespace ad {
namespace snacgame {

const inline filesystem::path gAssetRoot{"/home/franz/gamedev/snac-assets"};
// const inline filesystem::path
// gAssetRoot{"d:/projects/gamedev/2/snac-assets"};

namespace component {

struct Context
{
    Context(const resource::ResourceFinder aResourceFinder) :
        mGamepadMapping(*aResourceFinder.find("settings/gamepad_mapping.json")),
        mKeyboardMapping(
            *aResourceFinder.find("settings/keyboard_mapping.json"))
    {}

    GamepadMapping mGamepadMapping;
    KeyboardMapping mKeyboardMapping;

    void drawUi(const RawInput & aInput)
    {
        ImGui::Begin("Keyboard mappings");
        static int keyboardSelected = -1;
        for (auto & [groupName, mappings] : mKeyboardMapping.mKeymaps)
        {
            if (ImGui::CollapsingHeader(groupName.c_str()))
            {
                for (auto & pair : mappings)
                {
                    int move = pair.second;
                    std::string moveName{gCommandFlags.reverseLookup(move)};

                    std::string key(1, static_cast<char>(pair.first));

                    if (pair.first > std::numeric_limits<char>::max()
                        || key.at(0) == 0)
                    {
                        key = gKeyboardMappingDictionnary.reverseLookup(
                            pair.first);
                    }

                    ImGui::Text("%s:", moveName.c_str());
                    ImGui::SameLine();
                    if (ImGui::Selectable(key.c_str(),
                                          keyboardSelected == pair.second))
                    {
                        keyboardSelected = pair.second;
                    }
                    if (keyboardSelected == pair.second)
                    {
                        int index = 0;
                        for (const InputState & aState :
                             aInput.mKeyboard.mKeyState)
                        {
                            if (static_cast<ButtonStatus>(aState)
                                == ButtonStatus::PositiveEdge)
                            {
                                pair.first = index;
                                keyboardSelected = -1;
                                break;
                            }
                            index++;
                        }
                    }
                }
            }
        }
        ImGui::End();

        static int gamepadSelected = -1;

        ImGui::Begin("Gamepad mappings");
        for (auto & [groupName, mapping] : mGamepadMapping.mKeymaps)
        {
            if (ImGui::CollapsingHeader(groupName.c_str()))
            {
                for (auto & pair : mapping)
                {
                    int move = pair.second;
                    std::string moveName{gCommandFlags.reverseLookup(move)};

                    std::string key(1, static_cast<char>(pair.first));

                    key = gGamepadMappingDictionnary.reverseLookup(pair.first);

                    ImGui::Text("%s:", moveName.c_str());
                    ImGui::SameLine();
                    if (ImGui::Selectable(key.c_str(),
                                          gamepadSelected == pair.second))
                    {
                        gamepadSelected = pair.second;
                    }
                    if (gamepadSelected == pair.second)
                    {
                        for (const GamepadState & gamepad : aInput.mGamepads)
                        {
                            for (int index = 0; index < gamepad.mButtons.size();
                                 ++index)
                            {
                                const InputState & buttonState =
                                    gamepad.mButtons.at(index);
                                if (static_cast<ButtonStatus>(buttonState)
                                    == ButtonStatus::PositiveEdge)
                                {
                                    pair.first =
                                        static_cast<GamepadAtomicInput>(index);
                                    gamepadSelected = -1;
                                    break;
                                }
                            }
                            for (int index = 0; index < gamepad.mAxis.size();
                                 ++index)
                            {
                                const InputState & axisState =
                                    gamepad.mAxis.at(index);
                                const AxisStatus & status =
                                    std::get<AxisStatus>(axisState.mState);
                                if (static_cast<ButtonStatus>(status)
                                    == ButtonStatus::PositiveEdge)
                                {
                                    int stuff = index
                                                | static_cast<int>(
                                                    GamepadAtomicInput::
                                                        axisFlag)
                                                | static_cast<int>(status.mCurrent > 0.f
                                            ? static_cast<int>(GamepadAtomicInput::positiveFlag)
                                            : 0);
                                    GamepadAtomicInput newStuff = static_cast<GamepadAtomicInput>(stuff);
                                    pair.first = newStuff;
                                    gamepadSelected = -1;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
        ImGui::End();
    }
};

} // namespace component
} // namespace snacgame
} // namespace ad
