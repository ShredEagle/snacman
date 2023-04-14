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
        math::AffineMatrix<4, GLfloat> arbitraryScale = math::trans3d::scaleUniform(2.5f);

        mGlyphs.respecifyData(
            std::span{
                pass(mFont.mFontData.populateInstances("|worldspace", math::sdr::gWhite, arbitraryScale))
            });
        mTextRenderer.render(mGlyphs, mFont, aRenderer, setup);
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

            math::AffineMatrix<4, GLfloat> stringToScreen = 
                math::trans3d::translate(textScreenPosition_ndc)
                ;

            mGlyphs.respecifyData(
                std::span{
                    pass(mFont.mFontData.populateInstances("|viewspace: fixed-pixel", math::sdr::gWhite, stringToScreen))
                });
            mTextRenderer.render(mGlyphs, mFont, aRenderer, setup);
        }

        // Text size fixed in proportion to the window height
        {
            math::Vec<3, GLfloat> textScreenPosition_ndc{-0.8f, 0.60f, 0.f};

            // 600 is the supposed initial Framebuffer height
            // the ratio we apply is the current height to initial height
            const float ratio = FbSize.height() / (float)600;

             math::AffineMatrix<4, GLfloat> stringPixelToScreenNdc = 
                math::trans3d::scale(math::Size<3, GLfloat>{ratio, ratio, 1.f})
                * math::trans3d::translate(textScreenPosition_ndc)
                ;

            mGlyphs.respecifyData(
                std::span{
                    pass(mFont.mFontData.populateInstances("|viewspace: fixed-ndc",
                                                            math::sdr::gWhite,
                                                            stringPixelToScreenNdc))
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