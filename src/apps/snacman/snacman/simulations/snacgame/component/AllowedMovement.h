#pragma once

#include "handy/StringId.h"

#include "../GameParameters.h"
#include "../InputCommandConverter.h"

#include <memory>
#include <snacman/detail/Reflexion.h>
#include <snacman/detail/Reflexion_impl.h>
#include <snacman/detail/Serialization.h>

namespace ad {
namespace snacgame {
namespace component {

struct AllowedMovement
{
    int mAllowedMovement = 0;
    float mWindow = gPlayerTurningZoneHalfWidth;

    void drawUi() const;

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mAllowedMovement);
        archive & SERIAL_PARAM(mWindow);
    }
};

SNAC_SERIAL_REGISTER(AllowedMovement)

} // namespace component
} // namespace snacgame
} // namespace ad
