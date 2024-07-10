#pragma once

#include "../InputCommandConverter.h"

#include <snacman/detail/Reflexion.h>
#include <snacman/detail/Reflexion_impl.h>
#include <snacman/detail/Serialization.h>

namespace ad {

struct RawInput;
namespace snac { class Resources; }

namespace snacgame {

namespace component {

struct MappingContext
{
    MappingContext(const snac::Resources & aResources);

    void drawUi(const RawInput & aInput, bool * open = nullptr);

    GamepadMapping mGamepadMapping;
    KeyboardMapping mKeyboardMapping;

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mGamepadMapping);
        archive & SERIAL_PARAM(mKeyboardMapping);
    }
};

SNAC_SERIAL_REGISTER(MappingContext)

} // namespace component
} // namespace snacgame
} // namespace ad
