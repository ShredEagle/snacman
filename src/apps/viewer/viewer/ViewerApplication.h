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
    ViewerApplication(std::shared_ptr<graphics::AppInterface> aGlfwAppInterface,
                      const std::filesystem::path & aSceneFile,
                      const imguiui::ImguiUi & aImguiUi);

    void setEnvironmentCubemap(std::filesystem::path aEnvironmentStrip);
    void setEnvironmentEquirectangular(std::filesystem::path aEnvironmentEquirect);

    // TODO Ad 2024/06/27: This should probably live in a separate preprocessing application
    // instead of polluting the viewer with asset processing.
    void dumpEnvironmentCubemap(std::filesystem::path aOutputColumn);

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
};


} // namespace ad::renderer