#pragma once

#include "handy/StringId.h"

#include "../GameParameters.h"
#include "../InputCommandConverter.h"

#include <memory>
#include <snacman/detail/Reflexion.h>

namespace ad {
namespace snacgame {
namespace component {

struct AllowedMovement
{
    int mAllowedMovement = 0;
    float mWindow = gPlayerTurningZoneHalfWidth;

    void drawUi() const;

};

SNAC_SERIAL_REGISTER(AllowedMovement)

} // namespace component
} // namespace snacgame
} // namespace ad
