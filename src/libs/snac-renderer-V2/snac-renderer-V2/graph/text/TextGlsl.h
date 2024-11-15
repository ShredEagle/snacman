#pragma once


#include <math/Vector.h>

#include <renderer/GL_Loader.h>


namespace ad::renderer {


    struct alignas(16) GlyphMetrics_glsl
    {
        math::Size<2, GLfloat> mBoundingBox_pix; // glyph bounding box in texture pixel coordinates, including margins.
        math::Vec<2, GLfloat> mBearing_pix; // bearing, including  margins.
        math::Vec<2, GLuint> mOffsetInTexture_pix; // horizontal offset to the glyph in its ribbon texture.
    };


} // namespace ad::renderer