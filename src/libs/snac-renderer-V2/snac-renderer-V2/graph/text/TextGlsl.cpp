#include "TextGlsl.h"


#include "../../Semantics.h"

#include "../../utilities/VertexStreamUtilities.h"


namespace ad::renderer {


GenericStream makeGlyphInstanceStream(Storage & aStorage)
{
    BufferView glyphView = makeBufferGetView(sizeof(GlyphInstanceData),
                                             0, // Do not allocate storage space
                                             1, // intance divisor
                                             GL_STREAM_DRAW,
                                             aStorage);

    BufferView glyphToStringView = makeBufferGetView(sizeof(GLuint),
                                                     0, // Do not allocate storage space
                                                     1, // intance divisor
                                                     GL_STREAM_DRAW,
                                                     aStorage);

    // TODO Ad 2024/04/04: #perf Indices should be grouped by 4 in vertex attributes,
    // instead of using a distinct attribute each.
    return GenericStream{
        .mVertexBufferViews = {glyphView, glyphToStringView},
        .mSemanticToAttribute{
            {
                semantic::gGlyphIdx,
                AttributeAccessor{
                    .mBufferViewIndex = 0, // view is added above
                    .mClientDataFormat{
                        .mDimension = 1,
                        .mOffset = offsetof(GlyphInstanceData, mGlyphIdx),
                        .mComponentType = GL_UNSIGNED_INT,
                    },
                }
            },
            {
                semantic::gPosition,
                AttributeAccessor{
                    .mBufferViewIndex = 0, // view is added above
                    .mClientDataFormat{
                        .mDimension = 2,
                        .mOffset = offsetof(GlyphInstanceData, mPosition_stringPix),
                        .mComponentType = GL_FLOAT,
                    },
                }
            },
            {
                semantic::gEntityIdx,
                AttributeAccessor{
                    .mBufferViewIndex = 1, // view is added above
                    .mClientDataFormat{
                        .mDimension = 1,
                        .mOffset = 0,
                        .mComponentType = GL_UNSIGNED_INT
                    },
                }
            },
        }
    };
}


} // namespace ad::renderer