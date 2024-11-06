#pragma once


#include <math/Vector.h>


namespace ad::renderer {


struct ShadowCascadeBlock
{
    // TODO Ad 2024/10/11: #shader-system this debug toggle is too invasive here.
    // (Hopefully we land on a better solution for those kind of debug parameterization
    // of shaders while redesigning the shader system)
    // Note: by std140, a GLSL bool is 4 bytes aligned.
    //alignas(4) GLboolean mDebugTintCascade{GL_FALSE}; // Does not work, it seems the GLSL bool is actually 4 bytes
    std::int32_t mDebugTintCascade{GL_FALSE};
    alignas(16) math::Vec<4, GLfloat> mCascadeFarPlaneDepths_view;
};


} // namespace ad::renderer