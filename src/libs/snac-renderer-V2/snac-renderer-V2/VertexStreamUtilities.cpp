#include "VertexStreamUtilities.h"

#include <profiler/GlApi.h>


namespace ad::renderer {


namespace {


    BufferView makeBufferView(graphics::BufferAny * aBuffer,
                              GLsizei aElementSize,
                              GLsizeiptr aElementCount,
                              GLuint aInstanceDivisor,
                              GLintptr aOffsetIntoBuffer)
    {
        const GLsizeiptr bufferSize = aElementSize * aElementCount;

        return BufferView{
            .mGLBuffer = aBuffer,
            .mStride = aElementSize,
            .mInstanceDivisor = aInstanceDivisor,
            .mOffset = aOffsetIntoBuffer,
            .mSize = bufferSize, // The view has access to the provided range of elements
        };
    };


} // unnamed namespace


Handle<VertexStream> makeVertexStream(unsigned int aVerticesCount,
                                      unsigned int aIndicesCount,
                                      std::span<const AttributeDescription> aBufferedStreams,
                                      Storage & aStorage,
                                      const GenericStream & aStream,
                                      // This is probably only useful to create debug data buffers
                                      // (as there is little sense to not reuse the existing views)
                                      const VertexStream * aVertexStream)
{
    // TODO Ad 2023/10/11: Should we support smaller index types.
    using Index_t = GLuint;
    BufferView iboView = [&]()
    {
        if(aVertexStream)
        {
            const BufferView & previousIndexBufferView = aVertexStream->mIndexBufferView;
            GLintptr indexOffset = previousIndexBufferView.mSize;
            graphics::BufferAny * existingBuffer = aVertexStream->mIndexBufferView.mGLBuffer;
            return makeBufferView(existingBuffer, sizeof(Index_t), aIndicesCount, 0, indexOffset);
        }
        else
        {
            return createBuffer(sizeof(Index_t), aIndicesCount, 0, GL_STATIC_DRAW, aStorage);
        }
    }();

    // The consolidated vertex stream
    aStorage.mVertexStreams.push_back({
        .mVertexBufferViews{aStream.mVertexBufferViews},
        .mSemanticToAttribute{aStream.mSemanticToAttribute},
        .mIndexBufferView = iboView,
        .mIndicesType = graphics::MappedGL_v<Index_t>,
    });

    VertexStream & vertexStream = aStorage.mVertexStreams.back();

    for(const auto & attribute : aBufferedStreams)
    {
        const GLsizei attributeSize = 
            attribute.mDimension.countComponents() * graphics::getByteSize(attribute.mComponentType);

        BufferView attributeView = [&]()
        {
            if(aVertexStream)
            {
                // Will throw if there is no buffer for the attribute's semantic in the source VertexStream.
                const BufferView & previousBufferView = 
                    aVertexStream->mVertexBufferViews.at(
                        aVertexStream->mSemanticToAttribute.at(attribute.mSemantic).mBufferViewIndex);
                graphics::BufferAny * attributeBuffer = previousBufferView.mGLBuffer;

                return makeBufferView(attributeBuffer, attributeSize, aVerticesCount, 0, previousBufferView.mSize);
            }
            else
            {
                return createBuffer(attributeSize, aVerticesCount, 0, GL_STATIC_DRAW, aStorage);
            }
        }();

        // TODO allow interleaving of vertex attributes
        vertexStream.mSemanticToAttribute.emplace(
            attribute.mSemantic,
            AttributeAccessor{
                .mBufferViewIndex = vertexStream.mVertexBufferViews.size(), // view is added next
                .mClientDataFormat{
                    .mDimension = attribute.mDimension,
                    .mOffset = 0, // No interleaving is hardcoded at the moment
                    .mComponentType = attribute.mComponentType,
                },
            }
        );

        vertexStream.mVertexBufferViews.push_back(attributeView);
    }

    return &vertexStream;
}


BufferView createBuffer(GLsizei aElementSize,
                        GLsizeiptr aElementCount,
                        GLuint aInstanceDivisor,
                        GLenum aHint,
                        Storage & aStorage)
{
    graphics::BufferAny glBuffer; // glGenBuffers()
    // Note: Using a random target, the underlying buffer objects are all identical.
    constexpr auto target = graphics::BufferType::Array;
    graphics::ScopedBind boundBuffer{glBuffer, target}; // glBind()

    const GLsizeiptr bufferSize = aElementSize * aElementCount;

    gl.BufferData(
        static_cast<GLenum>(target),
        bufferSize,
        nullptr,
        aHint);

    aStorage.mBuffers.push_back(std::move(glBuffer));
    graphics::BufferAny * buffer = &aStorage.mBuffers.back();

    return BufferView{
        .mGLBuffer = buffer,
        .mStride = aElementSize,
        .mInstanceDivisor = aInstanceDivisor,
        .mOffset = 0,
        .mSize = bufferSize, // The view has access to the whole buffer ATM
    };
};


Handle<ConfiguredProgram> storeConfiguredProgram(IntrospectProgram aProgram, Storage & aStorage)
{
    aStorage.mPrograms.push_back(ConfiguredProgram{
        .mProgram = std::move(aProgram),
        // TODO Share Configs between programs that are compatibles (i.e., same input semantic and type at same attribute index)
        // (For simplicity, we create one Config per program at the moment).
        .mConfig = (aStorage.mProgramConfigs.emplace_back(), &aStorage.mProgramConfigs.back()),
    });
    return &aStorage.mPrograms.back();
}


} // namespace ad::renderer