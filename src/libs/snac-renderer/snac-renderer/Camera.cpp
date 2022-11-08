#include "Camera.h"

#include <graphics/CameraUtilities.h>

#include <math/Angle.h>

// TODO Address this dirty include, to get the GLFW input definitions
#include <GLFW/glfw3.h>


namespace ad {
namespace snac {

constexpr float gZNear = -0.1f;
constexpr float gZFar = -500.f;
constexpr math::Degree<float> gHFov{55.f};

Camera::Camera(float aAspectRatio)
{
    const float nearWidth = 2 * -gZNear * tan(gHFov);
    math::Size<2, float> nearPlaneSize = math::makeSizeFromWidth(nearWidth, aAspectRatio);

    math::Matrix<4, 4, float> perspectiveProjection =
        math::trans3d::perspective(gZNear, gZFar)
        * 
        math::trans3d::orthographicProjection(math::Box<float>{
            {-nearPlaneSize.as<math::Position>() / 2, gZFar},
            {nearPlaneSize, gZNear - gZFar}
        });
           
    const std::array<math::Matrix<4, 4, float>, 2> identities{
        math::Matrix<4, 4, float>::Identity(),
        perspectiveProjection,
    };

    load(mViewing, std::span{identities}, graphics::BufferHint::DynamicDraw);
}


void Camera::setAspectRatio(float aAspectRatio)
{

}


void Camera::setWorldToCamera(const math::AffineMatrix<4, GLfloat> & aTransformation)
{
    graphics::ScopedBind bound{mViewing};
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(aTransformation),
                    aTransformation.data());
}


math::AffineMatrix<4, float> OrbitalControl::getParentToLocal() const
{
    const math::Position<3, float> cartesian = mSpherical.toCartesian();
    const math::OrthonormalBase<3, float> tangentBase = mSpherical.computeTangentBase();

    return math::trans3d::canonicalToFrame(math::Frame<3, float>{
        mSphericalOrigin + cartesian.as<math::Vec>(),
        tangentBase
    });
}


void OrbitalControl::callbackCursorPosition(double xpos, double ypos)
{
    using Radian = math::Radian<float>;
    math::Position<2, float> cursorPosition{(float)xpos, (float)ypos};

    // top-left corner origin
    switch (mControlMode)
    {
    case ControlMode::Orbit:
    {
        auto angularIncrements = (cursorPosition - mPreviousDragPosition).cwMul(gMouseControlFactor);

        // The viewed object should turn in the direction of the mouse,
        // so the camera angles are changed in the opposite direction (hence the substractions).
        mSpherical.azimuthal() -= Radian{angularIncrements.x()};
        mSpherical.polar() -= Radian{angularIncrements.y()};
        mSpherical.polar() = std::max(Radian{0}, std::min(Radian{math::pi<float>}, mSpherical.polar()));
        break;
    }
    case ControlMode::Pan:
    {
        //auto dragVector{cursorPosition - mPreviousDragPosition};

        //// View height in the plane of the polar origin (plane perpendicular to the view vector)
        //// (the view height depends on the "plane distance" in the perspective case).
        //float viewHeight = 2 * tan(mVerticalFov / 2) * std::abs(mSpherical.r);
        //dragVector *= viewHeight / mAppInterface->getWindowSize().height();

        //mPolarOrigin -= dragVector.x() * mSpherical.getCCWTangent().normalize() 
        //                - dragVector.y() * mSpherical.getUpTangent().normalize();
        //break;
    }
    }

    mPreviousDragPosition = cursorPosition;
}


void OrbitalControl::callbackMouseButton(int button, int action, int mods, double xpos, double ypos)
{
    if (action == GLFW_PRESS)
    {
        switch (button)
        {
        case GLFW_MOUSE_BUTTON_LEFT:
            mControlMode = ControlMode::Orbit;
            break;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            mControlMode = ControlMode::Pan;
            break;
        }
    }
    else if ((button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_MIDDLE)
             && action == GLFW_RELEASE)
    {
        mControlMode = ControlMode::None;
    }
}


void OrbitalControl::callbackScroll(double xoffset, double yoffset)
{
    float factor = (1 - (float)yoffset * gScrollFactor);
    mSpherical.radius() *= factor;
}

} // namespace snac
} // namespace ad
