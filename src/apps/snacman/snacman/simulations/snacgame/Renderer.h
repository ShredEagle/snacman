#pragma once


#include "GraphicState.h"

#include <snac-renderer/Mesh.h>
#include <snac-renderer/Render.h>

#include <graphics/AppInterface.h>

// TODO remove this block
#include <snac-renderer/IntrospectProgram.h>
#include <resource/ResourceFinder.h>
#include <renderer/ShaderSource.h>
#include <arte/Freetype.h>
#include <graphics/detail/GlyphUtilities.h>


namespace ad {
namespace snacgame {


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
    arte::Freetype mFreetype;
    graphics::detail::GlyphMap mGlyphMap;
    snac::Mesh mGlyphMesh;
    snac::InstanceStream mGlyphInstances;
};


} // namespace cubes
} // namespace ad
