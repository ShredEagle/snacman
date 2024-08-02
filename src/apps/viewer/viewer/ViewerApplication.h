#pragma once


#include "CameraSystem.h"
#include "GraphGui.h"
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


struct ViewerApplication
{
    // Not easily copyable nor movable, due to the listener which binds to a subobject.
    // (Note: moving the subobject to the heap might be an easy workaround)
    ViewerApplication(const ViewerApplication &) = delete;
    ViewerApplication(ViewerApplication &&) = delete;

    ViewerApplication(std::shared_ptr<graphics::AppInterface> aGlfwAppInterface,
                      const std::filesystem::path & aSceneFile,
                      const imguiui::ImguiUi & aImguiUi);

    void setEnvironmentCubemap(std::filesystem::path aEnvironmentStrip);
    void setEnvironmentEquirectangular(std::filesystem::path aEnvironmentEquirect);

    // for camera movements, should be moved out to the simuation of course
    void update(const Timing & aTime);
    void render();

    void drawUi(const Timing & aTime);

    std::shared_ptr<graphics::AppInterface> mGlfwAppInterface;
    Storage mStorage;
    Scene mScene;
    Loader mLoader;
    SceneGui mSceneGui{mLoader.loadEffect("effects/Highlight.sefx", mStorage), mStorage};
    GraphGui mGraphGui;

    CameraSystem mCameraSystem;
    CameraSystemGui mCameraGui;

    TheGraph mGraph;
    std::shared_ptr<graphics::AppInterface::SizeListener> mFramebufferSizeListener;
};


} // namespace ad::renderer