#pragma once


#include "Mesh.h"

#include <renderer/GL_Loader.h>
#include <renderer/Shading.h>

#include <string>
#include <vector>


namespace ad {
namespace snac {


struct IntrospectProgram
{
    /*implicit*/ IntrospectProgram(graphics::Program aProgram);

    /*implicit*/ operator graphics::Program &()
    { return mProgram; }

    /*implicit*/ operator const graphics::Program &() const
    { return mProgram; }

    /*implicit*/ operator GLuint() const
    { return mProgram; }

    struct Attribute
    {
        GLint mLocation;
        GLenum mType;
        Semantic mSemantic;
        std::string mName; // To ease debugger inspection, we only need the semantic
    };

    graphics::Program mProgram;
    std::vector<Attribute> mAttributes;
};


} // namespace snac
} // namespace ad