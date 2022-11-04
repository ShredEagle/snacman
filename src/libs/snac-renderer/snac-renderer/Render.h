#pragma once

#include "Mesh.h"

#include <renderer/GL_Loader.h>
#include <renderer/Shading.h>


namespace ad {
namespace snac {


inline void clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}


class Renderer
{
public:
    Renderer();

    void render(const Mesh & aMesh) const;

private:
    graphics::Program mProgram;
};


} // namespace snac
} // namespace ad
