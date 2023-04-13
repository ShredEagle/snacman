#pragma once


#include "GraphicState.h"

#include <snac-renderer/Camera.h>
#include <snac-renderer/LoadInterface.h>
#include <snac-renderer/Mesh.h>
#include <snac-renderer/Render.h>
#include <snac-renderer/UniformParameters.h>

#include <snac-renderer/pipelines/ForwardShadows.h>

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

class Renderer;

// TODO #generic-render I would like the render code to be generic so we get rid of these specialized render classes.
struct TextRenderer
{
    TextRenderer();

    void render(Renderer & aRenderer, 
                const visu::GraphicState & aState,
                snac::ProgramSetup & aProgramSetup);

    snac::InstanceStream mGlyphInstances;
};

class Renderer
{
    friend struct TextRenderer;

public:
    using GraphicState_t = visu::GraphicState;

    Renderer(graphics::AppInterface & aAppInterface, snac::Load<snac::Technique> & aTechniqueAccess);

    void resetProjection(float aAspectRatio, snac::Camera::Parameters aParameters);

    // TODO Extend beyond cubes.
    static std::shared_ptr<snac::Model> LoadModel(filesystem::path aModel, snac::Resources & aResources);

    std::shared_ptr<snac::Font> loadFont(arte::FontFace aFontFace,
                                         unsigned int aPixelHeight,
                                         snac::Resources & aResources);

    void continueGui();

    void render(const visu::GraphicState & aState);

    /// \brief Forwards the request to reset repositories down to the generic renderer.
    void resetRepositories()
    { mRenderer.resetRepositories(); }

private:
    graphics::AppInterface & mAppInterface;
    snac::Renderer mRenderer;
    // TODO Is it the correct place to host the pipeline instance?
    // This notably force to instantiate it with the Renderer (before the Resources manager is available).
    snac::ForwardShadows mPipelineShadows;
    snac::Pass mTextPass{"text"};
    snac::CameraBuffer mCamera;
    snac::InstanceStream mMeshInstances;
    TextRenderer mTextRenderer;
};


} // namespace snacgame
} // namespace ad
