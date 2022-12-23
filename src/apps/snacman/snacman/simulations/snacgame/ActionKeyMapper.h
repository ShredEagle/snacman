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

struct GamepadMapping
{
    static constexpr std::size_t size = static_cast<std::size_t>(snac::GamepadAtomicCommand::End);
    std::array<int, GamepadMapping::size> mapping;

    using command_type = snac::GamepadAtomicCommand;

    constexpr GamepadMapping() :
        mapping{
            gPlayerMoveFlagRight, // leftXAxisPositive,
            gPlayerMoveFlagLeft,  // leftXAxisNegative,
            gPlayerMoveFlagUp,    // leftYAxisPositive,
            gPlayerMoveFlagDown,  // leftYAxisNegative,
            gPlayerMoveFlagNone,               // rightXAxisPositive,
            gPlayerMoveFlagNone,               // rightXAxisNegative,
            gPlayerMoveFlagNone,               // rightYAxisPositive,
            gPlayerMoveFlagNone,               // rightYAxisNegative,
            gPlayerMoveFlagNone,               // a,
            gPlayerMoveFlagNone,               // b,
            gPlayerMoveFlagNone,               // x,
            gPlayerMoveFlagNone,               // y,
            gPlayerMoveFlagNone,               // leftBumper,
            gPlayerMoveFlagNone,               // rightBumper,
            gPlayerMoveFlagNone,               // leftTrigger,
            gPlayerMoveFlagNone,               // rightTrigger,
        }
    {}
};

struct KeyboardMapping
{
    static constexpr std::size_t size = snac::gGlfwKeyboardListSize;
    std::array<int, KeyboardMapping::size> mapping;
    using command_type = int;

    constexpr KeyboardMapping() : mapping{}
    {
        mapping.fill(gPlayerMoveFlagNone);
        for (std::size_t i = 0; i != mapping.size(); ++i)
        {
            if (i == GLFW_KEY_W)
            {
                mapping.at(i) = gPlayerMoveFlagUp;
            }
            else if (i == GLFW_KEY_S)
            {
                mapping.at(i) = gPlayerMoveFlagDown;
            }
            else if (i == GLFW_KEY_D)
            {
                mapping.at(i) = gPlayerMoveFlagRight;
            }
            else if (i == GLFW_KEY_A)
            {
                mapping.at(i) = gPlayerMoveFlagLeft;
            }
        }
    }
};

constexpr KeyboardMapping gKeyboardDefaultMapping = KeyboardMapping();
constexpr GamepadMapping gGamepadDefaultMapping = GamepadMapping();

template <class T>
concept Mapping = requires(T a)
{
    {
        T::size
        } -> std::convertible_to<std::size_t>;
    {
        a.mapping
        } -> std::convertible_to<std::array<int, T::size>>;
    std::is_convertible_v<typename T::command_type, std::size_t>;
};

template <auto V_mapping>
requires Mapping<decltype(V_mapping)>
class ActionKeyMapper
{
public:
    using T_mapping = decltype(V_mapping);

    int get(typename T_mapping::command_type aCommand)
    {
        return mKeymaps.at(static_cast<size_t>(aCommand));
    }

    void setKeyMapping(typename T_mapping::command_type aCommand, int aAction)
    {
        mKeymaps.at(static_cast<size_t>(aCommand)) = aAction;
    }

    std::array<int, static_cast<std::size_t>(T_mapping::size)> mKeymaps =
        V_mapping.mapping;
};

} // namespace snacgame
} // namespace ad
