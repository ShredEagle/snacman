#pragma once

#include <math/Color.h>
#include <snacman/Input.h>

namespace ad {
namespace snacgame {
namespace component {

struct Unspawned
{};

struct PlayerSlot
{
    unsigned int mSlotIndex;
    unsigned int mControllerIndex;
    ControllerType mControllerType;
};

} // namespace component
} // namespace snacgame
} // namespace ad
