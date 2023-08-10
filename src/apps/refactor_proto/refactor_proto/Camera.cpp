#include "Camera.h"

#include <math/Transformations.h>

#include <graphics/CameraUtilities.h>

// TODO Address this dirty include, currently needed to get the GLFW input definitions
#include <GLFW/glfw3.h>


namespace ad::renderer {


////////
// Camera
////////

// TODO factorize those projection matrix functions
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


math::Matrix<4, 4, float> Camera::MakeProjection(PerspectiveParameters aParams)
{
    return 
        math::trans3d::perspective(aParams.mNearZ, aParams.mFarZ)
        * MakeProjection(OrthographicParameters{
                .mAspectRatio = aParams.mAspectRatio,
                .mViewHeight = 2 * tan(aParams.mVerticalFov / 2.f) * std::abs(aParams.mNearZ),
                .mNearZ = aParams.mNearZ,
                .mFarZ = aParams.mFarZ,
            })
    ;
}


void Camera::setupOrthographicProjection(OrthographicParameters aParams)
{
    mProjection = MakeProjection(aParams);
    mProjectionParameters = aParams;
}


void Camera::setupPerspectiveProjection(PerspectiveParameters aParams)
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
    // top-left corner origin
    math::Position<2, float> cursorPosition{(float)xpos, (float)ypos};

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


//
// FirstPersonControl
//

void FirstPersonControl::callbackMouseButton(int button, int action, int mods, double xpos, double ypos)
{
    if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT)
    {
        mControlMode = ControlMode::Aiming;
    }
    else if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT)
    {
        mControlMode = ControlMode::None;
    }
}


void FirstPersonControl::callbackCursorPosition(double xpos, double ypos)
{
    // top-left corner origin (i.e. +Y is going down on the window)
    math::Position<2, float> cursorPosition{(float)xpos, (float)ypos};

    if(mControlMode == ControlMode::Aiming)
    {
        auto angularIncrements = (cursorPosition - mPreviousCursorPosition).cwMul(gMouseControlFactor);
        mOrientation.roll.data() -= angularIncrements.y(); // mouse down -> +Y -> negative roation around X (to look down)
        mOrientation.pitch.data() -= angularIncrements.x(); // mouse right -> +X -> negative rotation around Y (to look right)

        // First clamp the pitch, then reduce
        static constexpr float quarterRevolution = math::Turn<float>{1.f/4.f}.as<math::Radian>().value();
        auto & roll = mOrientation.roll.data();
        roll = std::clamp(roll, -quarterRevolution, quarterRevolution);

        mOrientation = reduce(mOrientation);
    }

    mPreviousCursorPosition = cursorPosition;
}


void FirstPersonControl::callbackScroll(double xoffset, double yoffset)
{
    //TODO handle assigning only the callbacks that are present get rid of this empty
}


void FirstPersonControl::callbackKeyboard(int key, int scancode, int action, int mods)
{
    if(action != GLFW_PRESS && action != GLFW_RELEASE)
    {
        return;
    }

    switch(key)
    {
        case GLFW_KEY_I:
            mF = (action == GLFW_PRESS);
            break;
        case GLFW_KEY_K:
            mB = (action == GLFW_PRESS);
            break;
        case GLFW_KEY_J:
            mL = (action == GLFW_PRESS);
            break;
        case GLFW_KEY_L:
            mR = (action == GLFW_PRESS);
            break;
    }
}


/// @brief Rotation matrix applying rotations to match the EulerAngles orientation,
/// i.e. a change of basis from local to parent.
math::LinearMatrix<3, 3, float> dummyToRotationMatrix(math::EulerAngles<float> aEuler)
{
    return math::trans3d::rotateX(aEuler.roll)
        * math::trans3d::rotateY(aEuler.pitch)
        * math::trans3d::rotateZ(aEuler.yaw);
}

math::LinearMatrix<3, 3, float> dummyToRotationMatrixInverse(math::EulerAngles<float> aEuler)
{
    //return math::trans3d::rotateX(-aEuler.roll)
    //    * math::trans3d::rotateY(-aEuler.pitch)
    //    * math::trans3d::rotateZ(-aEuler.yaw);
    // TODO understand why this order, when I expected the opposite
    // I guess this is because the other order is for local to parent
    return math::trans3d::rotateZ(-aEuler.yaw)
        * math::trans3d::rotateY(-aEuler.pitch)
        * math::trans3d::rotateX(-aEuler.roll);
}
void FirstPersonControl::update(float aDeltaTime)
{
    float movement = aDeltaTime * gSpeed;
    // This is the rotation matrix in parent frame:
    // its rows are the base vectors of the camera frame, expressed in canonical coordinates.
    math::LinearMatrix<3, 3, float> rotation = dummyToRotationMatrix(mOrientation);
    // TODO I expected both toe be equivalent, but the quaternion form causes issue with the "backward"
    // vector when there is enough roll
    //math::LinearMatrix<3, 3, float> rotation = toQuaternion(mOrientation).toRotationMatrix();
    math::Vec<3, float> backward{rotation[2]}; // camera looks in -Z, so +Z is the backward vector
    math::Vec<3, float> right{rotation[0]};
    if(mF) mPosition -= backward * movement;
    if(mB) mPosition += backward * movement;
    if(mL) mPosition -= right * movement;
    if(mR) mPosition += right * movement;
}


math::AffineMatrix<4, float> FirstPersonControl::getParentToLocal() const
{
    // Translate the world by negated camera position, then rotate by inverse camera orientation
    return math::trans3d::translate(-mPosition.as<math::Vec>()) 
        * dummyToRotationMatrixInverse(mOrientation);
        // TODO investigate why the quaternion path is clipping roll to 3/4 turn instead of 1/4.
        //* math::toQuaternion(mOrientation).inverse().toRotationMatrix();
}

} // namespace ad::renderer