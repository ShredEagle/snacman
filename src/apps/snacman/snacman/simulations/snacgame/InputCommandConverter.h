#pragma once

#include "snacman/Input.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <GLFW/glfw3.h>

#include <concepts>
#include <initializer_list>

using json = nlohmann::json;

namespace ad {
namespace snacgame {

constexpr int gPlayerMoveFlagNone = 0x0;
constexpr int gPlayerMoveFlagUp = 0x1;
constexpr int gPlayerMoveFlagDown = 0x2;
constexpr int gPlayerMoveFlagLeft = 0x4;
constexpr int gPlayerMoveFlagRight = 0x8;

constexpr int gQuitCommand = 0x100;

template<class T_input_type>
struct InputMappingDictionnary
{
    InputMappingDictionnary(std::initializer_list<std::pair<std::string, T_input_type>> aStringInputList) :
        mMap{createMap(aStringInputList)},
        mReverseMap{createReverseMap(aStringInputList)}
    {}
    
    static std::map<std::string, T_input_type> createMap(std::vector<std::pair<std::string, T_input_type>> aStringInputList)
    {
        std::map<std::string, T_input_type> newMap;

        for (const auto & [name, input] : aStringInputList)
        {
            newMap.insert_or_assign(name, input);
        }

        return newMap;
    }

    static std::map<T_input_type, std::string> createReverseMap(std::vector<std::pair<std::string, T_input_type>> aStringInputList)
    {
        std::map<T_input_type, std::string> newMap;

        for (const auto & [name, input] : aStringInputList)
        {
            newMap.insert_or_assign(input, name);
        }

        return newMap;
    }

    const T_input_type lookup(const std::string & aName) const
    {
        return mMap.at(aName);
    }

    const std::string reverseLookup(const T_input_type & aInput) const
    {
        return mReverseMap.at(aInput);
    }

    const std::map<std::string, T_input_type> mMap;
    const std::map<T_input_type, std::string> mReverseMap;
};

const inline InputMappingDictionnary<GamepadAtomicInput> gGamepadMappingDictionnary{
    {"JOY_LEFT_UP", GamepadAtomicInput::leftYAxisPositive},
    {"JOY_LEFT_DOWN", GamepadAtomicInput::leftYAxisNegative},
    {"JOY_LEFT_LEFT", GamepadAtomicInput::leftXAxisNegative},
    {"JOY_LEFT_RIGHT", GamepadAtomicInput::leftXAxisPositive},
    {"SELECT", GamepadAtomicInput::guide},
};

const inline InputMappingDictionnary<int> gKeyboardMappingDictionnary{
    {"ctrl", GLFW_KEY_LEFT_CONTROL},
    {"esc", GLFW_KEY_ESCAPE},
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

    KeyMapping(filesystem::path aPath) :
        mKeymaps{}
    {
        std::ifstream configStream(aPath);

        json data = json::parse(configStream);

        mKeymaps.push_back({translateMappingValueToInputType<T_input_type>(data["gPlayerMoveFlagUp"]), gPlayerMoveFlagUp});
        mKeymaps.push_back({translateMappingValueToInputType<T_input_type>(data["gPlayerMoveFlagDown"]), gPlayerMoveFlagDown});
        mKeymaps.push_back({translateMappingValueToInputType<T_input_type>(data["gPlayerMoveFlagLeft"]), gPlayerMoveFlagLeft});
        mKeymaps.push_back({translateMappingValueToInputType<T_input_type>(data["gPlayerMoveFlagRight"]), gPlayerMoveFlagRight});
        mKeymaps.push_back({translateMappingValueToInputType<T_input_type>(data["gQuitCommand"]), gQuitCommand});
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
