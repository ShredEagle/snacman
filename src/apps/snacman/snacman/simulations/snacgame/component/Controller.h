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
    int mCommandQuery = 0;
    int mControllerId;
};

} // namespace component
} // namespace snacgame
} // namespace ad
