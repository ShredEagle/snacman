#include "CameraSystem.h"

#include <graphics/ApplicationGlfw.h>


namespace ad::renderer {


namespace {

    constexpr math::Degree<float> gInitialVFov{50.f};
    constexpr float gInitialRadius{2.f};
    constexpr float gNearZ{-0.1f};
    constexpr float gMinFarZ{-25.f};

} // unnamed namespace


/// @brief The orbital camera is moved away by the largest model dimension times this factor.
[[maybe_unused]] constexpr float gRadialDistancingFactor{1.5f};
/// @brief The far plane will be at least at this factor times the model depth.
constexpr float gDepthFactor{20.f};


CameraSystem::CameraSystem(std::shared_ptr<graphics::AppInterface> aAppInterface,
                           const imguiui::ImguiUi * aImguiUi,
                           Control aMode) :
    mAppInterface{std::move(aAppInterface)},
    mImguiUi{aImguiUi},
    mActive{aMode},
    mOrbitalControl{mAppInterface->getWindowSize(),
                    gInitialVFov,
                    Orbital{gInitialRadius}}
{
        setControlMode(mActive);

        // TODO #camera do the projection setup only on each projection change
        // Ideally, the far plane would be proportional to model depth
        mCamera.setupOrthographicProjection({
            .mAspectRatio = math::getRatio<GLfloat>(mAppInterface->getWindowSize()),
            .mViewHeight = mOrbitalControl.getViewHeightAtOrbitalCenter(),
            .mNearZ = gNearZ,
            .mFarZ = gMinFarZ,
        });

        mCamera.setupPerspectiveProjection({
            .mAspectRatio = math::getRatio<GLfloat>(mAppInterface->getWindowSize()),
            .mVerticalFov = gInitialVFov,
            .mNearZ = gNearZ,
            // Note: min (not max), because those values are negative.
            .mFarZ = gMinFarZ, 
        });
}


void CameraSystem::setupAsModelViewer(math::Box<float> aModelAabb)
{
    mOrbitalControl.mOrbital.mSphericalOrigin = aModelAabb.center();

    // Move the orbital camera away, depending on the model size
    mOrbitalControl.mOrbital.mSpherical.radius() = 
        std::max(mOrbitalControl.mOrbital.mSpherical.radius(),
                 gRadialDistancingFactor * (*aModelAabb.mDimension.getMaxMagnitudeElement()));

    mCamera.setupPerspectiveProjection({
        .mAspectRatio = math::getRatio<GLfloat>(mAppInterface->getWindowSize()),
        .mVerticalFov = gInitialVFov,
        .mNearZ = gNearZ,
        // Note: min (not max), because those values are negative.
        .mFarZ = std::min(gMinFarZ, 
                          -gDepthFactor * (*aModelAabb.mDimension.getMaxMagnitudeElement())),
    });

    setControlMode(CameraSystem::Control::Orbital);
}


void CameraSystem::setControlMode(Control aMode)
{
    if(aMode == Control::Orbital)
    {
        if(mActive == Control::FirstPerson)
        {}
        graphics::registerGlfwCallbacks(*mAppInterface, mOrbitalControl, graphics::EscKeyBehaviour::Close, mImguiUi);
    }
    else if(aMode == Control::FirstPerson)
    {
        if(mActive == Control::Orbital)
        {}
        graphics::registerGlfwCallbacks(*mAppInterface, mFirstPersonControl, graphics::EscKeyBehaviour::Close, mImguiUi);
    }
    mActive = aMode;
}


void CameraSystem::update(float aDeltaTime)
{
    if(mActive == Control::FirstPerson)
    {
        mFirstPersonControl.update(aDeltaTime);
    }

    // TODO #camera: handle this when necessary
    // Update camera to match current values in orbital control.
    //mCamera.setPose(mOribtalControl.mOrbital.getParentToLocal());
    //if(mCamera.isProjectionOrthographic())
    //{
    //    // Note: to allow "zooming" in the orthographic case, we change the viewed height of the ortho projection.
    //    // An alternative would be to apply a scale factor to the camera Pose transformation.
    //    changeOrthographicViewportHeight(mCamera, mOribtalControl.getViewHeightAtOrbitalCenter());
    //}
    
    switch(mActive)
    {
        case Control::Orbital:
        {
            mCamera.setPose(mOrbitalControl.mOrbital.getParentToLocal());
            break;
        }
        case Control::FirstPerson:
        {
            mCamera.setPose(mFirstPersonControl.mPose.getParentToLocal());
            break;
        }
    }
}


void CameraSystemGui::presentSection(CameraSystem & aCameraSystem)
{
    if (ImGui::CollapsingHeader("Camera"))
    {
        int mode = static_cast<int>(aCameraSystem.mActive);
        bool changed = ImGui::RadioButton("First-person",
                                          &mode,
                                          static_cast<int>(CameraSystem::Control::FirstPerson));
        ImGui::SameLine();
        changed |= ImGui::RadioButton("Orbital", 
                                      &mode,
                                      static_cast<int>(CameraSystem::Control::Orbital));

        if(changed)
        {
            aCameraSystem.setControlMode(static_cast<CameraSystem::Control>(mode));
        }
    }

}


} // namespace ad::renderer