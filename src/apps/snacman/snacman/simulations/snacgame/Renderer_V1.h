#pragma once


#include "GraphicState.h"

#include <snac-renderer-V1/Camera.h>
#include <snac-renderer-V1/DebugRenderer.h>
#include <snac-renderer-V1/LoadInterface.h>
#include <snac-renderer-V1/Mesh.h>
#include <snac-renderer-V1/Render.h>
#include <snac-renderer-V1/text/TextRenderer.h>
#include <snac-renderer-V1/UniformParameters.h>

#include <snac-renderer-V1/pipelines/ForwardShadows.h>

#include <filesystem>
#include <memory>

namespace ad {
namespace arte { class FontFace; }
namespace graphics { class AppInterface; }

// Forward declarations
namespace snac {
    class Resources;
    struct Font;
} // namespace snac

namespace snacgame {


class Renderer
{
    friend struct TextRenderer;

    struct Control
    {
        MovableAtomic<bool> mRenderModels{true};
        MovableAtomic<bool> mRenderText{true};
        MovableAtomic<bool> mRenderDebug{true};

        // This boolean is only accessed by main thread
        bool mShowShadowControls{false};
    };

public:
    using GraphicState_t = visu::GraphicState;

    Renderer(graphics::AppInterface & aAppInterface,
             snac::Load<snac::Technique> & aTechniqueAccess,
             arte::FontFace aDebugFontFace);

    //void resetProjection(float aAspectRatio, snac::Camera::Parameters aParameters);

    static std::shared_ptr<snac::Model> LoadModel(filesystem::path aModel,
                                                  filesystem::path aEffect,
                                                  snac::Resources & aResources);

    std::shared_ptr<snac::Font> loadFont(arte::FontFace aFontFace,
                                         unsigned int aPixelHeight,
                                         filesystem::path aEffect,
                                         snac::Resources & aResources);

    void continueGui();

    void render(const visu::GraphicState & aState);

    /// \brief Forwards the request to reset repositories down to the generic renderer.
    void resetRepositories()
    { mRenderer.resetRepositories(); }

private:
    template <class T_range>
    void renderText(const T_range & aTexts, snac::ProgramSetup & aProgramSetup);

    Control mControl;
    graphics::AppInterface & mAppInterface;
    snac::Renderer mRenderer;
    // TODO Is it the correct place to host the pipeline instance?
    // This notably force to instantiate it with the Renderer (before the Resources manager is available).
    snac::ForwardShadows mPipelineShadows;
    snac::Camera mCamera;
    snac::CameraBuffer mCameraBuffer;
    snac::TextRenderer mTextRenderer;
    snac::GlyphInstanceStream mDynamicStrings;
    snac::DebugRenderer mDebugRenderer;
    graphics::UniformBufferObject mJointMatrices;
};


} // namespace snacgame
} // namespace ad
