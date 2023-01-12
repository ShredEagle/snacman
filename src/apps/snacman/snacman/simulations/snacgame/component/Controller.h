#pragma once


#include "snacman/simulations/snacgame/InputCommandConverter.h"

namespace ad {
namespace snacgame {
namespace component {

enum class ControllerType
{
    Keyboard,
    Gamepad,
};

struct Controller
{
    ControllerType mType = ControllerType::Keyboard;
    int mId = -1;
    int mCommandQuery;
};

} // namespace component
} // namespace snacgame
} // namespace ad
