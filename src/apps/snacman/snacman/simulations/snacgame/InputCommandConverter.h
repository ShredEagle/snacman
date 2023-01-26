#pragma once

#include "snacman/Input.h"

#include <nlohmann/json.hpp>
#include <GLFW/glfw3.h>

#include <fstream>
#include <concepts>
#include <initializer_list>
#include <unordered_map>

using json = nlohmann::json;

namespace ad {
namespace snacgame {

constexpr int gPlayerMoveFlagNone = 0;

constexpr int gQuitCommand = 1 << 0;

constexpr int gPlayerMoveFlagUp = 1 << 1;
constexpr int gPlayerMoveFlagDown = 1 << 2;
constexpr int gPlayerMoveFlagLeft = 1 << 3;
constexpr int gPlayerMoveFlagRight = 1 << 4;

constexpr int gJoin = 1 << 5;

constexpr int gSelectItem = 1 << 6;
constexpr int gBack = 1 << 7;
constexpr int gGoUp = 1 << 8;
constexpr int gGoDown = 1 << 9;
constexpr int gGoLeft = 1 << 10;
constexpr int gGoRight = 1 << 11;


template<class T_forward>
concept Mappable = requires
{
    typename std::map<T_forward, int>;
};

template<Mappable T_forward, Mappable T_reverse>
struct BidirectionalMap
{
    BidirectionalMap(std::initializer_list<std::pair<T_forward, T_reverse>> aInitList) :
        mMap{createMap(aInitList)},
        mReverseMap{createReverseMap(aInitList)}
    {}
    
    static std::unordered_map<T_forward, T_reverse> createMap(std::vector<std::pair<T_forward, T_reverse>> aInitList)
    {
        std::unordered_map<T_forward, T_reverse> newMap;

        for (const auto & [name, input] : aInitList)
        {
            newMap.insert_or_assign(name, input);
        }

        return newMap;
    }

    static std::unordered_map<T_reverse, T_forward> createReverseMap(std::vector<std::pair<T_forward, T_reverse>> aInitList)
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

    const std::unordered_map<T_forward, T_reverse> mMap;
    const std::unordered_map<T_reverse, T_forward> mReverseMap;
};

const inline BidirectionalMap<std::string, int> gCommandFlags{
    {"gPlayerMoveFlagNone", gPlayerMoveFlagNone},

    {"gQuitCommand", gQuitCommand},

    {"gPlayerMoveFlagUp", gPlayerMoveFlagUp},
    {"gPlayerMoveFlagDown", gPlayerMoveFlagDown},
    {"gPlayerMoveFlagLeft", gPlayerMoveFlagLeft},
    {"gPlayerMoveFlagRight", gPlayerMoveFlagRight},

    {"gJoin", gJoin},

    {"gSelectItem", gSelectItem},
    {"gBack", gBack},
    {"gGoUp", gGoUp},
    {"gGoDown", gGoDown},
    {"gGoLeft", gGoLeft},
    {"gGoRight", gGoRight},
};


const inline BidirectionalMap<std::string, GamepadAtomicInput> gGamepadMappingDictionnary{
    {"JOY_LEFT_UP", GamepadAtomicInput::leftYAxisPositive},
    {"JOY_LEFT_DOWN", GamepadAtomicInput::leftYAxisNegative},
    {"JOY_LEFT_LEFT", GamepadAtomicInput::leftXAxisNegative},
    {"JOY_LEFT_RIGHT", GamepadAtomicInput::leftXAxisPositive},
    {"SELECT", GamepadAtomicInput::guide},
    {"B", GamepadAtomicInput::b},
    {"START", GamepadAtomicInput::start},

};

const inline BidirectionalMap<std::string, int> gKeyboardMappingDictionnary{
    {"ctrl", GLFW_KEY_LEFT_CONTROL},
    {"esc", GLFW_KEY_ESCAPE},
    {"up", GLFW_KEY_UP},
    {"down", GLFW_KEY_DOWN},
    {"left", GLFW_KEY_LEFT},
    {"right", GLFW_KEY_RIGHT},
    {"enter", GLFW_KEY_ENTER},
    {"backspace", GLFW_KEY_BACKSPACE},
};


template<class T_return_type>
inline T_return_type translateMappingValueToInputType(const std::string & aMappingValue);

template<>
inline GamepadAtomicInput translateMappingValueToInputType(const std::string & aMappingValue)
{
    return gGamepadMappingDictionnary.lookup(aMappingValue);
}

template<>
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

    KeyMapping(const filesystem::path & aPath) :
        mKeymaps{}
    {
        std::ifstream configStream(aPath);

        json data = json::parse(configStream);

        for (const auto & [group, mappings] : data.get<std::map<std::string, std::map<std::string, std::string>>>())
        {
            [[maybe_unused]] auto [it, success] = mKeymaps.insert({group, {}});
            std::vector<std::pair<T_input_type, int>> & groupMappings = it->second;
            for (const auto & [commandName, key] : mappings)
            {
                groupMappings.push_back({translateMappingValueToInputType<T_input_type>(key), gCommandFlags.lookup(commandName)});
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

    void setKeyMapping(const std::string & aGroup, T_input_type aInput, int aCommand)
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

inline int convertKeyboardInput(
        const std::string & aGroup, const KeyboardState & aKeyboardState, const KeyboardMapping & aKeyboardMapping)
{
    int commandFlags = 0;

    for (const auto & [input, command] : aKeyboardMapping.mKeymaps.at(aGroup))
    {
        InputState state = aKeyboardState.mKeyState.at(static_cast<size_t>(input));
        commandFlags |= state ? command : 0;
    }

    return commandFlags;
}

inline int convertGamepadInput(
        const std::string & aGroup,
        const GamepadState & aGamepadState, const GamepadMapping & aGamepadMapping)
{
    //int commandFlags = 0;

    //for (const auto & [input, command] : aGamepadMapping.mKeymaps)
    //{
    //    commandFlags |= aGamepadState.mAtomicInputList.at(static_cast<size_t>(input)).isPressed();
    //}

    //return commandFlags;
    throw std::runtime_error("Not implemented.");
}

} // namespace snacgame
} // namespace ad
