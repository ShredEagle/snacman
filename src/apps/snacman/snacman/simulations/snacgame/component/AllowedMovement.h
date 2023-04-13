#pragma once


#include "../GameParameters.h"
#include "../InputCommandConverter.h"
namespace ad {
namespace snacgame {
namespace component {

struct AllowedMovement
{
    int mAllowedMovement = 0;
    float mWindow = gPlayerTurningZoneHalfWidth;

    void drawUi() const;
};

} // namespace component
} // namespace snacgame
} // namespace ad
