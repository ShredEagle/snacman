#pragma once

#include "snacman/Input.h"

#include <concepts>
#include <GLFW/glfw3.h>

namespace ad {
namespace snacgame {

constexpr int gPlayerMoveFlagNone = 0x0;
constexpr int gPlayerMoveFlagUp = 0x1;
constexpr int gPlayerMoveFlagDown = 0x2;
constexpr int gPlayerMoveFlagLeft = 0x4;
constexpr int gPlayerMoveFlagRight = 0x8;

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
