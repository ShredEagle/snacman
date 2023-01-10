#include "SetupDrawing.h"

#include "Logging.h"


namespace ad {
namespace snac {

namespace {

    /// @brief Attach vertex buffer data to a shader program parameter.
    ///
    /// Make several sanity checks regarding the data and attribute "compatibility"
    /// (dimensions, etc.)
    void attachAttribute(const IntrospectProgram::Attribute & aShaderAttribute,
                         const graphics::ClientAttribute & aClientAttribute,
                         const BufferView & aView,
                         GLuint aInstanceDivisor,
                         std::string_view aProgramName /* for log messages */)
    {
            // Consider secondary dimension mismatch (i.e., number of rows) a fatal error
            // Note: this might be too conservative, to be revised when a use case requires it.
            if (aClientAttribute.mDimension[1] != aShaderAttribute.dimension()[1])
            {
                SELOG_LG(gRenderLogger, critical)(
                    "{}: '{}' program attribute '{}'({}) of dimension {} is to be attached to vertex data of dimension {}. Row counts are not allowed to differ.",
                    __func__,
                    aProgramName,
                    aShaderAttribute.mName,
                    to_string(aShaderAttribute.mSemantic),
                    fmt::streamed(aShaderAttribute.dimension()),
                    fmt::streamed(aClientAttribute.mDimension)
                );
                throw std::logic_error{"Mismatching row counts between shader attribute and buffer data."};
            }
            else if (aClientAttribute.mDimension[0] != aShaderAttribute.dimension()[0])
            {
                SELOG_LG(gRenderLogger, warn)(
                    "{}: '{}' program attribute '{}'({}) of dimension {} is to be attached to vertex data of dimension {}.",
                    __func__,
                    aProgramName,
                    aShaderAttribute.mName,
                    to_string(aShaderAttribute.mSemantic),
                    fmt::streamed(aShaderAttribute.dimension()),
                    fmt::streamed(aClientAttribute.mDimension)
                );
            }

            bind(aView.mBuffer);
            graphics::attachBoundVertexBuffer(
                {aShaderAttribute.toShaderParameter(), aClientAttribute},
                aView.mStride,
                aInstanceDivisor);
    }

}; // anonymous


graphics::VertexArrayObject prepareVAO(const Mesh & aMesh,
                                       const InstanceStream & aInstances,
                                       const IntrospectProgram & aProgram)
{
    graphics::VertexArrayObject vertexArray;
    // TODO scoped bind (also remove the unbind)
    bind (vertexArray);

    for (const IntrospectProgram::Attribute & shaderAttribute : aProgram.mAttributes)
    {
        if(auto found = aMesh.mStream.mAttributes.find(shaderAttribute.mSemantic);
           found != aMesh.mStream.mAttributes.end())
        {
            const AttributeAccessor & accessor = found->second;
            const BufferView & bufferView = 
                aMesh.mStream.mVertexBuffers.at(accessor.mBufferViewIndex);
            attachAttribute(shaderAttribute, accessor.mAttribute, bufferView, 0, aProgram.name());
        }
        else if (auto found = aInstances.mAttributes.find(shaderAttribute.mSemantic);
                 found != aInstances.mAttributes.end())
        {
            const graphics::ClientAttribute & client = found->second;
            attachAttribute(shaderAttribute, client, aInstances.mInstanceBuffer, 1, aProgram.name());
        }
        else
        {
            SELOG_LG(gRenderLogger, warn)(
                "{}: Could not find an a vertex array buffer for semantic '{}' in program '{}'.", 
                __func__,
                to_string(shaderAttribute.mSemantic),
                aProgram.name());
        }
    }

    SELOG_LG(gRenderLogger, info)("Added a new VAO to the repository.");

    unbind(vertexArray);
    return vertexArray;
}


const graphics::VertexArrayObject &
VertexArrayRepository::get(const Mesh & aMesh, // Maybe it should just be the VertexStream?
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


void setUniforms(const UniformRepository & aUniforms, const IntrospectProgram & aProgram)
{
    for (const IntrospectProgram::Resource & shaderUniform : aProgram.mUniforms)
    {
        if(auto found = aUniforms.find(shaderUniform.mSemantic);
           found != aUniforms.end())
        {
            // We should actually support matrices,
            // it is the uniform arrays we do not want to support
            //if(shaderUniform.dimension()[1] != 1)
            //{
            //    SELOG_LG(gRenderLogger, critical)(
            //        "{}: '{}' program uniform '{}'({}) has dimension {}, matrices assignment is not implemented.",
            //        __func__,
            //        aProgramName,
            //        aUniformAttribute.mName,
            //        to_string(aShaderAttribute.mSemantic),
            //        fmt::streamed(aShaderAttribute.dimension()),
            //    );
            //    throw std::logic_error{"Uniform matrices assignment not implemented."};
            //}
            // TODO Select among the big amout of glUniform* functions ...
            // The selection is based on two criterias, the dimension and the component type
            const UniformParameter & parameter = found->second;
            glUniform3fv(shaderUniform.mLocation,
                         1,
                         std::get<const GLfloat *>(parameter.mData));
        }
        else
        {
            // TODO since this function is currently called before each draw
            // this is much to verbose for a warning...
            SELOG_LG(gRenderLogger, warn)(
                "{}: Could not find an a uniform value for semantic '{}' in program '{}'.", 
                __func__,
                to_string(shaderUniform.mSemantic),
                aProgram.name());
        }
    }
}


} // namespace snac
} // namespace ad