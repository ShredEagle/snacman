#pragma once

#include "InputConstants.h"
#include "component/Controller.h"
#include "snacman/Logging.h"

#include <platform/Filesystem.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#undef GLFW_INCLUDE_NONE

#include <nlohmann/json.hpp>

#include <concepts>
#include <fstream>
#include <initializer_list>
#include <unordered_map>

using json = nlohmann::json;

namespace ad {
namespace snacgame {

template <class T_forward>
concept Mappable = requires { typename std::map<T_forward, int>; };

template <Mappable T_forward, Mappable T_reverse>
struct BidirectionalMap
{
    BidirectionalMap(
        std::initializer_list<std::pair<T_forward, T_reverse>> aInitList) :
        mMap{createMap(aInitList)}, mReverseMap{createReverseMap(aInitList)}
    {}

    static std::unordered_map<T_forward, T_reverse>
    createMap(std::vector<std::pair<T_forward, T_reverse>> aInitList)
    {
        std::unordered_map<T_forward, T_reverse> newMap;

        for (const auto & [name, input] : aInitList)
        {
            newMap.insert_or_assign(name, input);
        }

        return newMap;
    }

    static std::unordered_map<T_reverse, T_forward>
    createReverseMap(std::vector<std::pair<T_forward, T_reverse>> aInitList)
    {
        std::unordered_map<T_reverse, T_forward> newMap;

        for (const auto & [name, input] : aInitList)
        {
            newMap.insert_or_assign(input, name);
        }

        return newMap;
    }

    const T_reverse lookup(const T_forward & aName) const
    {
        return mMap.at(aName);
    }

    const T_forward reverseLookup(const T_reverse & aInput) const
    {
        return mReverseMap.at(aInput);
    }

    const bool contains(const T_forward & aName) const
    {
        return mMap.contains(aName);
    }

    const bool rcontains(const T_reverse & aName) const
    {
        return mReverseMap.contains(aName);
    }

    const std::unordered_map<T_forward, T_reverse> mMap;
    const std::unordered_map<T_reverse, T_forward> mReverseMap;
};

const inline BidirectionalMap<std::string, int> gCommandFlags{
    {"gPlayerMoveFlagNone", gPlayerMoveFlagNone},

    {"gQuitCommand", gQuitCommand | gPositiveEdge},

    {"gPlayerMoveFlagUp", gPlayerMoveFlagUp},
    {"gPlayerMoveFlagDown", gPlayerMoveFlagDown},
    {"gPlayerMoveFlagLeft", gPlayerMoveFlagLeft},
    {"gPlayerMoveFlagRight", gPlayerMoveFlagRight},
    {"gPlayerUsePowerup", gPlayerUsePowerup | gPositiveEdge},

    {"gJoin", gJoin | gPositiveEdge},

    {"gSelectItem", gSelectItem | gPositiveEdge},
    {"gBack", gBack | gPositiveEdge},
    {"gGoUp", gGoUp},
    {"gGoDown", gGoDown},
    {"gGoLeft", gGoLeft},
    {"gGoRight", gGoRight},
};

const inline BidirectionalMap<std::string, GamepadAtomicInput>
    gGamepadMappingDictionnary{
        {"A", GamepadAtomicInput::a},
        {"B", GamepadAtomicInput::b},
        {"X", GamepadAtomicInput::x},
        {"Y", GamepadAtomicInput::y},
        {"LEFT_BUMPER", GamepadAtomicInput::leftBumper},
        {"RIGHT_BUMPER", GamepadAtomicInput::rightBumper},
        {"SELECT", GamepadAtomicInput::back},
        {"START", GamepadAtomicInput::start},
        {"LEFT_THUMB", GamepadAtomicInput::leftThumb},
        {"RIGHT_THUMB", GamepadAtomicInput::rightThumb},
        {"DPAD_UP", GamepadAtomicInput::dPadUp},
        {"DPAD_DOWN", GamepadAtomicInput::dPadDown},
        {"DPAD_LEFT", GamepadAtomicInput::dPadLeft},
        {"DPAD_RIGHT", GamepadAtomicInput::dPadRight},
        {"JOY_LEFT_UP", GamepadAtomicInput::leftYAxisNegative},
        {"JOY_LEFT_DOWN", GamepadAtomicInput::leftYAxisPositive},
        {"JOY_LEFT_LEFT", GamepadAtomicInput::leftXAxisNegative},
        {"JOY_LEFT_RIGHT", GamepadAtomicInput::leftXAxisPositive},
        {"JOY_RIGHT_UP", GamepadAtomicInput::rightYAxisNegative},
        {"JOY_RIGHT_DOWN", GamepadAtomicInput::rightYAxisPositive},
        {"JOY_RIGHT_LEFT", GamepadAtomicInput::rightXAxisNegative},
        {"JOY_RIGHT_RIGHT", GamepadAtomicInput::rightXAxisPositive},
        {"LEFT_TRIGGER", GamepadAtomicInput::leftTrigger},
        {"RIGHT_TRIGGER", GamepadAtomicInput::rightTrigger},
    };

const inline BidirectionalMap<std::string, int> gKeyboardMappingDictionnary{
    {"ctrl", GLFW_KEY_LEFT_CONTROL},
    {"esc", GLFW_KEY_ESCAPE},
    {"up", GLFW_KEY_UP},
    {"down", GLFW_KEY_DOWN},
    {"space", GLFW_KEY_SPACE},
    {"left", GLFW_KEY_LEFT},
    {"right", GLFW_KEY_RIGHT},
    {"enter", GLFW_KEY_ENTER},
    {"backspace", GLFW_KEY_BACKSPACE},
};

template <class T_return_type>
inline T_return_type
translateMappingValueToInputType(const std::string & aMappingValue);

template <>
inline GamepadAtomicInput
translateMappingValueToInputType(const std::string & aMappingValue)
{
    return gGamepadMappingDictionnary.lookup(aMappingValue);
}

template <>
inline int translateMappingValueToInputType(const std::string & aMappingValue)
{
    if (aMappingValue.size() == 1)
    {
        char key = aMappingValue.at(0);

        if (key >= 'A' && key <= 'Z')
        {
            return 'a' + (key - 'A');
        }

        return key;
    }
    else
    {
        return gKeyboardMappingDictionnary.lookup(aMappingValue);
    }

    return 0;
}

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

    KeyMapping(const filesystem::path & aPath) : mKeymaps{}
    {
        std::ifstream configStream(aPath);

        json data = json::parse(configStream);

        for (const auto & [group, mappings] :
             data.get<
                 std::map<std::string, std::map<std::string, std::string>>>())
        {
            [[maybe_unused]] auto [it, success] = mKeymaps.insert({group, {}});
            std::vector<std::pair<T_input_type, int>> & groupMappings =
                it->second;
            for (const auto & [commandName, key] : mappings)
            {
                if (gCommandFlags.contains(commandName))
                {
                    groupMappings.push_back(
                        {translateMappingValueToInputType<T_input_type>(key),
                         gCommandFlags.lookup(commandName)});
                }
                else
                {
                    SELOG(error)("Missing command name \"{}\" present in \"{}\" in command flags", commandName, aPath.c_str());
                }
            }
        }
    }

    int get(const std::string & aGroup, T_input_type aInput) const
    {
        for (const auto & [input, command] : mKeymaps.at(aGroup))
        {
            if (input == aInput)
            {
                return command;
            }
        }
    }

    void
    setKeyMapping(const std::string & aGroup, T_input_type aInput, int aCommand)
    {
        for (auto & [input, command] : mKeymaps.at(aGroup))
        {
            if (input == aInput)
            {
                command = aCommand;
            }
        }
    }

    std::map<std::string, std::vector<std::pair<T_input_type, int>>> mKeymaps;
};

using GamepadMapping = KeyMapping<GamepadAtomicInput>;

using KeyboardMapping = KeyMapping<int>;

struct ControllerCommand {
    int mCommand;
    int mId;
    ControllerType mControllerType;
};

inline int convertKeyboardInput(const std::string & aGroup,
                                const KeyboardState & aKeyboardState,
                                const KeyboardMapping & aKeyboardMapping)
{
    int commandFlags = 0;

    for (const auto & [input, command] : aKeyboardMapping.mKeymaps.at(aGroup))
    {
        const InputState & state =
            aKeyboardState.mKeyState.at(static_cast<size_t>(input));
        ButtonStatus stateWanted = command & gPositiveEdge
                                       ? ButtonStatus::PositiveEdge
                                       : ButtonStatus::Pressed;
        commandFlags |= (static_cast<InputState::Enum_t>(
                             std::get<ButtonStatus>(state.mState))
                         >= static_cast<InputState::Enum_t>(stateWanted))
                            ? (command & ~gPositiveEdge)
                            : 0;
    }

    return commandFlags;
}

inline int convertGamepadInput(const std::string & aGroup,
                               const GamepadState & aGamepadState,
                               const GamepadMapping & aGamepadMapping)
{
    int commandFlags = 0;

    for (const auto & [input, command] : aGamepadMapping.mKeymaps.at(aGroup))
    {
        if (input <= GamepadAtomicInput::lastButton)
        {
            const InputState & state =
                aGamepadState.mButtons.at(static_cast<int>(input));
            ButtonStatus stateWanted = command & gPositiveEdge
                                           ? ButtonStatus::PositiveEdge
                                           : ButtonStatus::Pressed;
            commandFlags |= (static_cast<InputState::Enum_t>(
                                 std::get<ButtonStatus>(state.mState))
                             >= static_cast<InputState::Enum_t>(stateWanted))
                                ? (command & ~gPositiveEdge)
                                : 0;
        }
        else
        {
            int index = static_cast<int>(input)
                & ~static_cast<int>(GamepadAtomicInput::axisFlag)
                & ~static_cast<int>(GamepadAtomicInput::positiveFlag);
            const AxisStatus & state = aGamepadState.mAxis.at(index);

            if (static_cast<int>(input)
                & static_cast<int>(GamepadAtomicInput::positiveFlag))
            {
                commandFlags |= (static_cast<float>(state) > gJoystickDeadzone)
                                    ? (command & ~gPositiveEdge)
                                    : 0;
            }
            else
            {
                commandFlags |= (static_cast<float>(state) < -gJoystickDeadzone)
                                    ? (command & ~gPositiveEdge)
                                    : 0;
            }
        }
    }

    return commandFlags;
}

} // namespace snacgame
} // namespace ad
