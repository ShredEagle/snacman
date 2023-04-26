#include "SetupDrawing.h"

#include "Logging.h"

#include <fmt/ostream.h>
#include <spdlog/spdlog.h>


#define SELOG(level) SELOG_LG(::ad::snac::gRenderLogger, level)


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
                SELOG(critical)(
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
                SELOG(warn)(
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


graphics::VertexArrayObject prepareVAO(const VertexStream & aVertices,
                                       const InstanceStream & aInstances,
                                       const IntrospectProgram & aProgram)
{
    graphics::VertexArrayObject vertexArray;
    // I think the IBO must still be bound at the moment the VAO is unbound,
    // so the scope guard for IBO must outlive the scope guard for VAO.
    // YET, the VAO must already be bound when the IBO is bound in order to store this binding.
    // see: https://stackoverflow.com/a/72760508/1027706
    std::optional<graphics::ScopedBind> optionalScopedIbo;
    graphics::ScopedBind scopedVao{vertexArray};

    if (aVertices.mIndices)
    {
        // Now that the VAO is bound, it can store the index buffer binding.
        optionalScopedIbo = graphics::ScopedBind{aVertices.mIndices->mIndexBuffer};
    }

    for (const IntrospectProgram::Attribute & shaderAttribute : aProgram.mAttributes)
    {
        if(auto found = aVertices.mAttributes.find(shaderAttribute.mSemantic);
           found != aVertices.mAttributes.end())
        {
            const AttributeAccessor & accessor = found->second;
            const BufferView & bufferView = 
                aVertices.mVertexBuffers.at(accessor.mBufferViewIndex);
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
            SELOG(warn)(
                "{}: Could not find an a vertex array buffer for semantic '{}' in program '{}'.", 
                __func__,
                to_string(shaderAttribute.mSemantic),
                aProgram.name());
        }
    }

    SELOG(info)("Added a new VAO to the repository.");

    return vertexArray;
}


const graphics::VertexArrayObject &
VertexArrayRepository::get(const VertexStream & aVertices,
                           const InstanceStream & aInstances,
                           const IntrospectProgram & aProgram)
{
    Key key = std::make_tuple(&aVertices, &aInstances, &static_cast<const graphics::Program &>(aProgram));
    if (auto found = mVAOs.find(key);
        found != mVAOs.end())
    {
        return found->second;
    }
    else
    {
        return mVAOs.emplace(key, prepareVAO(aVertices, aInstances, aProgram)).first->second;
    }
}


WarningRepository::WarnedUniforms &
WarningRepository::get(std::string_view aPassName, const IntrospectProgram & aProgram)
{
    Key key = std::make_pair(handy::StringId{aPassName}, &static_cast<const graphics::Program &>(aProgram));
    auto [iterator, didInsert] = mWarnings.try_emplace(key);
    if (didInsert)
    {
        SELOG(debug)("Added a new warning set to the repository, for key (pass:'{}', program:'{}').",
                     aPassName, aProgram.name());
    }
    return iterator->second;
}


void setUniforms(const UniformRepository & aUniforms, 
                 const IntrospectProgram & aProgram,
                 WarningRepository::WarnedUniforms & aWarnedUniforms)
{
    for (const IntrospectProgram::Resource & shaderUniform : aProgram.mUniforms)
    {
        if (!isResourceSamplerType(shaderUniform.mType))
        {
            if(auto found = aUniforms.find(shaderUniform.mSemantic);
               found != aUniforms.end())
            {
                if (shaderUniform.mArraySize != 1)
                {
                    SELOG(error)(
                        "{}: '{}' program uniform '{}'({}) is an array of size {}, setting uniform arrays is not supported.",
                        __func__,
                        aProgram.name(),
                        shaderUniform.mName,
                        to_string(shaderUniform.mSemantic),
                        shaderUniform.mArraySize
                    );
                }

                const UniformParameter & parameter = found->second;
                // Very conservative assertions, this might be relaxed when legitimate use cases "show-up".
                assert(parameter.mComponentType == shaderUniform.componentType());
                assert(parameter.mDimension == shaderUniform.dimension());
                parameter.mSetter(aProgram, shaderUniform.mLocation);
            }
            else
            {
                const std::string uniformString = to_string(shaderUniform.mSemantic);
                if(auto [iterator, didInsert] = aWarnedUniforms.insert(uniformString);
                   didInsert)
                {
                    SELOG(warn)(
                        "{}: Could not find an a uniform value for semantic '{}' in program '{}'.", 
                        __func__,
                        uniformString,
                        aProgram.name());
                }
            }
        }
    }
}


void setTextures(const TextureRepository & aTextures,
                 const IntrospectProgram & aProgram,
                 WarningRepository::WarnedUniforms & aWarnedUniforms)
{
    GLint textureImageUnit = 0;
    for (const IntrospectProgram::Resource & shaderUniform : aProgram.mUniforms)
    {
        if (isResourceSamplerType(shaderUniform.mType))
        {
            if(auto found = aTextures.find(shaderUniform.mSemantic);
               found != aTextures.end())
            {
                if (shaderUniform.mArraySize != 1)
                {
                    SELOG(error)(
                        "{}: '{}' program uniform '{}'({}) is an array of size {}, setting uniform arrays is not supported.",
                        __func__,
                        aProgram.name(),
                        shaderUniform.mName,
                        to_string(shaderUniform.mSemantic),
                        shaderUniform.mArraySize
                    );
                }

                const graphics::Texture & texture = std::visit(
                    [](auto && texture) -> const graphics::Texture &
                    {
                        return *texture;
                    },
                    found->second);
                // TODO assertions regarding texture-sampler compatibility
                auto guardImageUnit = graphics::activateTextureUnitGuard(textureImageUnit);
                bind(texture); // should not be unbound when we exit the current scope.
                graphics::setUniform(aProgram, shaderUniform.mLocation, textureImageUnit);
                ++textureImageUnit;
            }
            else
            {
                // TODO since this function is currently called before each draw
                // this is much to verbose for a warning...
                const std::string textureString = to_string(shaderUniform.mSemantic);
                if(auto [iterator, didInsert] = aWarnedUniforms.insert(textureString);
                   didInsert)
                {
                // This is likely an error: the sampler being available means it is probably used.
                SELOG(error)(
                    "{}: Could not find an a texture for semantic '{}' in program '{}'.", 
                    __func__,
                    textureString,
                    aProgram.name());
                }
            }
        }
    }
}


void setBlocks(const UniformBlocks & aUniformBlocks, const IntrospectProgram & aProgram)
{
    for (const IntrospectProgram::UniformBlock & shaderBlock : aProgram.mUniformBlocks)
    {
        if(auto found = aUniformBlocks.find(shaderBlock.mSemantic);
           found != aUniformBlocks.end())
        {
            const graphics::UniformBufferObject & uniformBuffer = std::visit(
                [](auto && ubo) -> const graphics::UniformBufferObject &
                {
                    return *ubo;
                },
                found->second);
            // Currently, checking if the uniform block is an array is conducted
            // when getting its semantic.
            bind(uniformBuffer, shaderBlock.mBindingIndex);
        }
        else
        {
            // TODO since this function is currently called before each draw
            // this is much to verbose for a warning...
            SELOG(warn)(
                "{}: Could not find an a block uniform for block semantic '{}' in program '{}'.", 
                __func__,
                to_string(shaderBlock.mSemantic),
                aProgram.name());
        }
    }
}


} // namespace snac
} // namespace ad
