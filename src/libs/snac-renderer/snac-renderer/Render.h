#pragma once

#include "Camera.h"
#include "Mesh.h"
#include "SetupDrawing.h"
#include "UniformParameters.h"

#include <renderer/GL_Loader.h>


namespace ad {
namespace snac {


inline void clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}


class Renderer
{
public:
    void render(const Mesh & aMesh,
                const InstanceStream & aInstances,
                UniformRepository aUniforms,
                const UniformBlocks & aUniformBlocks);

private:
    VertexArrayRepository mVertexArrayRepo;
    WarningRepository mWarningRepo;
};


} // namespace snac
} // namespace ad