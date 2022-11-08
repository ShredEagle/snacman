#pragma once

#include <math/Homogeneous.h>
#include <math/Spherical.h>

#include <renderer/UniformBuffer.h>


namespace ad {
namespace snac {


struct Camera
{
    Camera(float aAspectRatio);

    void setAspectRatio(float aAspectRatio);
    void setWorldToCamera(const math::AffineMatrix<4, GLfloat> & aTransformation);

    graphics::UniformBufferObject mViewing;
};


class OrbitalControl
{
public:
    OrbitalControl(math::Size<2, int> aWindowSize) :
        mWindowSize{aWindowSize}
    {}

    void setWindowSize(math::Size<2, int> aWindowSize)
    { mWindowSize = aWindowSize; }

    void callbackCursorPosition(double xpos, double ypos);
    void callbackMouseButton(int button, int action, int mods, double xpos, double ypos);
    void callbackScroll(double xoffset, double yoffset);

    math::AffineMatrix<4, float> getParentToLocal() const;

private:
    enum class ControlMode
    {
        None,
        Orbit,
        Pan,
    };

    static constexpr math::Vec<2, GLfloat> gMouseControlFactor{1/700.f, 1/700.f};
    static constexpr float gScrollFactor = 0.05f;

    math::Position<3, float> mSphericalOrigin;
    math::Spherical<float> mSpherical{11.f}; // to be out of a 2 unit cube centered on the origin

    math::Size<2, int> mWindowSize;

    ControlMode mControlMode{ControlMode::None};
    math::Position<2, GLfloat> mPreviousDragPosition{0.f, 0.f};
};


} // namespace snac
} // namespace ad
