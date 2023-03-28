#pragma once



#include "snacman/Input.h"

namespace ad {
namespace snacgame {
namespace component {

struct Controller
{
    ControllerType mType = ControllerType::Keyboard;
    int mCommandQuery = 0;
    int mControllerId;

    void drawUi() const;
};

} // namespace component
} // namespace snacgame
} // namespace ad
