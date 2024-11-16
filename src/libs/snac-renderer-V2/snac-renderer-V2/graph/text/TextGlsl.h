#pragma once


#include "../../Model.h"

#include <math/Vector.h>

#include <renderer/GL_Loader.h>


namespace ad::renderer {


    struct alignas(16) GlyphMetrics_glsl
    {
        math::Size<2, GLfloat> mBoundingBox_pix; // glyph bounding box in texture pixel coordinates, including margins.
        math::Vec<2, GLfloat> mBearing_pix; // bearing, including  margins.
        math::Vec<2, GLuint> mOffsetInTexture_pix; // horizontal offset to the glyph in its ribbon texture.
    };


    /// @brief Intended to be loaded as instance data (GL_ARRAY_BUFFER with divisor 1)
    struct GlyphInstanceData
    {
        GLuint mGlyphIdx;
        math::Position<2, GLfloat> mPosition_stringPix;
    };

    using ClientText = std::vector<GlyphInstanceData>;

    /// @brief Models a string entity in the scene.
    struct alignas(16) StringEntity_glsl
    {
        math::AffineMatrix<4, GLfloat> mStringPixToWorld;
        math::hdr::Rgba_f mColor;
    };

    struct TextPart : public Part
    {
        Handle<const graphics::BufferAny> mGlyphInstanceBuffer;
        Handle<const graphics::BufferAny> mInstanceToStringEntityBuffer;
        Handle<graphics::UniformBufferObject> mStringEntitiesUbo;
    };

    GenericStream makeGlyphInstanceStream(renderer::Storage & aStorage);


} // namespace ad::renderer