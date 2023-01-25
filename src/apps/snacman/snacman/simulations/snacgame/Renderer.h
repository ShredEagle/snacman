#pragma once


#include "GraphicState.h"

#include <snac-renderer/Mesh.h>
#include <snac-renderer/Render.h>

#include <graphics/AppInterface.h>

// TODO #generic-render remove once all geometry and shader programs are created outside.
#include <snac-renderer/Cube.h>
#include <renderer/ShaderSource.h>
#include <resource/ResourceFinder.h>

namespace ad {
namespace snacgame {


// TODO #generic-render I would like the render code to be generic so we get rid of these specialized render classes.
struct TextRenderer
{
    TextRenderer();

    void render(snac::Renderer & aRenderer, 
                const visu::GraphicState & aState,
                const snac::UniformRepository & aUniforms,
                const snac::UniformBlocks & aUniformBlocks);

    snac::InstanceStream mGlyphInstances;
};

class Renderer
{
public:
    using GraphicState_t = visu::GraphicState;

    Renderer(graphics::AppInterface & aAppInterface,
             snac::Camera::Parameters aCameraParameters,
             const resource::ResourceFinder & aResourceFinder /* TODO remove when not used anymore*/);

    void resetProjection(float aAspectRatio, snac::Camera::Parameters aParameters);

    void render(const visu::GraphicState & aState);

private:
    graphics::AppInterface & mAppInterface;

    snac::Renderer mRenderer;

    snac::Camera mCamera;

    snac::Mesh mCubeMesh;
    snac::InstanceStream mCubeInstances;

    TextRenderer mTextRenderer;
};


} // namespace snacgame
} // namespace ad