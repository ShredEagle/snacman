#pragma once


#include <renderer/GL_Loader.h>

#include <renderer/Shading.h>
#include <renderer/VertexSpecification.h>


namespace ad::renderer {


struct DrawQuadParameters
{
    enum Operation : GLuint
    {
        None = 0,
        DepthLinearize1 = 1,
        DepthLinearize2 = 2,
        AccumNormalize = 3, // For transparency accum texture
    };

    GLuint mTextureUnit = 0;
    GLint mSourceChannel = -1; // use all channels
    Operation mOperation = None;
    GLfloat mNearDistance;
    GLfloat mFarDistance;
};


void drawQuad(DrawQuadParameters aParameters = {});


struct QuadDrawer
{
    QuadDrawer(graphics::ShaderSourceView aFragmentCode);

    void drawQuad() const;

    // Should it be an introspect program?
    // It should be accounted for by the storage, for statistics
    graphics::Program mProgram;
    // Sadly a non zero VAO must be bound for any draw call
    // Initialy, this dummy VAO was a static in drawQuad, but it meant it was destroyed after the GL context.
    graphics::VertexArrayObject mDummyVao;
};


} // namespace ad::renderer