#pragma once


#include "GraphicState.h"

#include <snac-renderer/Mesh.h>
#include <snac-renderer/Render.h>


namespace ad {
namespace snacgame {


class Renderer
{
public:
    using GraphicState_t = visu::GraphicState;

    Renderer(float aAspectRatio, snac::Camera::Parameters aCameraParameters);

    void resetProjection(float aAspectRatio, snac::Camera::Parameters aParameters);

    void render(const visu::GraphicState & aState);

private:
    snac::Renderer mRenderer;

    snac::Camera mCamera;

    snac::Mesh mCubeMesh;
    snac::InstanceStream mInstances;
};


} // namespace cubes
} // namespace ad
