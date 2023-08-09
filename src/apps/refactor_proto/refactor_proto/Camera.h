#pragma once


#include <math/Angle.h>
#include <math/Homogeneous.h>
#include <math/Spherical.h>
#include <math/Vector.h>

#include <variant>


namespace ad::renderer {


/// @brief Models a camera in the world, with a spatial-pose and a projection.
/// It can be used to generate eye transformation and projection transformation matrices.
class Camera
{
public:
    //math::Matrix<4, 4, GLfloat> assembleViewMatrix() const
    //{ return mWorldToCamera * mProjection; }

    void setPose(math::AffineMatrix<4, float> aParentToCamera)
    { mParentToCamera = aParentToCamera; }

    struct OrthographicParameters
    {
        float mAspectRatio;
        float mViewHeight;
        float mNearZ; // usually negative: Z coordinate of the near plane in the right handed coordinate system.
        float mFarZ;  // usually negative: Z coordinate of the far plane in the right handed coordinate system.
    };

    // TODO Decide whether it should be lifted into a more general library?
    static math::Matrix<4, 4, float> MakeProjection(OrthographicParameters aParams);

    void setupOrthographicProjection(OrthographicParameters aParams);

    struct PerspectiveParameters
    {};

    const math::AffineMatrix<4, float> & getParentToCamera() const
    { return mParentToCamera; }

    const math::Matrix<4, 4, float> & getProjection() const
    { return mProjection; }

    const std::variant<OrthographicParameters, PerspectiveParameters> & getProjectionParameters() const
    { return mProjectionParameters; }

    bool isProjectionOrthographic() const
    { return std::holds_alternative<OrthographicParameters>(mProjectionParameters); }

private:
    math::AffineMatrix<4, float> mParentToCamera = math::AffineMatrix<4, float>::Identity(); 
    math::Matrix<4, 4, float> mProjection = MakeProjection(OrthographicParameters{
            .mAspectRatio = 1,
            .mViewHeight = 1,
            .mNearZ = +1,
            .mFarZ = -1,
        }); 

    std::variant<OrthographicParameters, PerspectiveParameters> mProjectionParameters;
};


/// @brief Helper method to set `aCamera` projection as orthographic with viewed height `aNewHeight`.
/// @param aCamera 
/// @param aNewHeight viewport height in world coordinates.
void changeOrthographicViewportHeight(Camera & aCamera, float aNewHeight);


// TODO factorize, this is more general than purely camera movements.

/// @brief `Orbital` represent a position on a spherical orbit around an origin.
/// This can be used to represent camera poses, with the camera placed at the `Orbital` position,
/// and looking toward the orbit origin.
struct Orbital
{
    Orbital(float aRadius, math::Radian<float> aPolar = math::Degree<float>{90.f},
            math::Radian<float> aAzimuthal = math::Radian<float>{0.f},
            math::Position<3, float> aPosition = {0.f, 0.f, 0.f}) :
        mSpherical{aRadius, aPolar, aAzimuthal}
    {}

    /// \warning aAzimuthal first, aPolar second (matches the usual convention for x and y)
    void incrementOrbit(math::Radian<float> aAzimuthal, math::Radian<float> aPolar);
    void incrementOrbitRadians(math::Vec<2, float> aIncrements)
    { incrementOrbit(math::Radian<float>{aIncrements.x()}, math::Radian<float>{aIncrements.y()}); }

    void pan(math::Vec<2, float> aPanning);

    float & radius();
    float radius() const;

    math::AffineMatrix<4, float> getParentToLocal() const;

    math::Position<3, float> mSphericalOrigin;
    math::Spherical<float> mSpherical;
};


//TODO Ad 2023/07/27: 
// This should be abstracted away from being used purely for rendering cameras, removing
// knowledge of window size and vertical FOV. Yet this cause au complication for _panning_ movements.

/// @brief Controls an Orbital position with mouse movements (movements of an usual orbital camera).
struct OrbitalControl
{
    template <class T_unitTag>
    OrbitalControl(math::Size<2, int> aWindowSize,
                   math::Angle<float, T_unitTag> aVerticalFov,
                   Orbital aInitialPose) :
        mOrbital{aInitialPose},
        mWindowSize{aWindowSize},
        mVerticalFov{aVerticalFov}
    {}

    // Glfw compatible callbacks
    void callbackMouseButton(int button, int action, int mods, double xpos, double ypos);
    void callbackCursorPosition(double xpos, double ypos);
    void callbackScroll(double xoffset, double yoffset);

    /// \brief Return the view height in world coordinates, for a view plan place at the Orbital origin.
    /// Computation is based on the currently set Fov and Oribtal radius, even if the actual projection
    /// in use is orthographic. Thus, it can be used to transit from perspective to orthographic while
    /// conserving the apparent size of object at orbital center.
    float getViewHeightAtOrbitalCenter() const;

    template <class T_unitTag>
    void setVerticalFov(math::Angle<float, T_unitTag> aFov)
    {
        mVerticalFov(aFov);
    }

    Orbital mOrbital;

private:
    enum class ControlMode
    {
        None,
        Orbit,
        Pan,
    };

    static constexpr math::Vec<2, float> gMouseControlFactor{1/700.f, 1/700.f};
    static constexpr float gScrollFactor = 0.05f;

    math::Size<2, int> mWindowSize;
    math::Radian<float> mVerticalFov;

    ControlMode mControlMode{ControlMode::None};
    math::Position<2, float> mPreviousDragPosition{0.f, 0.f};
};

} // namespace ad::renderer
