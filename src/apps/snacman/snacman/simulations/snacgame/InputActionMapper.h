#include "snacman/Input.h"
#include "snacman/simulations/snacgame/component/PlayerAction.h"
#include <GLFW/glfw3.h>

#include <concepts>

namespace ad {
namespace snacgame {

typedef int PlayerActionFlag;

constexpr PlayerActionFlag gPlayerMoveRight = 0;
constexpr PlayerActionFlag gPlayerMoveLeft = 0b1;
constexpr PlayerActionFlag gPlayerMoveUp = 0b10;
constexpr PlayerActionFlag gPlayerMoveDown = 0b100;

struct GamepadMapping
{
    static constexpr std::size_t size = static_cast<std::size_t>(snac::GamepadAtomicCommand::End);
    std::array<PlayerActionFlag,
                         GamepadMapping::size> mapping;

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

    constexpr KeyboardMapping() :
        mapping{}
    {
        for (std::size_t i = 0; i != mapping.size(); ++i)
        {
            if (i == GLFW_KEY_Z)
            {
                mapping.at(i) = gPlayerMoveUp;
            }
            else if (i == GLFW_KEY_Q)
            {
                mapping.at(i) = gPlayerMoveDown;
            }
            else if (i == GLFW_KEY_D)
            {
                mapping.at(i) = gPlayerMoveRight;
            }
            else if (i == GLFW_KEY_S)
            {
                mapping.at(i) = gPlayerMoveLeft;
            }
        }
    }
};

constexpr KeyboardMapping gKeyboardDefaultMapping();
constexpr GamepadMapping gGamepadDefaultMapping();

template<class T>
concept Mapping = requires(T a)
{
    {T::size} -> std::convertible_to<std::size_t>;
    {a.mapping} -> std::convertible_to<std::array<PlayerActionFlag, T::size>>;
    // TODO: this is wonky but i don't know how to do
    std::is_convertible_v<typename T::command_type, std::size_t>;
};

template<Mapping T_mapping, T_mapping V_mapping>
class ActionKeyMapper
{
    PlayerActionFlag get(typename T_mapping::command_type aCommand)
    {
        return mKeymaps.at(static_cast<size_t>(aCommand));
    }

    void setKeyMapping(typename T_mapping::command_type aCommand, PlayerActionFlag aAction)
    {
        mKeymaps.at(static_cast<size_t>(aCommand)) = aAction;
    }

    std::array<PlayerActionFlag,
               static_cast<std::size_t>(T_mapping::size)>
        mKeymaps = V_mapping;
};

template<KeyboardMapping V_mapping>
class ActionKeyMapper<KeyboardMapping, V_mapping>;

template<GamepadMapping V_mapping>
class ActionKeyMapper<GamepadMapping, V_mapping>;

ActionKeyMapper<gKeyboardDefaultMapping> allo;

} // namespace snacgame
} // namespace ad
