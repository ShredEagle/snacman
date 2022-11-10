#include "Camera.h"

#include <graphics/CameraUtilities.h>

#include <renderer/BufferUtilities.h>

#include <math/Angle.h>

// TODO Address this dirty include, currently needed to get the GLFW input definitions
#include <GLFW/glfw3.h>

#include <span>


namespace ad {
namespace snac {

constexpr float gZNear = -0.1f;
constexpr float gZFar = -500.f;
constexpr math::Degree<float> gVFov{45.f};
//// I am about 50cm away from screen, a 600px high window is ~14cm on it.
////const math::Radian<float> gVFov{2 * std::atan2(140.f/2, 500.f)};


math::Matrix<4, 4, float> makePerspectiveProjection(math::Radian<float> aVerticalFov, float aAspectRatio,
                                                    float aZNear, float aZFar)
{
    const float nearHeight = 2 * -aZNear * tan(aVerticalFov / 2);
    math::Size<2, float> nearPlaneSize = math::makeSizeFromHeight(nearHeight, aAspectRatio);

    return math::trans3d::perspective(aZNear, aZFar)
           * 
           (math::trans3d::orthographicProjection(math::Box<float>{
               {-nearPlaneSize.as<math::Position>() / 2, aZFar},
               {nearPlaneSize, aZNear - aZFar}
           })
           * 
           math::trans3d::scale(1.f, 1.f, -1.f)) // OpenGL clipping space is left handed.
           ;
}
           
Camera::Camera(float aAspectRatio)
{
    const std::array<math::Matrix<4, 4, float>, 2> identities{
        math::Matrix<4, 4, float>::Identity(),
        makePerspectiveProjection(gVFov, aAspectRatio, gZNear, gZFar)
    };

    load(mViewing, std::span{identities}, graphics::BufferHint::DynamicDraw);
}


void Camera::setAspectRatio(float aAspectRatio)
{
    graphics::ScopedBind bound{mViewing};
    auto perspectiveProjection = makePerspectiveProjection(gVFov, aAspectRatio, gZNear, gZFar);
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(math::Matrix<4, 4, float>), sizeof(perspectiveProjection),
                    perspectiveProjection.data());
}


void Camera::setWorldToCamera(const math::AffineMatrix<4, GLfloat> & aTransformation)
{
    graphics::ScopedBind bound{mViewing};
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(aTransformation),
                    aTransformation.data());
}


math::AffineMatrix<4, float> OrbitalControl::getParentToLocal() const
{
    const math::Frame<3, float> tangentFrame = mSpherical.computeTangentFrame();

    return math::trans3d::canonicalToFrame(math::Frame<3, float>{
        mSphericalOrigin + tangentFrame.origin.as<math::Vec>(),
        tangentFrame.base
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
            auto dragVector{cursorPosition - mPreviousDragPosition};

            // View height in the plane of the polar origin (plane perpendicular to the view vector)
            // (the view height depends on the "plane distance" in the perspective case).
            float viewHeight = 2 * tan(gVFov / 2) * std::abs(mSpherical.radius());
            dragVector *= viewHeight / mWindowSize.height();

            math::OrthonormalBase<3, float> tangent = mSpherical.computeTangentFrame().base;
            mSphericalOrigin -= dragVector.x() * tangent.u().normalize() - dragVector.y() * tangent.v();
            break;
        }
        default:
        {
            // pass
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
