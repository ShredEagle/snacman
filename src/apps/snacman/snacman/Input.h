#pragma once


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



/// \brief Produced by the HidManager
struct Input
{
    const InputState & get(MouseButton aButton) const
    { return mMouseButtons[static_cast<ButtonEnum_t>(aButton)]; }

    math::Position<2, float> mCursorPosition;
    math::Vec<2, float> mCursorDisplacement;
    math::Vec<2, float> mScrollOffset{0.f, 0.f};

    std::array<InputState, static_cast<std::size_t>(MouseButton::_End)> mMouseButtons;
};


/// \brief Handles all the input devices, and produce the `Input` instance that is consumed by the simulation.
class HidManager
{
public:
    class Inhibiter
    {
    public:
        virtual bool isCapturingMouse() const = 0;
        virtual bool isCapturingKeyboard() const = 0;

        virtual ~Inhibiter() = default;

        class Null;

        static const Null gNull;
    };

    HidManager(graphics::ApplicationGlfw & aApplication);

    Input initialInput() const;

    Input read(const Input & aPrevious, const Inhibiter & aInhibiter = Inhibiter::gNull);

private:
    // Note: must be defined within HidManager (not after) because it is used as a default value on read().
    class Inhibiter::Null final : public Inhibiter
    {
        bool isCapturingMouse() const override
        { return false; }
        bool isCapturingKeyboard() const override
        { return false; }
    };

    // TODO Answer: might the callback be called several times between/by glfwPollEvents()?
    void callbackCursorPosition(double xpos, double ypos);
    void callbackMouseButton(int button, int action, int mods, double xpos, double ypos);
    void callbackScroll(double xoffset, double yoffset);

    math::Position<2, float> mCursorPosition;
    // TODO could be made more compact, only need a bit per button
    std::array<bool, static_cast<std::size_t>(MouseButton::_End)> mMouseButtons;
    math::Vec<2, float> mScrollOffset{0.f, 0.f};
};


inline const HidManager::Inhibiter::Null HidManager::Inhibiter::gNull{};


} // namespace snac
} // namespace ad
