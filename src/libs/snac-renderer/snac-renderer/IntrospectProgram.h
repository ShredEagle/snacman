#pragma once


#include "Semantic.h"
#include "ShaderResource.h"

#include <renderer/Shading.h>
#include <renderer/VertexSpecification.h>
#include <renderer/BufferIndexedBinding.h>

#include <string>
#include <vector>


namespace ad {
namespace snac {


struct IntrospectProgram
{
    /*implicit*/ IntrospectProgram(graphics::Program aProgram, std::string aName);

    /*implicit*/ operator graphics::Program &()
    { return mProgram; }

    /*implicit*/ operator const graphics::Program &() const
    { return mProgram; }

    /*implicit*/ operator GLuint() const
    { return mProgram; }

    std::string_view name() const
    { return mName; }

    struct Resource
    {
        GLint mLocation; // keep it signed, so it can hold -1 for uniforms from uniform blocks.
        GLenum mType;
        Semantic mSemantic;
        GLuint mArraySize;
        std::string mName; // To ease debugger inspection, we only need the semantic

        graphics::AttributeDimension dimension() const
        { return getResourceDimension(mType); }

        GLenum componentType() const
        { return getResourceComponentType(mType); }
    };

    struct Attribute : public Resource
    {
        graphics::ShaderParameter toShaderParameter() const;
    };

    struct UniformBlock
    {
        GLuint mBlockIndex;
        graphics::BindingIndex mBindingIndex;
        BlockSemantic mSemantic;
        std::string mName;
        // TODO do we want to use another type that does not have a location data member?
        std::vector<Resource> mUniforms;
    };

    graphics::Program mProgram;
    std::vector<Attribute> mAttributes;
    std::vector<Resource> mUniforms;
    std::vector<UniformBlock> mUniformBlocks;
    std::string mName;
};


std::ostream & operator<<(std::ostream & aOut, const IntrospectProgram & aProgram);
std::ostream & operator<<(std::ostream & aOut, const IntrospectProgram::Resource & aResource);
std::ostream & operator<<(std::ostream & aOut, const IntrospectProgram::UniformBlock & aBlock);


} // namespace snac
} // namespace ad