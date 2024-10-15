#pragma once

#include "math/Box.h"
#include "reflexion/Reflexion.h"

#include <chrono>
#include <math/Vector.h>
#include <reflexion/NameValuePair.h>

#include <vector>
#include <string>
#include <map>
#include <unordered_map>

struct CompA
{
    int aInt;
    bool aBool;
    std::string aString;
    float aFloat;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(aInt));
        aWitness.witness(NVP(aBool));
        aWitness.witness(NVP(aString));
        aWitness.witness(NVP(aFloat));
    }
};

REFLEXION_REGISTER(CompA)

struct CompB
{
    std::array<int, 8> aArray;
    std::vector<int> aVector;
    std::map<std::string, int> aMap;
    std::unordered_map<std::string, int> aUnorderedMap;
    ad::math::Vec<3, float> aMathVec;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(aArray));
        aWitness.witness(NVP(aVector));
        aWitness.witness(NVP(aMap));
        aWitness.witness(NVP(aUnorderedMap));
        aWitness.witness(NVP(aMathVec));
    }
};

REFLEXION_REGISTER(CompB)

struct CompPart
{
    int aInt;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(aInt));
    }
};

REFLEXION_REGISTER(CompPart)

struct CompC
{
    ad::ent::Handle<ad::ent::Entity> aOther;
    CompPart aCompPart;
    std::vector<CompPart> aCompPartVector;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(aOther));
        aWitness.witness(NVP(aCompPart));
        aWitness.witness(NVP(aCompPartVector));
    }
};

REFLEXION_REGISTER(CompC)

enum class ControllerType
{
    Keyboard,
    Gamepad,
    Dummy,
};

struct GameInput
{
    // input flags
    int mCommand = 0;
    // Left joystick and zqsd
    ad::math::Vec<2, float> mLeftDirection;
    // Right joystick and up, down, left, right
    ad::math::Vec<2, float> mRightDirection;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mCommand));
        aWitness.witness(NVP(mLeftDirection));
        aWitness.witness(NVP(mRightDirection));
    }
};

struct CompSnacTest
{
    ad::math::Box<float> mPlayerHitbox;
    ControllerType mType;
    GameInput mInput;
    std::chrono::high_resolution_clock::time_point mStartTime;
    // math::ParameterAnimation<float, math::AnimationResult::Clamp> mParameter;
    // math::Quaternion<float> mOrientation = math::Quaternion<float>::Identity();
    // math::hdr::Rgba_f mColor = math::hdr::gWhite<float>;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mPlayerHitbox));
        aWitness.witness(NVP(mType));
        aWitness.witness(NVP(mInput));
        aWitness.witness(NVP(mStartTime));
    }
};

REFLEXION_REGISTER(CompSnacTest)
