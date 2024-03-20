#pragma once


#include "Scene.h"
#include "SceneGui.h"
#include "TheGraph.h"

#include <snac-renderer-V2/Camera.h>
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

    // for camera movements, should be moved out to the simuation of course
    void update(const Timing & aTime);
    void render();

    void drawUi(const Timing & aTime);

    std::shared_ptr<graphics::AppInterface> mGlfwAppInterface;
    Storage mStorage;
    Scene mScene;
    Loader mLoader;
    SceneGui mSceneGui{mLoader.loadEffect("effects/Highlight.sefx", mStorage), mStorage};

    Camera mCamera;
    
    // TODO #camera allow both control modes, ideally via DearImgui
    OrbitalControl mCameraControl;
    FirstPersonControl mFirstPersonControl;

    TheGraph mGraph;
};


} // namespace ad::renderer