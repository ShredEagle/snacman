#include "SetupDrawing.h"

#include "Logging.h"
#include "Model.h"

#include <renderer/Uniforms.h>
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
                         const BufferView & aVertexBufferView,
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

            graphics::ScopedBind scopeVertexBuffer{*aVertexBufferView.mGLBuffer, graphics::BufferType::Array};
            graphics::attachBoundVertexBuffer(
                {aShaderAttribute.toShaderParameter(), aClientAttribute},
                aVertexBufferView.mStride,
                aVertexBufferView.mInstanceDivisor);
    }


} // unnamed namespace


graphics::VertexArrayObject prepareVAO(const IntrospectProgram & aProgram,
                                       const VertexStream & aVertices)
{
    graphics::VertexArrayObject vertexArray;
    graphics::ScopedBind scopedVao{vertexArray};

    if (const graphics::BufferAny * ibo = aVertices.mIndexBufferView.mGLBuffer;
        ibo != nullptr)
    {
        // Now that the VAO is bound, it can store the index buffer binding.
        // Note that this binding is permanent, no need to scope it.
        graphics::bind(*ibo, graphics::BufferType::ElementArray);
    }

    for (const IntrospectProgram::Attribute & shaderAttribute : aProgram.mAttributes)
    {
        if(auto found = aVertices.mSemanticToAttribute.find(shaderAttribute.mSemantic);
           found != aVertices.mSemanticToAttribute.end())
        {
            const AttributeAccessor & accessor = found->second;
            const BufferView & vertexBufferView = 
                aVertices.mVertexBufferViews.at(accessor.mBufferViewIndex);
            // Assertion below is not true anymore, since we merge everything into the VertexStream
            //assert(vertexBufferView.mInstanceDivisor == 0); // should always be 0 for the VertexStream as far as I imagine
            attachAttribute(shaderAttribute,
                            accessor.mClientDataFormat,
                            vertexBufferView,
                            aProgram.name());
        }
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


void setBufferBackedBlocks(const IntrospectProgram & aProgram,
                           const RepositoryUbo & aUniformBufferObjects)
{
    for (const IntrospectProgram::UniformBlock & shaderBlock : aProgram.mUniformBlocks)
    {
        if(auto found = aUniformBufferObjects.find(shaderBlock.mSemantic);
           found != aUniformBufferObjects.end())
        {
            bind(*found->second, shaderBlock.mBindingIndex);
        }
        else
        {
            // TODO since this function is currently called before each draw
            // this is much too verbose for a warning...
            SELOG(warn)(
                "{}: Could not find an a block uniform for block semantic '{}' in program '{}'.", 
                __func__,
                to_string(shaderBlock.mSemantic),
                aProgram.name());
        }
    }
}


void setTextures(const IntrospectProgram & aProgram,
                 const RepositoryTexture & aTextures)
{
    GLint textureImageUnit = 0;
    for (const IntrospectProgram::Resource & shaderUniform : aProgram.mUniforms)
    {
        if (graphics::isResourceSamplerType(shaderUniform.mType))
        {
            if(auto found = aTextures.find(shaderUniform.mSemantic);
               found != aTextures.end())
            {
                if (shaderUniform.mArraySize != 1)
                {
                    SELOG(error)(
                        "{}: '{}' program uniform '{}'({}) is a sampler array of size {}, setting uniform arrays is not supported.",
                        __func__,
                        aProgram.name(),
                        shaderUniform.mName,
                        to_string(shaderUniform.mSemantic),
                        shaderUniform.mArraySize
                    );
                }

                // TODO assertions regarding texture-sampler compatibility
                auto guardImageUnit = graphics::activateTextureUnitGuard(textureImageUnit);
                bind(*found->second); // should not be unbound when we exit the current scope.
                graphics::setUniform(aProgram, shaderUniform.mLocation, textureImageUnit);
                ++textureImageUnit;
            }
            else
            {
                // TODO since this function is currently called before each draw
                // this is much to verbose for a warning...
                // In the case of samplers thought this is likely an error:
                // the sampler being available means it is probably used.
                SELOG(error)(
                    "{}: Could not find an a texture for semantic '{}' in program '{}'.", 
                    __func__,
                    to_string(shaderUniform.mSemantic),
                    aProgram.name());
            }
        }
    }
}


} // namespace ad::renderer