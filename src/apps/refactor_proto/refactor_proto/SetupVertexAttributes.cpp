#include "SetupVertexAttributes.h"

#include "Logging.h"
#include "Model.h"

#include <renderer/ScopeGuards.h>

#include <fmt/ostream.h>


namespace ad::renderer {


namespace {


    /// @brief Attach vertex buffer data to a shader program parameter.
    ///
    /// Make several sanity checks regarding the data and attribute "compatibility"
    /// (dimensions, etc.)
    void attachAttribute(const IntrospectProgram::Attribute & aShaderAttribute,
                         const graphics::ClientAttribute & aClientAttribute,
                         const VertexBufferView & aView,
                         GLuint aInstanceDivisor,
                         std::string_view aProgramName /* for log messages */)
    {
            // Consider secondary dimension mismatch (i.e., number of rows) a fatal error
            // Note: this might be too conservative, to be revised when a use case requires it.
            if (aClientAttribute.mDimension[1] != aShaderAttribute.dimension()[1])
            {
                SELOG(critical)(
                    "{}: Program '{}' attribute '{}'({}) of dimension {} is to be attached to vertex data of dimension {}. Row counts are not allowed to differ.",
                    __func__,
                    aProgramName,
                    aShaderAttribute.mName,
                    to_string(aShaderAttribute.mSemantic),
                    fmt::streamed(aShaderAttribute.dimension()),
                    fmt::streamed(aClientAttribute.mDimension)
                );
                throw std::logic_error{"Mismatching row counts between shader attribute and buffer data."};
            }
            // We allow the primary dimension of the vertex attribute to mismatch the provided vertex data, but warn.
            else if (aClientAttribute.mDimension[0] != aShaderAttribute.dimension()[0])
            {
                SELOG(warn)(
                    "{}: Program '{}' attribute '{}'({}) of dimension {} is to be attached to vertex data of dimension {}.",
                    __func__,
                    aProgramName,
                    aShaderAttribute.mName,
                    to_string(aShaderAttribute.mSemantic),
                    fmt::streamed(aShaderAttribute.dimension()),
                    fmt::streamed(aClientAttribute.mDimension)
                );
            }

            graphics::ScopedBind scopeVertexBuffer{*aView.mGLBuffer};
            graphics::attachBoundVertexBuffer(
                {aShaderAttribute.toShaderParameter(), aClientAttribute},
                aView.mStride,
                aInstanceDivisor);
    }


} // unnamed namespace


graphics::VertexArrayObject prepareVAO(const VertexStream & aVertices,
                                       //const InstanceStream & aInstances,
                                       const IntrospectProgram & aProgram)
{
    graphics::VertexArrayObject vertexArray;
    graphics::ScopedBind scopedVao{vertexArray};

    if (const graphics::IndexBufferObject * ibo = aVertices.mIndexBufferView.mGLBuffer;
        ibo != nullptr)
    {
        // Now that the VAO is bound, it can store the index buffer binding.
        // Note that this binding is permanent, no need to scope it.
        graphics::bind(*ibo);
    }

    for (const IntrospectProgram::Attribute & shaderAttribute : aProgram.mAttributes)
    {
        if(auto found = aVertices.mSemanticToAttribute.find(shaderAttribute.mSemantic);
           found != aVertices.mSemanticToAttribute.end())
        {
            const AttributeAccessor & accessor = found->second;
            const VertexBufferView & bufferView = 
                aVertices.mBufferViews.at(accessor.mBufferViewIndex);
            attachAttribute(shaderAttribute, accessor.mClientDataFormat, bufferView, 0, aProgram.name());
        }
        //else if (auto found = aInstances.mAttributes.find(shaderAttribute.mSemantic);
        //         found != aInstances.mAttributes.end())
        //{
        //    const graphics::ClientAttribute & client = found->second;
        //    attachAttribute(shaderAttribute, client, aInstances.mInstanceBuffer, 1, aProgram.name());
        //}
        else
        {
            SELOG(warn)(
                "{}: Could not find an a vertex array buffer for semantic '{}' in program '{}'.", 
                __func__,
                to_string(shaderAttribute.mSemantic),
                aProgram.name());
        }
    }

    SELOG(info)("Configured a new VAO.");

    return vertexArray;
}


} // namespace ad::renderer