#include "VertexStreamUtilities.h"

#include <profiler/GlApi.h>


namespace ad::renderer {


namespace {


    BufferView makeBufferView(const graphics::BufferAny * aBuffer,
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


Handle<graphics::BufferAny> makeBuffer(GLsizei aElementSize,
                                       GLsizeiptr aElementCount,
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
    return &aStorage.mBuffers.back();
}


BufferView makeBufferGetView(GLsizei aElementSize,
                             GLsizeiptr aElementCount,
                             GLuint aInstanceDivisor,
                             GLenum aHint,
                             Storage & aStorage)
{
    return makeBufferView(
        makeBuffer(aElementSize, aElementCount, aHint, aStorage),
        aElementSize,
        aElementCount,
        aInstanceDivisor,
        0/*offset*/);
};


Handle<VertexStream> primeVertexStream(const GenericStream & aGenericStream, 
                                       Storage & aStorage)
{

    aStorage.mVertexStreams.push_back({
        .mVertexBufferViews{aGenericStream.mVertexBufferViews},
        .mSemanticToAttribute{aGenericStream.mSemanticToAttribute},
    });

    return & aStorage.mVertexStreams.back();
}


void setIndexBuffer(Handle<VertexStream> aVertexStream, 
                    GLenum aIndexType,
                    Handle<const graphics::BufferAny> aIndexBuffer,
                    unsigned int aIndicesCount,
                    GLintptr aBufferOffset)
{
    aVertexStream->mIndexBufferView = makeBufferView(aIndexBuffer,
                                                     graphics::getByteSize(aIndexType),
                                                     aIndicesCount,
                                                     0,
                                                     aBufferOffset);
    aVertexStream->mIndicesType = aIndexType;
}


void addVertexAttributes(Handle<VertexStream> aVertexStream, 
                         AttributeDescription aAttribute,
                         Handle<const graphics::BufferAny> aVertexBuffer,
                         unsigned int aVerticesCount,
                         GLintptr aBufferOffset)
{
    BufferView attributeView = makeBufferView(aVertexBuffer,
                                              getByteSize(aAttribute),
                                              aVerticesCount,
                                              0, // This is a per vertex attribute, divisor is 0
                                              aBufferOffset);

    aVertexStream->mSemanticToAttribute.emplace(
        aAttribute.mSemantic,
        AttributeAccessor{
            .mBufferViewIndex = aVertexStream->mVertexBufferViews.size(), // view is added next
            .mClientDataFormat{
                .mDimension = aAttribute.mDimension,
                // TODO allow interleaving of vertex attributes
                .mOffset = 0, // No interleaving is hardcoded at the moment
                .mComponentType = aAttribute.mComponentType,
            },
        }
    );

    aVertexStream->mVertexBufferViews.push_back(std::move(attributeView));
}



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


Handle<VertexStream> makeVertexStream(unsigned int aVerticesCount,
                                      unsigned int aIndicesCount,
                                      GLenum aIndexType,
                                      std::span<const AttributeDescription> aBufferedStreams,
                                      Storage & aStorage,
                                      const GenericStream & aStream)
{
    // TODO Ad 2023/10/11: Should we support smaller index types.
    BufferView iboView = makeBufferGetView(
        graphics::getByteSize(aIndexType),
        aIndicesCount,
        0,
        GL_STATIC_DRAW,
        aStorage);

    // The consolidated vertex stream
    aStorage.mVertexStreams.push_back({
        .mVertexBufferViews{aStream.mVertexBufferViews},
        .mSemanticToAttribute{aStream.mSemanticToAttribute},
        .mIndexBufferView = iboView,
        .mIndicesType = aIndexType,
    });

    VertexStream & vertexStream = aStorage.mVertexStreams.back();

    for(const auto & attribute : aBufferedStreams)
    {
        const GLsizei attributeSize = 
            attribute.mDimension.countComponents() * graphics::getByteSize(attribute.mComponentType);

        BufferView attributeView = makeBufferGetView(
            attributeSize,
            aVerticesCount,
            0,
            GL_STATIC_DRAW,
            aStorage);

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


} // namespace ad::renderer