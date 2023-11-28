#pragma once


#include "Camera.h"
#include "Model.h"
#include "Loader.h"
#include "Scene.h"
#include "SceneGui.h"
#include "TheGraph.h"

#include <graphics/ApplicationGlfw.h>


namespace ad::imguiui {
    class ImguiUi;
} // namespace ad::imguiui


namespace ad::renderer {


struct PassCache;


struct RenderGraph
{
    RenderGraph(std::shared_ptr<graphics::AppInterface> aGlfwAppInterface,
                const std::filesystem::path & aSceneFile,
                const imguiui::ImguiUi & aImguiUi);

    // for camera movements, should be moved out to the simuation of course
    void update(float aDeltaTime);
    void render();

    void drawUi();

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