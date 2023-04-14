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
             Font aFont,
             DebugRenderer aDebugRenderer,
             const resource::ResourceFinder & aFinder) :
    mAppInterface{*aGlfwApp.getAppInterface()},
    mCamera{math::getRatio<float>(mAppInterface.getFramebufferSize()), Camera::gDefaults},
    mCameraBuffer{math::getRatio<float>(mAppInterface.getFramebufferSize()), Camera::gDefaults},
    mCameraControl{mAppInterface.getWindowSize(), Camera::gDefaults.vFov},
    mFinder{aFinder},
    mDebugRenderer{std::move(aDebugRenderer)},
    mFont{std::move(aFont)}
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
            //mCameraBuffer.resetProjection(math::getRatio<float>(size), Camera::gDefaults); 
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
    clear();

    math::Size<2, int> FbSize = mAppInterface.getFramebufferSize();

    auto viewportScope = graphics::scopeViewport({{0, 0}, FbSize});

    ProgramSetup setup{
        .mUniforms{
            {snac::Semantic::FramebufferResolution, FbSize},
            {snac::Semantic::ViewingMatrix, mCamera.assembleViewMatrix()},
        },
        //.mUniformBlocks{
        //    {BlockSemantic::Viewing, &mCameraBuffer.mViewing},
        //},
    };

    const auto identity = math::AffineMatrix<4, GLfloat>::Identity();

    // World space text
    {
        mGlyphs.respecifyData(
            std::span{
                pass(mFont.mFontData.populateInstances("|worldspace", math::sdr::gWhite, identity))
            });
        //mTextRenderer.render(mGlyphs, mFont, aRenderer, setup);
    }

    // View space text
    {
        auto setupScope = setup.mUniforms.push({
            {snac::Semantic::ViewingMatrix, identity},
        });

        // Text size fixed in absolute numbers of pixels (i.e. independent of Window size)
        // This is good for rasterized text, where the glyph screen coverage should not change with framebuffer size.
        {
            math::Vec<3, GLfloat> textScreenPosition_ndc{-0.8f, 0.75f, 0.f};
            // This is in pixels, with origin at the center of the frame buffer
            // (i.e. going from -half_resolution to + half_resolution)
            math::Vec<3, GLfloat> textScreenPosition_pix = 
                textScreenPosition_ndc.cwMul(math::Vec<3, GLfloat>{
                    static_cast<math::Vec<2, GLfloat>>(FbSize)/2.f,
                    1.f});

            math::AffineMatrix<4, GLfloat> stringPixelToScreenPixel = 
                math::trans3d::translate(textScreenPosition_pix)
                ;

            mGlyphs.respecifyData(
                std::span{
                    pass(mFont.mFontData.populateInstances("viewspace: fixed-pixel", math::sdr::gWhite, stringPixelToScreenPixel))
                });
            mTextRenderer.render(mGlyphs, mFont, aRenderer, setup);
        }
    }

    //
    // Debug drawer
    //
    mDebugRenderer.render(DebugDrawer::EndFrame(), aRenderer, setup);
}


} // namespace snac
} // namespace ad