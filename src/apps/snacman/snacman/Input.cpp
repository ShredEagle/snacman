#include "Input.h"
#include <graphics/AppInterface.h>
#include <graphics/ApplicationGlfw.h>

#include <functional>
#include <cmath>

namespace ad {

AxisStatus & AxisStatus::operator =(const float & aRhs)
{
    mPrevious = mCurrent;
    mCurrent = aRhs;

    return *this;
}

AxisStatus::operator ButtonStatus() const
{
    // Conversion table looks like this
    // p    0 1 0 1
    // ~p   1 0 1 0
    // c    0 0 1 1
    // r    1 0 3 2
    //
    // with ButtonStatus arranged as it is this makes an easy conversion
    // from AxisStatus to ButtonStatus
    int previousBit = static_cast<int>(std::abs(mPrevious) >= gJoystickDeadzone) << 0;
    int currentBit = static_cast<int>(std::abs(mCurrent) >= gJoystickDeadzone) << 1;

    return static_cast<ButtonStatus>(
        (~previousBit & 1 << 0) | (currentBit & 1 << 1)
    );
}

AxisStatus::operator float() const
{
    return mCurrent;
}

InputState::InputState(const bool & aBool) :
    mState{aBool ? ButtonStatus::Pressed : ButtonStatus::Released}
{}
InputState::InputState(const float & aFloat) :
    mState{AxisStatus{aFloat}}
{}

InputState & InputState::operator =(const bool & aRhs)
{
    std::get<ButtonStatus>(mState) = aRhs ? ButtonStatus::Pressed : ButtonStatus::Released;
    return *this;
}

InputState & InputState::operator =(const float & aRhs)
{
    std::get<AxisStatus>(mState) = aRhs;
    return *this;
}

InputState::operator AxisStatus() const
{ return std::get<AxisStatus>(mState); }

InputState::operator ButtonStatus() const
{ return std::get<ButtonStatus>(mState); }

InputState::operator bool() const
{ return isPressed(); }

bool InputState::isPressed() const
{ return mState >= std::variant<ButtonStatus, AxisStatus>{ButtonStatus::Pressed}; }


namespace snac {

namespace {

void handleButtonEdges(InputState & aInputState, bool aCurrentlyPressed)
{
    if (aInputState && aCurrentlyPressed)
    {
        aInputState.mState = ButtonStatus::Pressed;
    }
    else if (aInputState && !aCurrentlyPressed)
    {
        aInputState.mState = ButtonStatus::NegativeEdge;
    }
    else if (!aInputState && aCurrentlyPressed)
    {
        aInputState.mState = ButtonStatus::PositiveEdge;
    }
    else
    {
        aInputState.mState = ButtonStatus::Released;
    }
}

} // anonymous namespace

HidManager::HidManager(graphics::ApplicationGlfw & aApplication)
{
    using namespace std::placeholders;
    std::shared_ptr<graphics::AppInterface> appInterface =
        aApplication.getAppInterface();

    appInterface->registerCursorPositionCallback(
        std::bind(&HidManager::callbackCursorPosition, this, _1, _2));

    appInterface->registerMouseButtonCallback(
        std::bind(&HidManager::callbackMouseButton, this, _1, _2, _3, _4, _5));

    appInterface->registerScrollCallback(
        std::bind(&HidManager::callbackScroll, this, _1, _2));

    appInterface->registerKeyCallback(
        std::bind(&HidManager::callbackKeyboardStroke, this, _1, _2, _3, _4));

    double x, y;
    glfwGetCursorPos(aApplication.getGlfwWindow(), &x, &y);
    callbackCursorPosition(x, y);

    for (InputState & mouseButton : mMouse.mMouseButtons)
    {
        mouseButton = false;
    };

    mKeyState.fill(false);
}

RawInput HidManager::initialInput() const
{
    return RawInput{
        .mMouse{
            .mCursorPosition = mMouse.mCursorPosition,
        },
    };
}

RawInput HidManager::read(const RawInput & aPrevious,
                          const Inhibiter & aInhibiter)
{
    RawInput result{
        .mMouse{mMouse.diff(aPrevious.mMouse)},
        .mKeyboard = aPrevious.mKeyboard,
        .mGamepads = aPrevious.mGamepads,
    };

    if (!aInhibiter.isCapturingMouse())
    {
        for (std::size_t id = 0; id != mMouse.mMouseButtons.size(); ++id)
        {
            handleButtonEdges(result.mMouse.mMouseButtons.at(id),
                              mMouse.mMouseButtons.at(id));
        }

        result.mMouse.mScrollOffset = mMouse.mScrollOffset;
    }

    if (!aInhibiter.isCapturingKeyboard())
    {
        for (std::size_t id = 0; id != mKeyState.size(); ++id)
        {
            handleButtonEdges(result.mKeyboard.mKeyState.at(id),
                              mKeyState.at(id));
        }
    }

    for (std::size_t joystickId = 0; joystickId != result.mGamepads.size();
         ++joystickId)
    {
        GamepadState & gamepadState = result.mGamepads.at(joystickId);
        GLFWgamepadstate rawGamepadState;
        int connected =
            glfwGetGamepadState(static_cast<int>(joystickId), &rawGamepadState);
        gamepadState.mConnected = connected;

        if (connected)
        {
            for (std::size_t buttonId = 0; buttonId < gamepadState.mButtons.size(); ++buttonId)
            {
                handleButtonEdges(gamepadState.mButtons.at(buttonId),
                                  rawGamepadState.buttons[buttonId]);
            }

            for (std::size_t axisId = 0; axisId < gamepadState.mAxis.size(); ++axisId)
            {
                float axisValue = rawGamepadState.axes[axisId];
                if (axisId == GLFW_GAMEPAD_AXIS_LEFT_TRIGGER || axisId == GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER)
                {
                    axisValue = (axisValue + 1.f) / 2.f;
                }
                gamepadState.mAxis.at(axisId) = axisValue;
            }
        }
    }

    mMouse.mScrollOffset.setZero();

    return result;
}

void HidManager::callbackCursorPosition(double xpos, double ypos)
{
    mMouse.mCursorPosition = {static_cast<float>(xpos),
                              static_cast<float>(ypos)};
}

void HidManager::callbackMouseButton(
    int button, int action, int mods, double xpos, double ypos)
{
    // constexpr std::array<int, static_cast<std::size_t>(MouseButton::_End)>
    // gMapping
    //{
    //     GLFW_MOUSE_BUTTON_LEFT,
    //     GLFW_MOUSE_BUTTON_MIDDLE,
    //     GLFW_MOUSE_BUTTON_RIGHT,
    // };

    MouseButton mouseButton = [&button]() {
        switch (button)
        {
        case GLFW_MOUSE_BUTTON_LEFT:
            return MouseButton::Left;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            return MouseButton::Middle;
        case GLFW_MOUSE_BUTTON_RIGHT:
            return MouseButton::Right;
        default:
            return MouseButton::_End;
        }
    }();

    if (mouseButton != MouseButton::_End)
    {
        mMouse.mMouseButtons.at(static_cast<ButtonEnum_t>(mouseButton)) =
            (action != GLFW_RELEASE);
    }
}

void HidManager::callbackScroll(double xoffset, double yoffset)
{
    // In case the callbacks can be called several times...
    mMouse.mScrollOffset += math::Vec<2, float>{static_cast<float>(xoffset),
                                                static_cast<float>(yoffset)};
}

void HidManager::callbackKeyboardStroke(int key,
                                        int scancode,
                                        int action,
                                        int mods)
{
    const char * keyName = glfwGetKeyName(key, 0);

    if (keyName && keyName[0] && keyName[1] == 0)
    {
        mKeyState.at(keyName[0]) = (action != GLFW_RELEASE);
    }
    else
    {
        mKeyState.at(key) = (action != GLFW_RELEASE);
    }
}

} // namespace snac
} // namespace ad
