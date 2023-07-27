#include "Camera.h"

#include <math/Transformations.h>

#include <graphics/CameraUtilities.h>

// TODO Address this dirty include, currently needed to get the GLFW input definitions
#include <GLFW/glfw3.h>


namespace ad::renderer {

////////
// Camera
////////

math::Matrix<4, 4, float> Camera::MakeProjection(OrthographicParameters aParams)
{
    assert(aParams.mNearZ > aParams.mFarZ);

    math::Box<GLfloat> viewVolume = 
        graphics::getViewVolumeRightHanded(aParams.mAspectRatio,
                                           aParams.mViewHeight,
                                           aParams.mNearZ,
                                           aParams.mNearZ - aParams.mFarZ /* both should be negative,
                                                                             resulting in a positive */
                                          );

    return
        math::trans3d::orthographicProjection(viewVolume)
        * math::trans3d::scale(1.f, 1.f, -1.f) // camera space is right handed, but gl clip space is left handed.
        ;
}


void Camera::setupOrthographicProjection(OrthographicParameters aParams)
{
    mProjection = MakeProjection(aParams);
    mProjectionParameters = aParams;
}


void changeOrthographicViewportHeight(Camera & aCamera, float aNewHeight)
{
    Camera::OrthographicParameters orthoParams = std::visit(
        [aNewHeight](auto aParams) -> Camera::OrthographicParameters
        {
            using T_param = std::decay_t<decltype(aParams)>;
            if constexpr(std::is_same_v<T_param, Camera::OrthographicParameters>)
            {
                aParams.mViewHeight = aNewHeight;
                return aParams;
            }
            else
            {
                // TODO
                throw std::logic_error{"Not implemented yet."};
            }
        },
        aCamera.getProjectionParameters());

    aCamera.setupOrthographicProjection(orthoParams);
}


////////
// Orbital
////////

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


float Orbital::radius() const
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


////////
// OrbitalControl
////////

float OrbitalControl::getViewHeightAtOrbitalCenter() const
{

    // View height in the plane of the polar origin (plane perpendicular to the view vector)
    // (the view height depends on the "plane distance" in the perspective case).
    return 2 * tan(mVerticalFov / 2) * std::abs(mOrbital.radius());
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
            mOrbital.incrementOrbit(Radian{-angularIncrements.x()}, Radian{-angularIncrements.y()});
            break;
        }
        case ControlMode::Pan:
        {
            auto dragVector{cursorPosition - mPreviousDragPosition};

            // TODO handle both perspective and ortho case here
            // View height in the plane of the polar origin (plane perpendicular to the view vector)
            // (the view height depends on the "plane distance" in the perspective case).
            //float viewHeight_world = 2 * tan(mVerticalFov / 2) * std::abs(mOrbital.radius());
            dragVector *=  getViewHeightAtOrbitalCenter() / mWindowSize.height();

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
    mOrbital.radius() *= factor;
}

} // namespace ad::renderer