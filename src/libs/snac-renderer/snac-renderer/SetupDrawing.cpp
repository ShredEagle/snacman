#include "SetupDrawing.h"

#include "Logging.h"


namespace ad {
namespace snac {

graphics::VertexArrayObject prepareVAO(const Mesh & aMesh,
                                       const InstanceStream & aInstances,
                                       const graphics::Program & aProgram)
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

    const VertexStream::VertexBuffer & view = aMesh.mStream.mVertexBuffers.front();
    //graphics::ScopedBind boundVBO{aMesh.mStream.mVertexBuffers.front()};
    bind(view.mBuffer);
    {
        const graphics::ClientAttribute & attribute =
            aMesh.mStream.mAttributes.at(Semantic::Position).mAttribute;
        glVertexAttribPointer(0, attribute.mDimension, attribute.mDataType, 
                                GL_FALSE, static_cast<GLsizei>(view.mStride), (void *)attribute.mOffset);
        glEnableVertexAttribArray(0);
    }
    {
        const graphics::ClientAttribute & attribute =
            aMesh.mStream.mAttributes.at(Semantic::Normal).mAttribute;
        glVertexAttribPointer(1, attribute.mDimension, attribute.mDataType, 
                                GL_FALSE, static_cast<GLsizei>(view.mStride), (void *)attribute.mOffset);
        glEnableVertexAttribArray(1);
    }

    SELOG_LG(gRenderLogger, info)("Added a new VAO to the repository.");

    unbind(vertexArray);
    return vertexArray;
}

const graphics::VertexArrayObject &
VertexArrayRepository::get(const Mesh & aMesh,
                           const InstanceStream & aInstances,
                           const graphics::Program & aProgram)
{
    Key key = std::make_tuple(&aMesh, &aInstances, &aProgram);
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