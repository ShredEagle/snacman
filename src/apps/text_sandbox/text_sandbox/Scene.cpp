#include "Scene.h"

#include <graphics/CameraUtilities.h>

#include <math/Transformations.h>

#include <snac-renderer/DebugDrawer.h>
#include <snac-renderer/ResourceLoad.h>


namespace ad {
namespace snac {


const std::string gDebugDrawer = "textdebugdrawer";

#define SEDRAW(drawer) ::ad::snac::DebugDrawer::Get(drawer)

Scene::Scene(graphics::ApplicationGlfw & aGlfwApp,
             DebugRenderer aDebugRenderer,
             const resource::ResourceFinder & aFinder) :
    mAppInterface{*aGlfwApp.getAppInterface()},
    mCamera{math::getRatio<float>(mAppInterface.getFramebufferSize()), Camera::gDefaults},
    mCameraBuffer{math::getRatio<float>(mAppInterface.getFramebufferSize()), Camera::gDefaults},
    mCameraControl{mAppInterface.getWindowSize(), Camera::gDefaults.vFov},
    mFinder{aFinder},
    mDebugRenderer{std::move(aDebugRenderer)}
{
    DebugDrawer::AddDrawer(gDebugDrawer);

    mAppInterface.registerCursorPositionCallback(
        [this](double xpos, double ypos)
        {
            //if(!mGui.mImguiUi.isCapturingMouse())
            //{
                mCameraControl.callbackCursorPosition(xpos, ypos);
            //}
        });

    mAppInterface.registerMouseButtonCallback(
        [this](int button, int action, int mods, double xpos, double ypos)
        {
            //if(!mGui.mImguiUi.isCapturingMouse())
            //{
                mCameraControl.callbackMouseButton(button, action, mods, xpos, ypos);
            //}
        });

    mAppInterface.registerScrollCallback(
        [this](double xoffset, double yoffset)
        {
            //if(!mGui.mImguiUi.isCapturingMouse())
            //{
                mCameraControl.callbackScroll(xoffset, yoffset);
            //}
        });

    mSizeListening = mAppInterface.listenWindowResize(
        [this](const math::Size<2, int> & size)
        {
            mCamera.setPerspectiveProjection(math::getRatio<float>(size), Camera::gDefaults); 
            mCameraBuffer.resetProjection(math::getRatio<float>(size), Camera::gDefaults); 
            mCameraControl.setWindowSize(size);
        });

}


void Scene::update()
{
    using Level = DebugDrawer::Level;

    DebugDrawer::StartFrame();
    mCamera.setPose(mCameraControl.getParentToLocal());
    mCameraBuffer.setWorldToCamera(mCameraControl.getParentToLocal());
    SEDRAW(gDebugDrawer)->addBasis(Level::info, {});
}


void Scene::render(Renderer & aRenderer)
{
    // TODO #camera remove that local camera
    ProgramSetup setup{
        .mUniforms{
            {snac::Semantic::ViewingMatrix, mCamera.assembleViewMatrix()},
        },
        .mUniformBlocks{
            {BlockSemantic::Viewing, &mCameraBuffer.mViewing},
        },
    };

    clear();
    mDebugRenderer.render(DebugDrawer::EndFrame(), aRenderer, setup);
}


} // namespace snac
} // namespace ad