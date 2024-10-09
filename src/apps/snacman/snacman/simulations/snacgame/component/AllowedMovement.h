#pragma once

#include "snacman/serialization/Witness.h"

#include <snac-reflexion/Reflexion.h>
#include <snac-reflexion/Reflexion_impl.h>
#include <reflexion/NameValuePair.h>

#include "handy/StringId.h"

#include "../GameParameters.h"
#include "../InputCommandConverter.h"

#include <memory>

namespace ad {
namespace snacgame {
namespace component {

struct AllowedMovement
{
    int mAllowedMovement = 0;
    float mWindow = gPlayerTurningZoneHalfWidth;

    void drawUi() const;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mAllowedMovement));
        aWitness.witness(NVP(mWindow));
    }
};

REFLEXION_REGISTER(AllowedMovement)

} // namespace component
} // namespace snacgame
} // namespace ad
