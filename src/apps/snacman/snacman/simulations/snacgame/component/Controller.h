#pragma once

#include "../InputCommandConverter.h"

#include <snacman/Input.h>

#include <snacman/detail/Reflexion.h>
#include <snacman/detail/Reflexion_impl.h>
#include <snacman/detail/Serialization.h>

namespace ad {
namespace snacgame {
namespace component {

struct Controller
{
    ControllerType mType = ControllerType::Keyboard;
    GameInput mInput;
    unsigned int mControllerId;

    void drawUi() const;
    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mType);
        archive & SERIAL_PARAM(mInput);
        archive & SERIAL_PARAM(mControllerId);
    }
};

SNAC_SERIAL_REGISTER(Controller)

} // namespace component
} // namespace snacgame
} // namespace ad
