#pragma once


#include "CameraSystem.h"
#include "GraphGui.h"
#include "SecondaryView.h"
#include "Scene.h"
#include "SceneGui.h"
#include "TheGraph.h"

#include <snac-renderer-V2/Model.h>
#include <snac-renderer-V2/files/Loader.h>

#include <graphics/ApplicationGlfw.h>


namespace ad::imguiui {
    class ImguiUi;
} // namespace ad::imguiui


namespace ad::renderer {


struct PassCache;


// An attempt to separate what is related to the main view from what is transverse to views.
struct PrimaryView
{
    // Not easily copyable nor movable, due to the listener which binds to a subobject.
    // (Note: moving the subobject to the heap might be an easy workaround)
    PrimaryView(const PrimaryView &) = delete;
    PrimaryView(PrimaryView &&) = delete;

    PrimaryView(const std::shared_ptr<graphics::AppInterface> & aGlfwAppInterface,
                const imguiui::ImguiUi & aImguiUi,
                Loader & aLoader,
                Storage & aStorage);

    void update(const Timing & aTime);

    void render(const Scene & aScene, bool aLightsInCameraSpace, Storage & aStorage);

    CameraSystem mCameraSystem;
    CameraSystemGui mCameraGui;

    TheGraph mGraph;
    GraphGui mGraphGui;

    std::shared_ptr<graphics::AppInterface::SizeListener> mFramebufferSizeListener;
};


struct ViewerApplication
{
    ViewerApplication(std::shared_ptr<graphics::AppInterface> aGlfwAppInterface,
                      std::shared_ptr<graphics::AppInterface> aSecondViewAppInterface,
                      const std::filesystem::path & aSceneFile,
                      const imguiui::ImguiUi & aMainImguiUi,
                      const imguiui::ImguiUi & aSecondViewImguiUi);

    void setEnvironmentCubemap(std::filesystem::path aEnvironmentStrip);
    void setEnvironmentEquirectangular(std::filesystem::path aEnvironmentEquirect);

    // for camera movements, should be moved out to the simuation of course
    void update(const Timing & aTime);
    void render();

    void renderDebugDrawlist(const snac::DebugDrawer::DrawList & aDrawList,
                             math::Rectangle<int> aViewport,
                             const RepositoryUbo & aUboRepo);

    void drawUi(const Timing & aTime);

    std::shared_ptr<graphics::AppInterface> mGlfwAppInterface;
    std::shared_ptr<graphics::AppInterface> mSecondViewAppInterface;
    Storage mStorage;
    Loader mLoader;
    Scene mScene;
    SceneGui mSceneGui;

    DebugRenderer mDebugRenderer;

    PrimaryView mPrimaryView;
    SecondaryView mSecondaryView;
};


} // namespace ad::renderer