#include "Camera.h"

#include <graphics/CameraUtilities.h>

#include <renderer/BufferLoad.h>

#include <math/Angle.h>

// TODO Address this dirty include, currently needed to get the GLFW input definitions
#include <GLFW/glfw3.h>

#include <span>


namespace ad {
namespace snac {


math::Matrix<4, 4, float> makePerspectiveProjection(math::Radian<float> aVerticalFov,
                                                    float aAspectRatio,
                                                    float aZNear,
                                                    float aZFar)
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


Camera::Camera(float aAspectRatio, Camera::Parameters aParameters)
{
    setPerspectiveProjection(aAspectRatio, aParameters);
}


void Camera::setPerspectiveProjection(float aAspectRatio, Parameters aParameters)
{
    mProjection = 
        makePerspectiveProjection(aParameters.vFov,
                                  aAspectRatio,
                                  aParameters.zNear,
                                  aParameters.zFar);
}


CameraBuffer::CameraBuffer(float aAspectRatio, Camera::Parameters aParameters) :
    mCurrentParameters{aParameters}
{
    const std::array<math::Matrix<4, 4, float>, 2> identities{
        math::Matrix<4, 4, float>::Identity(),
        makePerspectiveProjection(aParameters.vFov, aAspectRatio, aParameters.zNear, aParameters.zFar)
    };

    load(mViewing, std::span{identities}, graphics::BufferHint::DynamicDraw);
}


void CameraBuffer::resetProjection(float aAspectRatio, Camera::Parameters aParameters)
{
    auto perspectiveProjection = 
        makePerspectiveProjection(aParameters.vFov, aAspectRatio, aParameters.zNear, aParameters.zFar);
    graphics::ScopedBind bound{mViewing};
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(math::Matrix<4, 4, float>), sizeof(perspectiveProjection),
                    perspectiveProjection.data());
    mCurrentParameters = aParameters;
}


void CameraBuffer::setWorldToCamera(const math::AffineMatrix<4, GLfloat> & aTransformation)
{
    graphics::ScopedBind bound{mViewing};
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(aTransformation),
                    aTransformation.data());
}


void Orbital::incrementOrbit(math::Radian<float> aAzimuthal, math::Radian<float> aPolar)
{
    using Radian = math::Radian<float>;
    mSpherical.polar() += aPolar;
    mSpherical.azimuthal() += aAzimuthal;
    mSpherical.polar() = std::max(Radian{0},
                                  std::min(Radian{math::pi<float>},
                                           mSpherical.polar()));
}


void Orbital::pan(math::Vec<2, float> aPanning)
{
    math::OrthonormalBase<3, float> tangent = mSpherical.computeTangentFrame().base;
    mSphericalOrigin -= aPanning.x() * tangent.u().normalize() - aPanning.y() * tangent.v();
}


float & Orbital::radius()
{
    return mSpherical.radius();
}


math::AffineMatrix<4, float> Orbital::getParentToLocal() const
{
    const math::Frame<3, float> tangentFrame = mSpherical.computeTangentFrame();

    return math::trans3d::canonicalToFrame(math::Frame<3, float>{
        mSphericalOrigin + tangentFrame.origin.as<math::Vec>(),
        tangentFrame.base
    });
}


void MouseOrbitalControl::callbackCursorPosition(double xpos, double ypos)
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
            mOrbital.incrementOrbit(Radian{-angularIncrements.x()}, Radian{-angularIncrements.y()});
            break;
        }
        case ControlMode::Pan:
        {
            auto dragVector{cursorPosition - mPreviousDragPosition};

            // View height in the plane of the polar origin (plane perpendicular to the view vector)
            // (the view height depends on the "plane distance" in the perspective case).
            float viewHeight_world = 2 * tan(mVerticalFov / 2) * std::abs(mOrbital.radius());
            dragVector *= viewHeight_world / mWindowSize.height();

            mOrbital.pan(dragVector);
            break;
        }
        default:
        {
            // pass
        }
    }

    mPreviousDragPosition = cursorPosition;
}


void MouseOrbitalControl::callbackMouseButton(int button, int action, int mods, double xpos, double ypos)
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


void MouseOrbitalControl::callbackScroll(double xoffset, double yoffset)
{
    float factor = (1 - (float)yoffset * gScrollFactor);
    mOrbital.radius() *= factor;
}

} // namespace snac
} // namespace ad
