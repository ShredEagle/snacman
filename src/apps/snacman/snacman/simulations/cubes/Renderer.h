#pragma once


#include "GraphicState.h"

#include <snac-renderer/Mesh.h>
#include <snac-renderer/Render.h>


namespace ad {
namespace cubes {


class Renderer
{
public:
    using GraphicState_t = GraphicState;

    Renderer(float aAspectRatio);

    void render(const GraphicState & aState);

private:
    snac::Renderer mRenderer;

    snac::Camera mCamera;
    snac::Mesh mCubeMesh;
    snac::InstanceStream mInstances;
};


} // namespace cubes
} // namespace ad
