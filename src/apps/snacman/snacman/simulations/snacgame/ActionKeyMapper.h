#pragma once

#include "snacman/Input.h"

#include <concepts>
#include <GLFW/glfw3.h>

namespace ad {
namespace snacgame {

typedef int PlayerActionFlag;

constexpr PlayerActionFlag gPlayerMoveRight = 0x0;
constexpr PlayerActionFlag gPlayerMoveLeft = 0x1;
constexpr PlayerActionFlag gPlayerMoveUp = 0x2;
constexpr PlayerActionFlag gPlayerMoveDown = 0x4;

struct GamepadMapping
{
    static constexpr std::size_t size = static_cast<std::size_t>(snac::GamepadAtomicCommand::End);
    std::array<PlayerActionFlag, GamepadMapping::size> mapping;

    using command_type = snac::GamepadAtomicCommand;

    constexpr GamepadMapping() :
        mapping{
            gPlayerMoveRight, // leftXAxisPositive,
            gPlayerMoveLeft,  // leftXAxisNegative,
            gPlayerMoveUp,    // leftYAxisPositive,
            gPlayerMoveDown,  // leftYAxisNegative,
            -1,               // rightXAxisPositive,
            -1,               // rightXAxisNegative,
            -1,               // rightYAxisPositive,
            -1,               // rightYAxisNegative,
            -1,               // a,
            -1,               // b,
            -1,               // x,
            -1,               // y,
            -1,               // leftBumper,
            -1,               // rightBumper,
            -1,               // leftTrigger,
            -1,               // rightTrigger,
        }
    {}
};

struct KeyboardMapping
{
    static constexpr std::size_t size = snac::gGlfwKeyboardListSize;
    std::array<PlayerActionFlag, KeyboardMapping::size> mapping;
    using command_type = int;

    constexpr KeyboardMapping() : mapping{}
    {
        mapping.fill(-1);
        for (std::size_t i = 0; i != mapping.size(); ++i)
        {
            if (i == GLFW_KEY_W)
            {
                mapping.at(i) = gPlayerMoveUp;
            }
            else if (i == GLFW_KEY_S)
            {
                mapping.at(i) = gPlayerMoveDown;
            }
            else if (i == GLFW_KEY_D)
            {
                mapping.at(i) = gPlayerMoveRight;
            }
            else if (i == GLFW_KEY_A)
            {
                mapping.at(i) = gPlayerMoveLeft;
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
        } -> std::convertible_to<std::array<PlayerActionFlag, T::size>>;
    std::is_convertible_v<typename T::command_type, std::size_t>;
};

template <auto V_mapping>
requires Mapping<decltype(V_mapping)>
class ActionKeyMapper
{
public:
    using T_mapping = decltype(V_mapping);

    PlayerActionFlag get(typename T_mapping::command_type aCommand)
    {
        return mKeymaps.at(static_cast<size_t>(aCommand));
    }

    void setKeyMapping(typename T_mapping::command_type aCommand, PlayerActionFlag aAction)
    {
        mKeymaps.at(static_cast<size_t>(aCommand)) = aAction;
    }

    std::array<PlayerActionFlag, static_cast<std::size_t>(T_mapping::size)> mKeymaps =
        V_mapping.mapping;
};

} // namespace snacgame
} // namespace ad
