#pragma once


#include <graphics/AppInterface.h>

#include <imguiui/ImguiUi.h>

#include <snac-renderer-V2/Camera.h>

#include <memory>


namespace ad::renderer {


struct CameraSystem
{
    enum class Control
    {
        Orbital,
        FirstPerson,
    };

    CameraSystem(std::shared_ptr<graphics::AppInterface> aAppInterface,
                 const imguiui::ImguiUi * aImguiUi,
                 Control aMode);

    
    /// \brief High-level function configuring the CameraSystem to view a model
    /// defined by its bounding box
    void setupAsModelViewer(math::Box<float> aModelAabb);

    void setControlMode(Control aMode);

    void resetCamera();

    void update(float aDeltaTime);

    /// @brief The view height in world coordinates.
    /// For orthographic projection, this is the viewed height.
    /// For perspective, this is taken has the heigh of the view plane placed at orbital origin
    /// (even orbital control is not currently active).
    float getViewHeight() const;

    std::shared_ptr<graphics::AppInterface> mAppInterface;
    const imguiui::ImguiUi * mImguiUi; //for inhibiters
    Camera mCamera;
    Control mActive{Control::FirstPerson}; 
    Orbital mOrbitalHome; // Record the orbital pose considered home.
    OrbitalControl mOrbitalControl;
    FirstPersonControl mFirstPersonControl;
    // Hackish member, saving the last orbital radius to be able to recompute equivalent FOV for orthographic zoom.
    float mPreviousRadius;
};


struct CameraSystemGui
{
    void presentSection(CameraSystem & aCameraSystem);
};


} // namespace ad::renderer