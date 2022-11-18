#include "Input.h"

#include <graphics/AppInterface.h>
#include <graphics/ApplicationGlfw.h>

#include <GLFW/glfw3.h>

#include <functional>


namespace ad {
namespace snac {


namespace {

    void handleButtonEdges(InputState & aInputState, bool aCurrentlyPressed)
    {
        if(aInputState && aCurrentlyPressed)
        {
            aInputState.mState = ButtonStatus::Pressed;
        }
        else if(aInputState && ! aCurrentlyPressed)
        {
            aInputState.mState = ButtonStatus::NegativeEdge;
        }
        else if(! aInputState && aCurrentlyPressed)
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
    std::shared_ptr<graphics::AppInterface> appInterface = aApplication.getAppInterface();

    appInterface->registerCursorPositionCallback(
        std::bind(&HidManager::callbackCursorPosition, this, _1, _2));

    appInterface->registerMouseButtonCallback(
        std::bind(&HidManager::callbackMouseButton, this, _1, _2, _3, _4, _5));

    appInterface->registerScrollCallback(
        std::bind(&HidManager::callbackScroll, this, _1, _2));

    double x, y;
    glfwGetCursorPos(aApplication.getGlfwWindow(), &x, &y);
    callbackCursorPosition(x, y);  

    for(bool & mouseButton : mMouseButtons)
    {
        mouseButton = false;
    };
}


Input HidManager::initialInput() const
{
    return Input{
        .mCursorPosition = mCursorPosition,
    };
}


Input HidManager::read(const Input & aPrevious, const Inhibiter & aInhibiter)
{
    Input result{
        .mCursorPosition = mCursorPosition,
        .mCursorDisplacement = mCursorPosition - aPrevious.mCursorPosition,
    };

    if (!aInhibiter.isCapturingMouse())
    {
        for (auto id = 0; id != mMouseButtons.size(); ++id)
        {
            handleButtonEdges(result.mMouseButtons[id], mMouseButtons[id]);
        }

        result.mScrollOffset = mScrollOffset;
    }

    mScrollOffset.setZero();

    return result;
}


void HidManager::callbackCursorPosition(double xpos, double ypos)
{
    mCursorPosition = {static_cast<float>(xpos), static_cast<float>(ypos)};
}


void HidManager::callbackMouseButton(int button, int action, int mods, double xpos, double ypos)
{
    //constexpr std::array<int, static_cast<std::size_t>(MouseButton::_End)> gMapping
    //{
    //    GLFW_MOUSE_BUTTON_LEFT,
    //    GLFW_MOUSE_BUTTON_MIDDLE,
    //    GLFW_MOUSE_BUTTON_RIGHT,
    //};

    MouseButton mouseButton = [&button]()
    {
        switch(button)
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
        mMouseButtons[static_cast<ButtonEnum_t>(mouseButton)] = (action != GLFW_RELEASE);
    }
}


void HidManager::callbackScroll(double xoffset, double yoffset)
{
    // In case the callbacks can be called several times...
    mScrollOffset += math::Vec<2, float>{static_cast<float>(xoffset), static_cast<float>(yoffset)};
}

} // namespace snac
} // namespace ad
