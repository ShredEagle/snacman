#include "CameraSystem.h"

#include <graphics/ApplicationGlfw.h>


namespace ad::renderer {


namespace {

    constexpr math::Degree<float> gInitialVFov{50.f};
    constexpr float gNearZ{-0.1f};
    constexpr float gMinFarZ{-20.f};

    // GUI related values
    constexpr float gMinZ{-0.01f};
    constexpr float gMaxZ{-1000.f};
    constexpr float gMinViewHeight{0.01f};
    constexpr float gMaxViewHeight{1000.f};

    constexpr graphics::EscKeyBehaviour gEscBehaviour =
        graphics::EscKeyBehaviour::Ignore;

} // unnamed namespace


/// @brief The orbital camera is moved away by the largest model dimension times this factor.
[[maybe_unused]] constexpr float gRadialDistancingFactor{1.5f};
/// @brief The far plane will be at least at this factor times the model depth.
constexpr float gDepthFactor{20.f};


CameraSystem::CameraSystem(std::shared_ptr<graphics::AppInterface> aAppInterface,
                           const imguiui::ImguiUi * aImguiUi,
                           Control aMode,
                           Orbital aInitialOrbitalPose) :
    mAppInterface{std::move(aAppInterface)},
    mImguiUi{aImguiUi},
    mActive{aMode},
    mOrbitalHome{aInitialOrbitalPose},
    mOrbitalControl{mOrbitalHome},
    mFramebufferSizeListener{mAppInterface->listenFramebufferResize(
        std::bind(&CameraSystem::onFramebufferResize, this, std::placeholders::_1))
    },
    mPreviousRadius{aInitialOrbitalPose.mSpherical.radius()}
{
        setControlMode(mActive);

        // TODO Ideally, the far plane would be proportional to model depth
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
    mOrbitalHome.mSphericalOrigin = aModelAabb.center();

    // Move the orbital camera away, depending on the model size
    mOrbitalHome.mSpherical.radius() = 
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
    resetCamera();
}


void CameraSystem::setControlMode(Control aMode)
{
    if(aMode == Control::Orbital)
    {
        if(mActive == Control::FirstPerson)
        {/* TODO conversion */}
        graphics::registerGlfwCallbacks(*mAppInterface, mOrbitalControl, gEscBehaviour, mImguiUi);
    }
    else if(aMode == Control::FirstPerson)
    {
        if(mActive == Control::Orbital)
        {
            /* TODO conversion */
            // In case we would otherwise miss a change.
            // This kind of logic confirms that the member is a hack.
            mPreviousRadius = mOrbitalControl.mOrbital.radius();
        }
        graphics::registerGlfwCallbacks(*mAppInterface, mFirstPersonControl, gEscBehaviour, mImguiUi);
    }
    mActive = aMode;
}


void CameraSystem::resetCamera()
{
    switch(mActive)
    {
        case Control::Orbital:
        {
            mOrbitalControl.mOrbital = mOrbitalHome;
            break;
        }
        case Control::FirstPerson:
        {
            mFirstPersonControl.mPose = {};
            break;
        }
    }
}


void CameraSystem::update(float aDeltaTime)
{
    switch(mActive)
    {
        case Control::Orbital:
        {
            mOrbitalControl.update(getViewHeight(), mAppInterface->getWindowSize().height());
            float currentRadius = mOrbitalControl.mOrbital.radius();
            // If the radius changed and the projection is orthographic,
            // the projection view height should be changed to implement a zoom effect.
            if(mCamera.isProjectionOrthographic()
                 && mPreviousRadius != currentRadius)
            {
                auto orthographic = 
                    std::get<graphics::OrthographicParameters>(mCamera.getProjectionParameters());

                // The previous radius allows us to recompute the original FOV of the equivalent perspective projection
                auto verticalFov = graphics::computeVerticalFov(mPreviousRadius, orthographic.mViewHeight);
                changeOrthographicViewportHeight(mCamera,
                                                 graphics::computePlaneHeight(currentRadius, verticalFov));
            }
            mCamera.setPose(mOrbitalControl.mOrbital.getParentToLocal());
            mPreviousRadius = currentRadius;
            break;
        }
        case Control::FirstPerson:
        {
            mFirstPersonControl.update(aDeltaTime);
            mCamera.setPose(mFirstPersonControl.mPose.getParentToLocal());
            break;
        }
    }
}


float CameraSystem::getViewHeight() const
{
    return std::visit([& orbital = mOrbitalControl.mOrbital](auto && params)
    {
        using T = std::decay_t<decltype(params)>;
        if constexpr(std::is_same_v<T, graphics::OrthographicParameters>)
        {
            return params.mViewHeight;
        }
        else if constexpr(std::is_same_v<T, graphics::PerspectiveParameters>)
        {
            // Note: the plane height might be cached, but this would violate DRY.
            // Compute each time for the time being.
            // View height in the plane of the polar origin (plane perpendicular to the view vector)
            // (the view height depends on the "plane distance" in the perspective case).
            return graphics::computePlaneHeight(orbital.radius(), params.mVerticalFov);
        }
    }, mCamera.getProjectionParameters());
}


void CameraSystem::onFramebufferResize(math::Size<2, int> aNewSize)
{
    std::visit([aNewSize, this](auto params)
    {
        params.mAspectRatio = math::getRatio<GLfloat>(aNewSize);
        this->mCamera.setupProjection(params);
    }, mCamera.getProjectionParameters());
}


void CameraSystemGui::presentSection(CameraSystem & aCameraSystem)
{
    if (ImGui::CollapsingHeader("Camera"))
    {
        // Camera control
        {
            ImGui::SeparatorText("Camera control");

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

            if(ImGui::Button("Home"))
            {
                aCameraSystem.resetCamera();
            }

            switch(aCameraSystem.mActive)
            {
                case CameraSystem::Control::FirstPerson:
                {
                    const SolidEulerPose & pose = aCameraSystem.mFirstPersonControl.mPose;
                    if (ImGui::BeginTable("fp_pose", 3))
                    {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("Position");
                        ImGui::TableNextColumn();
                        ImGui::Text("%.2f, %.2f, %.2f", pose.mPosition.x(), pose.mPosition.y(), pose.mPosition.z());

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("Yaw");
                        ImGui::TableNextColumn();
                        ImGui::Text("%.2f°", pose.mOrientation.y.as<math::Degree>().value());

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("Pitch");
                        ImGui::TableNextColumn();
                        ImGui::Text("%.2f°", pose.mOrientation.x.as<math::Degree>().value());

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("Roll");
                        ImGui::TableNextColumn();
                        ImGui::Text("%.2f°", pose.mOrientation.z.as<math::Degree>().value());

                        ImGui::EndTable();
                    }
                    break;
                }
                case CameraSystem::Control::Orbital:
                {
                    const Orbital & orbital = aCameraSystem.mOrbitalControl.mOrbital;
                    if (ImGui::BeginTable("fp_pose", 3))
                    {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("Origin");
                        ImGui::TableNextColumn();
                        ImGui::Text("%.2f, %.2f, %.2f", orbital.mSphericalOrigin.x(), orbital.mSphericalOrigin.y(), orbital.mSphericalOrigin.z());

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("Radius");
                        ImGui::TableNextColumn();
                        ImGui::Text("%.3f", orbital.radius());

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("Polar");
                        ImGui::TableNextColumn();
                        ImGui::Text("%.2f°", orbital.mSpherical.polar().as<math::Degree>().value());

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("Azimuthal");
                        ImGui::TableNextColumn();
                        ImGui::Text("%.2f°", orbital.mSpherical.azimuthal().as<math::Degree>().value());

                        ImGui::EndTable();
                    }
                    break;
                }
            }
        }

        // Projection
        {
            ImGui::SeparatorText("Projection");

            auto projectionParams = aCameraSystem.mCamera.getProjectionParameters();

            // local semantic only: 0 Orthographic, 1 Perspective
            enum Projection
            {
                Orthographic = 0,
                Perspective = 1,
            };
            int projectionId = std::holds_alternative<graphics::OrthographicParameters>(projectionParams) ?
                Orthographic : Perspective;
            const int previousProjectionId = projectionId;

            ImGui::RadioButton("Orthographic",
                               &projectionId,
                               Orthographic);
            ImGui::SameLine();
            ImGui::RadioButton("Perspective",
                               &projectionId,
                               Perspective);

            // Note: initially, we used the return value from ImGui::RadioButton to detect change
            // but clicking the already selected button would also return true.
            bool changed = (projectionId != previousProjectionId);

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
                                graphics::computePlaneHeight(radius, perspective.mVerticalFov));
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