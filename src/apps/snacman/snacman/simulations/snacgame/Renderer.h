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

    Renderer(graphics::AppInterface & aAppInterface,
             const resource::ResourceFinder & aResourceFinder /* TODO remove when not used anymore*/);

    void resetProjection(float aAspectRatio, snac::Camera::Parameters aParameters);

    // TODO Extend beyond cubes.
    static std::shared_ptr<snac::Mesh> LoadShape(const resource::ResourceFinder & aResourceFinder);

    std::shared_ptr<snac::Font> loadFont(filesystem::path aFont,
                                         unsigned int aPixelHeight,
                                         const resource::ResourceFinder & aResource);

    void render(const visu::GraphicState & aState);

private:
    graphics::AppInterface & mAppInterface;
    snac::Renderer mRenderer;
    snac::Camera mCamera;
    snac::InstanceStream mMeshInstances;
    TextRenderer mTextRenderer;

    // Must outlive all FontFaces, thus it must outlive the EntityManager,
    // which might contain Freetype FontFaces. 
    arte::Freetype mFreetype;
};


} // namespace snacgame
} // namespace ad