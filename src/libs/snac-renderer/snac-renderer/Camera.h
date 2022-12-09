#pragma once

#include "math/Angle.h"
#include <math/Homogeneous.h>
#include <math/Spherical.h>

#include <renderer/UniformBuffer.h>


namespace ad {
namespace snac {


// TODO rename to camera buffer
struct Camera
{
    struct Parameters
    {
        math::Radian<float> vFov;
        float zNear = -0.1f;
        float zFar = -500.f;
    };

    Camera(float aAspectRatio, Parameters aParameters);

    void resetProjection(float aAspectRatio, Parameters aParameters);
    void setWorldToCamera(const math::AffineMatrix<4, GLfloat> & aTransformation);

    graphics::UniformBufferObject mViewing;

    static constexpr Parameters gDefaults{
        .vFov = math::Degree<float>{45.f},
        //// I am about 50cm away from screen, a 600px high window is ~14cm on it.
        ///.vFov = math::Radian<float>{2 * std::atan2(140.f/2, 500.f)},
        .zNear = -0.1f,
        .zFar = -500.f,
    };
};


class Orbital
{
public:
    Orbital(float aRadius, math::Radian<float> aPolar = math::Radian<float>{0.f}, math::Radian<float> aAzimuthal = math::Radian<float>{0.f}) :
        mSpherical{aRadius, aPolar, aAzimuthal}
    {}

    /// \warning aAzimuthal first, aPolar second (matches the usual convention for x and y)
    void incrementOrbit(math::Radian<float> aAzimuthal, math::Radian<float> aPolar);
    void incrementOrbitRadians(math::Vec<2, float> aIncrements)
    { incrementOrbit(math::Radian<float>{aIncrements.x()}, math::Radian<float>{aIncrements.y()}); }

    void pan(math::Vec<2, float> aPanning);
    float & radius();

    math::AffineMatrix<4, float> getParentToLocal() const;

private:
    math::Position<3, float> mSphericalOrigin;
    math::Spherical<float> mSpherical;
};


class MouseOrbitalControl
{
public:
    MouseOrbitalControl(math::Size<2, int> aWindowSize, math::Radian<float> aVerticalFov) :
        mWindowSize{aWindowSize},
        mVerticalFov{aVerticalFov}
    {}

    void setWindowSize(math::Size<2, int> aWindowSize)
    { mWindowSize = aWindowSize; }

    void callbackCursorPosition(double xpos, double ypos);
    void callbackMouseButton(int button, int action, int mods, double xpos, double ypos);
    void callbackScroll(double xoffset, double yoffset);

    math::AffineMatrix<4, float> getParentToLocal() const
    { return mOrbital.getParentToLocal(); }

private:
    enum class ControlMode
    {
        None,
        Orbit,
        Pan,
    };

    static constexpr math::Vec<2, GLfloat> gMouseControlFactor{1/700.f, 1/700.f};
    static constexpr float gScrollFactor = 0.05f;

    math::Size<2, int> mWindowSize;
    math::Radian<float> mVerticalFov;

    ControlMode mControlMode{ControlMode::None};
    math::Position<2, GLfloat> mPreviousDragPosition{0.f, 0.f};

    Orbital mOrbital{6.0}; // to be out of a 2 unit cube centered on the origin
};


} // namespace snac
} // namespace ad
