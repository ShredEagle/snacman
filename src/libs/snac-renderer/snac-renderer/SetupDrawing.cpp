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
            // TODO some assertions regarding the number of components compatibility
            graphics::attachBoundVertexBuffer(
                {attribute.toShaderParameter(), client},
                vertexBuffer.mStride);
        }
        else if (auto found = aInstances.mAttributes.find(attribute.mSemantic);
                 found != aInstances.mAttributes.end())
        {
            bind(aInstances.mInstanceBuffer.mBuffer);
            const graphics::ClientAttribute & client = found->second;
            graphics::attachBoundVertexBuffer(
                {attribute.toShaderParameter(), client},
                aInstances.mInstanceBuffer.mStride,
                1);
        }
        else
        {
            SELOG_LG(gRenderLogger, warn)(
                "{}: Could not find an a vertex array buffer for semantic '{}' in program '{}'.", 
                __func__,
                to_string(attribute.mSemantic),
                aProgram.name());
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