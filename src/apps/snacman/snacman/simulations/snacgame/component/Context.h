#pragma once

#include "../InputCommandConverter.h"

#include "../context/InputDeviceDirectory.h"

#include <platform/Filesystem.h>

namespace ad {
namespace snacgame {
namespace component {

struct Context
{
    Context(InputDeviceDirectory aDeviceDirectory) :
        mInputDeviceDirectory{aDeviceDirectory}
        mGamepadMapping("/home/franz/snac-assets/gamepadmapping.json"),
        mKeyboardMapping("/home/franz/snac-assets/gamepadmapping.json")
    {
    }

    InputDeviceDirectory mInputDeviceDirectory;
    GamepadMapping mGamepadMapping;
    KeyboardMapping mKeyboardMapping;
};

}
}
}
