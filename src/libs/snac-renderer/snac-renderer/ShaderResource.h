#pragma once


#include <renderer/AttributeDimension.h>
#include <renderer/GL_Loader.h>
#include <renderer/VertexSpecification.h>


namespace ad {
namespace snac {

// TODO move to a more general library

// Provide analysis for the shader program resource types, as queried with
// glGetProgramResourceiv().
// For the types in the enumeration, see OpenGL 4.6 core specification,
// table 7.3  p 116.

graphics::AttributeDimension getResourceDimension(GLenum aType);

graphics::ShaderParameter::Access getResourceShaderAccess(GLenum aType);


} // namespace graphics
} // namespace ad