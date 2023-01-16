#pragma once

#include "snacman/Input.h"


#include <fstream>
#include <nlohmann/json.hpp>
#include <GLFW/glfw3.h>

#include <concepts>

using json = nlohmann::json;

namespace ad {
namespace snacgame {

constexpr int gPlayerMoveFlagNone = 0x0;
constexpr int gPlayerMoveFlagUp = 0x1;
constexpr int gPlayerMoveFlagDown = 0x2;
constexpr int gPlayerMoveFlagLeft = 0x4;
constexpr int gPlayerMoveFlagRight = 0x8;

constexpr int gQuitCommand = 0x100;

// Maybe we should just give up on GamepadAtomicInput
// being a class enum and this template would
// dissapear
template <class T_input_type>
class KeyMapping
{
public:
    using input_type = T_input_type;

    KeyMapping(const std::vector<std::pair<T_input_type, int>> & aMapping) :
        mKeymaps{aMapping}
    {}

    KeyMapping(filesystem::path aPath) :
        mKeymaps{}
    {
        std::ifstream configStream(aPath);

        json data = json::parse(configStream);

        mKeymaps.push_back({static_cast<T_input_type>(data["gPlayerMoveFlagUp"]), gPlayerMoveFlagUp});
        mKeymaps.push_back({static_cast<T_input_type>(data["gPlayerMoveFlagDown"]), gPlayerMoveFlagDown});
        mKeymaps.push_back({static_cast<T_input_type>(data["gPlayerMoveFlagLeft"]), gPlayerMoveFlagLeft});
        mKeymaps.push_back({static_cast<T_input_type>(data["gPlayerMoveFlagRight"]), gPlayerMoveFlagRight});
        mKeymaps.push_back({static_cast<T_input_type>(data["gQuitCommand"]), gQuitCommand});
    }

    int get(T_input_type aInput)
    {
        for (const auto & [input, command] : mKeymaps)
        {
            if (input == aInput)
            {
                return command;
            }
        }
    }

    void setKeyMapping(T_input_type aInput, int aCommand)
    {
        for (auto & [input, command] : mKeymaps)
        {
            if (input == aInput)
            {
                command = aCommand;
            }
        }
    }

    std::vector<std::pair<T_input_type, int>> mKeymaps;
};


using GamepadMapping = KeyMapping<GamepadAtomicInput>;

using KeyboardMapping = KeyMapping<int>;

inline int convertKeyboardInput(
        const KeyboardState & aKeyboardState, const KeyboardMapping & aKeyboardMapping)
{
    int commandFlags = 0;

    for (const auto & [input, command] : aKeyboardMapping.mKeymaps)
    {
        InputState state = aKeyboardState.mKeyState.at(static_cast<size_t>(input));
        commandFlags |= state ? command : 0;
    }

    return commandFlags;
}

inline int convertGamepadInput(
        const GamepadState & aGamepadState, const GamepadMapping & aGamepadMapping)
{
    int commandFlags = 0;

    for (const auto & [input, command] : aGamepadMapping.mKeymaps)
    {
        commandFlags |= aGamepadState.mAtomicInputList.at(static_cast<size_t>(input)).isPressed();
    }

    return commandFlags;
}

} // namespace snacgame
} // namespace ad
