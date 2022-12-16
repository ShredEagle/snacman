#include "SetupDrawing.h"

#include "Logging.h"


namespace ad {
namespace snac {

graphics::VertexArrayObject prepareVAO(const Mesh & aMesh,
                                       const InstanceStream & aInstances,
                                       const IntrospectProgram & aProgram)
{
    graphics::VertexArrayObject vertexArray;
    // TODO scoped bind (also remove the unbind)
    bind (vertexArray);

    const VertexStream::VertexBuffer & instanceView = aInstances.mInstanceBuffer;
    bind(instanceView.mBuffer);
    {
        const graphics::ClientAttribute & attribute =
            aInstances.mAttributes.at(Semantic::LocalToWorld).mAttribute;
        // TODO: Remove hardcoding knowledge this will be a multiple of dimensions 4.
        for (int attributeOffset = 0; attributeOffset != (int)attribute.mDimension / 4; ++attributeOffset)
        {
            int dimension = 4; // TODO stop hardcoding
            int shaderIndex = 4 + attributeOffset;
            glVertexAttribPointer(shaderIndex,
                                    dimension,
                                    attribute.mDataType, 
                                    GL_FALSE, 
                                    static_cast<GLsizei>(instanceView.mStride),
                                    (void *)(attribute.mOffset 
                                            + attributeOffset * dimension * graphics::getByteSize(attribute.mDataType)));
            glEnableVertexAttribArray(shaderIndex);
            glVertexAttribDivisor(shaderIndex, 1);
        }
    }
    {
        const graphics::ClientAttribute & attribute =
            aInstances.mAttributes.at(Semantic::Albedo).mAttribute;
        // Note: has to handle normalized attributes here
        glVertexAttribPointer(12, attribute.mDimension, attribute.mDataType, 
                                GL_TRUE, static_cast<GLsizei>(instanceView.mStride), (void *)attribute.mOffset);
        glEnableVertexAttribArray(12);
        glVertexAttribDivisor(12, 1);
    }

    for (const IntrospectProgram::Attribute & attribute : aProgram.mAttributes)
    {
        if(auto found = aMesh.mStream.mAttributes.find(attribute.mSemantic);
           found != aMesh.mStream.mAttributes.end())
        {
            const VertexStream::Attribute & arrayBuffer = found->second;
            const graphics::ClientAttribute & client = found->second.mAttribute;
            const VertexStream::VertexBuffer & vertexBuffer = 
                aMesh.mStream.mVertexBuffers.at(arrayBuffer.mVertexBufferIndex);
            bind(vertexBuffer.mBuffer);
            // TODO handle integral program attributes
            // TODO some assertions regarding the number of components compatibility
            // TODO handle normalizing
            glVertexAttribPointer(attribute.mLocation, client.mDimension, client.mDataType, 
                                  GL_FALSE, static_cast<GLsizei>(vertexBuffer.mStride), (void *)client.mOffset);
            glEnableVertexAttribArray(attribute.mLocation);
        }
    }

    SELOG_LG(gRenderLogger, info)("Added a new VAO to the repository.");

    unbind(vertexArray);
    return vertexArray;
}

const graphics::VertexArrayObject &
VertexArrayRepository::get(const Mesh & aMesh,
                           const InstanceStream & aInstances,
                           const IntrospectProgram & aProgram)
{
    Key key = std::make_tuple(&aMesh, &aInstances, &static_cast<const graphics::Program &>(aProgram));
    if (auto found = mVAOs.find(key);
        found != mVAOs.end())
    {
        return found->second;
    }
    else
    {
        return mVAOs.emplace(key, prepareVAO(aMesh, aInstances, aProgram)).first->second;
    }
}



} // namespace snac
} // namespace ad