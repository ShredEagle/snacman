#pragma once


#include "GraphicState.h"

#include <snac-renderer/Mesh.h>
#include <snac-renderer/Render.h>

#include <graphics/AppInterface.h>


namespace ad {

// Forward declarations
namespace snac {
    class Resources;
} // namespace snac

namespace snacgame {

class Renderer;

// TODO #generic-render I would like the render code to be generic so we get rid of these specialized render classes.
struct TextRenderer
{
    TextRenderer();

    void render(Renderer & aRenderer, 
                const visu::GraphicState & aState,
                const snac::UniformRepository & aUniforms,
                const snac::UniformBlocks & aUniformBlocks);

    snac::InstanceStream mGlyphInstances;
};

class Renderer
{
    friend struct TextRenderer;

public:
    using GraphicState_t = visu::GraphicState;

    Renderer(graphics::AppInterface & aAppInterface);

    void resetProjection(float aAspectRatio, snac::Camera::Parameters aParameters);

    // TODO Extend beyond cubes.
    static std::shared_ptr<snac::Mesh> LoadShape(filesystem::path aShape, snac::Resources & aResources);

    std::shared_ptr<snac::Font> loadFont(arte::FontFace aFontFace,
                                         unsigned int aPixelHeight,
                                         snac::Resources & aResources);

    void render(const visu::GraphicState & aState);

private:
    graphics::AppInterface & mAppInterface;
    snac::Renderer mRenderer;
    snac::CameraBuffer mCamera;
    snac::InstanceStream mMeshInstances;
    TextRenderer mTextRenderer;
};


} // namespace snacgame
} // namespace ad