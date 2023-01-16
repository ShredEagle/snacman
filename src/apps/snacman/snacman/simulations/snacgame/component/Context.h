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
        mInputDeviceDirectory{aDeviceDirectory},
        mGamepadMapping("/home/franz/gamedev/snac-assets/settings/gamepad_mapping.json"),
        mKeyboardMapping("/home/franz/gamedev/snac-assets/settings/keyboard_mapping.json")
    {
    }

    InputDeviceDirectory mInputDeviceDirectory;
    GamepadMapping mGamepadMapping;
    KeyboardMapping mKeyboardMapping;
};

}
}
}
