#pragma once

#include "../InputCommandConverter.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <limits>
#include <string>

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
};

} // namespace component
} // namespace snacgame
} // namespace ad
