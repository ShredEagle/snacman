#pragma once

#include <cmath>
#include <graphics/AppInterface.h>
#include <graphics/ApplicationGlfw.h>

#include <math/Vector.h>

#include <type_traits>
#include <variant>

namespace ad {

// Forward
namespace graphics
{
    class ApplicationGlfw;
} // namespace graphics

constexpr std::size_t gGlfwButtonListSize = GLFW_GAMEPAD_BUTTON_LAST + 1;
constexpr std::size_t gGlfwAxisListSize = GLFW_GAMEPAD_AXIS_LAST + 1;
constexpr std::size_t gGlfwJoystickListSize = GLFW_JOYSTICK_LAST + 1;
constexpr std::size_t gGlfwKeyboardListSize = GLFW_KEY_LAST + 1;

constexpr float gJoystickDeadzone = 0.4f;
constexpr float gTriggerDeadzone = 0.1f;

using ButtonEnum_t = std::int8_t;

enum class ButtonStatus : ButtonEnum_t
{
    // Values are set because their values
    // is important for AxisStatus to 
    // ButtonStatus conversion
    NegativeEdge = 0, // just released
    Released = 1,
    Pressed = 2,
    PositiveEdge = 3, // just pressed
};

enum class AxisName : ButtonEnum_t
{
    LeftX = GLFW_GAMEPAD_AXIS_LEFT_X,
    LeftY = GLFW_GAMEPAD_AXIS_LEFT_Y,
    RightX = GLFW_GAMEPAD_AXIS_RIGHT_X,
    RightY = GLFW_GAMEPAD_AXIS_RIGHT_Y,
    LeftTrigger = GLFW_GAMEPAD_AXIS_LEFT_TRIGGER,
    RightTrigger = GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER,
};

struct AxisStatus
{
    AxisStatus & operator =(const float & aRhs);
    
    operator ButtonStatus() const;

    operator float() const;

    bool operator ==(const AxisStatus & aRhs) const
    { return mCurrent == aRhs.mCurrent; }
    bool operator >(const AxisStatus & aRhs) const
    { return mCurrent > aRhs.mCurrent; }
    bool operator <(const AxisStatus & aRhs) const
    { return mCurrent > aRhs.mCurrent; }
    bool operator <=(const AxisStatus & aRhs) const
    { return *this < aRhs || *this == aRhs; }
    bool operator >=(const AxisStatus & aRhs) const
    { return *this > aRhs || *this == aRhs; }

    float mPrevious;
    float mCurrent;
};

struct InputState
{
    using Enum_t = std::underlying_type_t<ButtonStatus>;

    InputState() = default;
    InputState(const bool & aBool);
    InputState(const float & aFloat);

    InputState & operator =(const bool & aRhs);
    InputState & operator =(const float & aRhs);

    operator AxisStatus() const;

    operator ButtonStatus() const;

    operator bool() const;

    bool isPressed() const;

    std::variant<ButtonStatus, AxisStatus> mState{ButtonStatus::Released};
};

// Note: used to index into an array of ButtonStatus
enum class MouseButton
{
    Left,
    Middle,
    Right,

    _End/* Keep last */
};

enum class GamepadAtomicInput
{
    a = GLFW_GAMEPAD_BUTTON_A,
    b = GLFW_GAMEPAD_BUTTON_B,
    x = GLFW_GAMEPAD_BUTTON_X,
    y = GLFW_GAMEPAD_BUTTON_Y,
    leftBumper = GLFW_GAMEPAD_BUTTON_LEFT_BUMPER,
    rightBumper = GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER,
    back = GLFW_GAMEPAD_BUTTON_BACK,
    start = GLFW_GAMEPAD_BUTTON_START,
    guide = GLFW_GAMEPAD_BUTTON_GUIDE,
    leftThumb = GLFW_GAMEPAD_BUTTON_LEFT_THUMB,
    rightThumb = GLFW_GAMEPAD_BUTTON_RIGHT_THUMB,
    dPadUp = GLFW_GAMEPAD_BUTTON_DPAD_UP,
    dPadDown = GLFW_GAMEPAD_BUTTON_DPAD_DOWN,
    dPadRight = GLFW_GAMEPAD_BUTTON_DPAD_RIGHT,
    dPadLeft = GLFW_GAMEPAD_BUTTON_DPAD_LEFT,

    lastButton = GLFW_GAMEPAD_BUTTON_LAST,

    // We can't separate axis from buttons
    // because the mapping maps a GamepadAtomicInput
    // To a specific player command
    // Thus we need gamepad atomic input to be one enum
    // not two. This is why we flag axis atomic inputs
    axisFlag = 1 << 4,
    positiveFlag = 1 << 5,
    leftXAxisPositive = GLFW_GAMEPAD_AXIS_LEFT_X | axisFlag | positiveFlag,
    leftXAxisNegative = GLFW_GAMEPAD_AXIS_LEFT_X | axisFlag,
    leftYAxisPositive = GLFW_GAMEPAD_AXIS_LEFT_Y | axisFlag | positiveFlag,
    leftYAxisNegative = GLFW_GAMEPAD_AXIS_LEFT_Y | axisFlag,
    rightXAxisPositive = GLFW_GAMEPAD_AXIS_RIGHT_X | axisFlag | positiveFlag,
    rightXAxisNegative = GLFW_GAMEPAD_AXIS_RIGHT_X | axisFlag,
    rightYAxisPositive = GLFW_GAMEPAD_AXIS_RIGHT_Y | axisFlag | positiveFlag,
    rightYAxisNegative = GLFW_GAMEPAD_AXIS_RIGHT_Y | axisFlag,
    leftTrigger = GLFW_GAMEPAD_AXIS_LEFT_TRIGGER | axisFlag,
    rightTrigger = GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER | axisFlag,
};

struct GamepadState
{
    GamepadState()
    {
        for (auto & axis : mAxis)
        {
            axis.mState = AxisStatus{0.f};
        }
    }
    bool mConnected = false;
    std::array<InputState, gGlfwButtonListSize> mButtons;
    std::array<InputState, gGlfwAxisListSize> mAxis;
    bool mBound = false;
};

struct KeyboardState
{
    std::array<InputState, static_cast<std::size_t>(gGlfwKeyboardListSize)> mKeyState;
    bool mBound = false;
};

struct MouseState
{
    MouseState diff(const MouseState & aPrevious)
    {
        return MouseState{
            .mCursorPosition = mCursorPosition,
            .mCursorDisplacement = mCursorPosition - aPrevious.mCursorPosition,
            .mMouseButtons = aPrevious.mMouseButtons,
        };
    }

    const InputState & get(MouseButton aButton) const
    { return mMouseButtons.at(static_cast<ButtonEnum_t>(aButton)); }

    math::Position<2, float> mCursorPosition;
    math::Vec<2, float> mCursorDisplacement;
    math::Vec<2, float> mScrollOffset{0.f, 0.f};

    std::array<InputState, static_cast<std::size_t>(MouseButton::_End)> mMouseButtons;
};

/// \brief Produced by the HidManager
struct RawInput
{
    MouseState mMouse;
    KeyboardState mKeyboard;
    std::array<GamepadState, gGlfwJoystickListSize> mGamepads;
};

namespace snac {

/// \brief Handles all the input devices, and produce the `Input` instance that is consumed by the simulation.
class HidManager
{
public:
    // Note: Could not find a way for clang to accept Null as class nested under Inhibiter.
    class InhibiterNull;

    class Inhibiter
    {
    public:
        virtual bool isCapturingMouse() const = 0;
        virtual bool isCapturingKeyboard() const = 0;

        virtual ~Inhibiter() = default;

        static const InhibiterNull gNull;
    };

    class InhibiterNull final : public Inhibiter
    {
    public:
        bool isCapturingMouse() const override
        { return false; }
        bool isCapturingKeyboard() const override
        { return false; }
    };

    HidManager(graphics::ApplicationGlfw & aApplication);

    RawInput initialInput() const;

    RawInput read(const RawInput & aPrevious, const Inhibiter & aInhibiter = Inhibiter::gNull);

private:
    // TODO Answer: might the callback be called several times between/by glfwPollEvents()?
    void callbackCursorPosition(double xpos, double ypos);
    void callbackMouseButton(int button, int action, int mods, double xpos, double ypos);
    void callbackScroll(double xoffset, double yoffset);
    void callbackKeyboardStroke(int key, int scancode, int action, int mods);

    MouseState mMouse;

    // TODO could be made more compact, only need a bit per button (Franz says 2 bits per buttons, Release, neg, pos and pressed)
    std::array<bool, static_cast<std::size_t>(gGlfwKeyboardListSize)> mKeyState;
};


// Note: must be defined within HidManager (not after) because it is used as a default value on read().


inline const HidManager::InhibiterNull HidManager::Inhibiter::gNull{};


} // namespace snac
} // namespace ad

namespace std {
    template<>
    struct hash<ad::GamepadAtomicInput>
    {
        size_t operator()(const ad::GamepadAtomicInput & aAtomicInput) const
        {
            return hash<int>()(static_cast<int>(aAtomicInput));
        }
    };
};
