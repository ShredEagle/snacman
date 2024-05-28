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

    void update(float aDeltaTime);

    std::shared_ptr<graphics::AppInterface> mAppInterface;
    const imguiui::ImguiUi * mImguiUi; //for inhibiters
    Camera mCamera;
    Control mActive{Control::FirstPerson}; 
    OrbitalControl mOrbitalControl;
    FirstPersonControl mFirstPersonControl;
};


struct CameraSystemGui
{
    void presentSection(CameraSystem & aCameraSystem);
};


} // namespace ad::renderer