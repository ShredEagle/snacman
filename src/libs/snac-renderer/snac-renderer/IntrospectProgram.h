#pragma once


#include "Semantic.h"
#include "ShaderResource.h"

#include <renderer/Shading.h>
#include <renderer/VertexSpecification.h>

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

    struct Attribute
    {
        GLuint mLocation;
        GLenum mType;
        Semantic mSemantic;
        std::string mName; // To ease debugger inspection, we only need the semantic

        graphics::AttributeDimension dimension() const
        { return getResourceDimension(mType); }

        graphics::ShaderParameter toShaderParameter() const;
    };

    graphics::Program mProgram;
    std::vector<Attribute> mAttributes;
    std::string mName;
};


} // namespace snac
} // namespace ad