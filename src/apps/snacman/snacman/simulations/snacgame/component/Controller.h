#pragma once

#include "../InputCommandConverter.h"

#include <snacman/Input.h>

namespace ad {
namespace snacgame {
namespace component {

struct Controller
{
    ControllerType mType = ControllerType::Keyboard;
    GameInput mInput;
    unsigned int mControllerId;

    void drawUi() const;
};

} // namespace component
} // namespace snacgame
} // namespace ad
