#pragma once


#include <graphics/AppInterface.h>
#include <graphics/ApplicationGlfw.h>

#include <math/Vector.h>

#include <type_traits>


namespace ad {


// Forward
namespace graphics
{
    class ApplicationGlfw;
} // namespace graphics


namespace snac {


using ButtonEnum_t = std::int8_t;

enum class ButtonStatus : ButtonEnum_t
{
    Released,
    NegativeEdge, // just released
    Pressed,
    PositiveEdge, // just pressed
};


struct InputState
{
    using Enum_t = std::underlying_type_t<ButtonStatus>;

    operator ButtonStatus() const
    { return mState; }

    operator bool() const
    { return isPressed(); }

    bool isPressed() const
    { return static_cast<Enum_t>(mState) >= static_cast<Enum_t>(ButtonStatus::Pressed); }

    ButtonStatus mState{ButtonStatus::Released};
};

// Note: used to index into an array of ButtonStatus
enum class MouseButton
{
    Left,
    Middle,
    Right,

    _End/* Keep last
*/};

constexpr std::size_t gGlfwButtonListSize = GLFW_GAMEPAD_BUTTON_LAST + 1;
constexpr std::size_t gGlfwJoystickListSize = GLFW_JOYSTICK_LAST + 1;
constexpr std::size_t gGlfwKeyboardListSize = GLFW_KEY_LAST + 1;

// What is a mapping
// a mapping links an input device type and an atomic command
// to an action
// For snacman there will be a mapping for keyboard
// and a mapping for gamepad
// There won't be a mapping that mix device type
// a player will be linked to a keyboard or a gamepad
// and its action will be extracted from that device


// Gamepad command needs to be enriched to be able to map
// action to a signed axis command
// one other solution would be to simplify with only one
// command for all axis and associate a move action to that
// command and do the differentiation in a system
// if so axis should be one command and dpad should be one command
enum class GamepadAtomicCommand
{
    leftXAxisPositive,
    leftXAxisNegative,
    leftYAxisPositive,
    leftYAxisNegative,
    rightXAxisPositive,
    rightXAxisNegative,
    rightYAxisPositive,
    rightYAxisNegative,
    a,
    b,
    x,
    y,
    dPadRight,
    dPadLeft,
    dPadUp,
    dPadDown,
    leftBumper,
    rightBumper,
    leftTrigger,
    rightTrigger,

    End, // keep last
};

struct GamepadState
{
    bool mConnected = false;
    std::array<InputState, gGlfwButtonListSize> mButtons;
    math::Vec<2, float> mLeftJoystick;
    math::Vec<2, float> mRightJoystick;
    float mLeftTrigger = 0.f;
    float mRightTrigger = 0.f;
    std::array<InputState, static_cast<std::size_t>(GamepadAtomicCommand::End)> mAtomicCommands;
};

struct KeyboardState
{
    std::array<InputState, static_cast<std::size_t>(gGlfwKeyboardListSize)> mKeyState;
};



/// \brief Produced by the HidManager
struct Input
{
    const InputState & get(MouseButton aButton) const
    { return mMouseButtons[static_cast<ButtonEnum_t>(aButton)]; }

    const GamepadState & get(int aJoystickId) const
    { return mGamepads.at(aJoystickId); }

    math::Position<2, float> mCursorPosition;
    math::Vec<2, float> mCursorDisplacement;
    math::Vec<2, float> mScrollOffset{0.f, 0.f};

    std::array<InputState, static_cast<std::size_t>(MouseButton::_End)> mMouseButtons;
    std::array<GamepadState, gGlfwJoystickListSize> mGamepads;
    KeyboardState mKeyboard;
};


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

    Input initialInput() const;

    Input read(const Input & aPrevious, const Inhibiter & aInhibiter = Inhibiter::gNull);

private:
    // TODO Answer: might the callback be called several times between/by glfwPollEvents()?
    void callbackCursorPosition(double xpos, double ypos);
    void callbackMouseButton(int button, int action, int mods, double xpos, double ypos);
    void callbackScroll(double xoffset, double yoffset);
    void callbackKeyboardStroke(int key, int scancode, int action, int mods);

    math::Position<2, float> mCursorPosition;
    // TODO could be made more compact, only need a bit per button (Franz says 2 bits per buttons, Release, neg, pos and pressed)
    std::array<bool, static_cast<std::size_t>(MouseButton::_End)> mMouseButtons;
    math::Vec<2, float> mScrollOffset{0.f, 0.f};

    // TODO could be made more compact, only need a bit per button (Franz says 2 bits per buttons, Release, neg, pos and pressed)
    // However it would introduce a fucking long list of masks
    // right now this is sparse because GLFW_KEY_* is sparse
    std::array<bool, static_cast<std::size_t>(gGlfwKeyboardListSize)> mKeyState;
};


// Note: must be defined within HidManager (not after) because it is used as a default value on read().


inline const HidManager::InhibiterNull HidManager::Inhibiter::gNull{};


} // namespace snac
} // namespace ad
