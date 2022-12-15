#pragma once


namespace ad {
namespace snacgame {
namespace component {

enum class ControllerType
{
    Keyboard,
    Gamepad,
};

struct Controller
{
    ControllerType mType = ControllerType::Keyboard;
    int mId = -1;
};

} // namespace component
} // namespace snacgame
} // namespace ad
