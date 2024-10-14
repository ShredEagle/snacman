#pragma once

#include "snacman/serialization/Witness.h"

#include <snac-reflexion/Reflexion.h>
#include <snac-reflexion/Reflexion_impl.h>
#include <reflexion/NameValuePair.h>

#include "../InputCommandConverter.h"

#include <snacman/Input.h>

namespace ad {
namespace snacgame {
namespace component {

struct Controller
{
    ControllerType mType = ControllerType::Keyboard;
    GameInput mInput;
    unsigned int mControllerId = 0;

    void drawUi() const;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mType));
        aWitness.witness(NVP(mInput));
        aWitness.witness(NVP(mControllerId));
    }
};

REFLEXION_REGISTER(Controller)

} // namespace component
} // namespace snacgame
} // namespace ad
