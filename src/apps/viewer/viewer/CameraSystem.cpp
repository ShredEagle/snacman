#include "CameraSystem.h"

#include <graphics/ApplicationGlfw.h>


namespace ad::renderer {


namespace {

    constexpr math::Degree<float> gInitialVFov{50.f};
    constexpr float gInitialRadius{2.f};
    constexpr float gNearZ{-0.1f};
    constexpr float gMinFarZ{-25.f};

    // GUI related values
    constexpr float gMinZ{-0.01f};
    constexpr float gMaxZ{-1000.f};
    constexpr float gMinViewHeight{0.01f};
    constexpr float gMaxViewHeight{1000.f};

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
        // Camera control
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

        // Projection
        {
            auto projectionParams = aCameraSystem.mCamera.getProjectionParameters();

            // local semantic only: 0 Orthographic, 1 Perspective
            enum Projection
            {
                Orthographic = 0,
                Perspective = 1,
            };
            int projectionId = std::holds_alternative<graphics::OrthographicParameters>(projectionParams) ?
                Orthographic : Perspective;

            bool changed = ImGui::RadioButton("Orthographic",
                                              &projectionId,
                                              Orthographic);
            ImGui::SameLine();
            changed |= ImGui::RadioButton("Perspective",
                                          &projectionId,
                                          Perspective);

            // If a change occured, convert from one projection parameter type to the other
            if(changed)
            {
                // Even if this is not the active control mode, we use orbital to determine the plane distance
                const float radius = aCameraSystem.mOrbitalControl.mOrbital.mSpherical.radius();

                switch(projectionId)
                {
                    case Orthographic:
                    {
                        // Was perspective
                        graphics::PerspectiveParameters perspective =
                            std::get<graphics::PerspectiveParameters>(projectionParams);
                        projectionParams = toOrthographic(
                                perspective,
                                graphics::computePlaneHeightt(radius, perspective.mVerticalFov));
                        break;
                    }
                    case Perspective:
                    {
                        // Was orthograhic
                        graphics::OrthographicParameters ortho = 
                            std::get<graphics::OrthographicParameters>(projectionParams);

                        projectionParams = toPerspective(
                            ortho,
                            graphics::computeVerticalFov(radius, ortho.mViewHeight));
                        break;
                    }
                }
            }

            // Allow to edit the parameter values
            // Note that using the same boolean to detect change ensure the camera projection will be
            // setup whether the projection type or just parameter values changed.
            std::visit([& changed, & camera = aCameraSystem.mCamera](auto && params)
            {
                using T = std::decay_t<decltype(params)>;
                if constexpr(std::is_same_v<T, graphics::OrthographicParameters>)
                {
                    ImGui::Text("Aspect ratio: %f", params.mAspectRatio);
                    // Note: Cannot put it in a single instruction 
                    // because shortcut evaluation would stop drawing subsequent widgets
                    changed |= ImGui::InputFloat("View height", &params.mViewHeight, 0.1f, 1.f, "%.2f");
                    changed |= ImGui::InputFloat("Near Z", &params.mNearZ, 0.01f, .5f, "%.2f");
                    changed |= ImGui::InputFloat("Far Z", &params.mFarZ, 0.01f, .5f, "%.2f");
                    if(changed)
                    {
                        params.mViewHeight = std::clamp(params.mViewHeight, gMinViewHeight, gMaxViewHeight); 
                        params.mNearZ = std::clamp(params.mNearZ, gMaxZ - gMinZ, gMinZ); 
                        params.mFarZ = std::clamp(params.mFarZ, gMaxZ, params.mNearZ + gMinZ); 
                        camera.setupOrthographicProjection(params);
                    }
                }
                else if constexpr(std::is_same_v<T, graphics::PerspectiveParameters>)
                {
                    ImGui::Text("Aspect ratio: %f", params.mAspectRatio);
                    changed |= ImGui::SliderAngle("Vertical FOV", &params.mVerticalFov.data(), 1.f, 180.f);
                    changed |= ImGui::InputFloat("Near Z", &params.mNearZ, 0.01f, .5f, "%.2f");
                    changed |= ImGui::InputFloat("Far Z", &params.mFarZ, 0.01f, .5f, "%.2f");
                    if(changed)
                    {
                        params.mNearZ = std::clamp(params.mNearZ, gMaxZ - gMinZ, gMinZ); 
                        params.mFarZ = std::clamp(params.mFarZ, gMaxZ, params.mNearZ + gMinZ); 
                        camera.setupPerspectiveProjection(params);
                    }
                }
            }, projectionParams);
        }

    }

}


} // namespace ad::renderer