#include "ShaderResource.h"

#include <stdexcept>
#include <string>


namespace ad {
namespace snac {


graphics::ShaderParameter::Access getResourceShaderAccess(GLenum aType)
{
    switch(aType)
    {
        case GL_FLOAT:
        case GL_FLOAT_VEC2:
        case GL_FLOAT_VEC3:
        case GL_FLOAT_VEC4:
        case GL_FLOAT_MAT2:
        case GL_FLOAT_MAT3:
        case GL_FLOAT_MAT4:
        case GL_FLOAT_MAT2x3:
        case GL_FLOAT_MAT2x4:
        case GL_FLOAT_MAT3x2:
        case GL_FLOAT_MAT3x4:
        case GL_FLOAT_MAT4x2:
        case GL_FLOAT_MAT4x3:
        {
            return graphics::ShaderParameter::Access::Float;
        }

        case GL_DOUBLE:
        case GL_DOUBLE_VEC2:
        case GL_DOUBLE_VEC3:
        case GL_DOUBLE_VEC4:
        case GL_DOUBLE_MAT2:
        case GL_DOUBLE_MAT3:
        case GL_DOUBLE_MAT4:
        case GL_DOUBLE_MAT2x3:
        case GL_DOUBLE_MAT2x4:
        case GL_DOUBLE_MAT3x2:
        case GL_DOUBLE_MAT3x4:
        case GL_DOUBLE_MAT4x2:
        case GL_DOUBLE_MAT4x3:
        {
            throw std::logic_error{
                std::string{__func__} + ": Double parameters are not handled at the moment."};
        }

        case GL_INT:
        case GL_INT_VEC2:
        case GL_INT_VEC3:
        case GL_INT_VEC4:

        case GL_UNSIGNED_INT:
        case GL_UNSIGNED_INT_VEC2:
        case GL_UNSIGNED_INT_VEC3:
        case GL_UNSIGNED_INT_VEC4:
        {
            return graphics::ShaderParameter::Access::Integer;
        }

        default:
        {
            throw std::logic_error{
                std::string{__func__} + ": Unhandled enumerator with value: " 
                + std::to_string(aType) + "."};
        }
    }
}


graphics::AttributeDimension getResourceDimension(GLenum aType)
{
    switch(aType)
    {
        case GL_FLOAT:
        case GL_DOUBLE:
        case GL_INT:
        case GL_UNSIGNED_INT:
            return {1, 1};

        case GL_FLOAT_VEC2:
        case GL_DOUBLE_VEC2:
        case GL_INT_VEC2:
        case GL_UNSIGNED_INT_VEC2:
            return {2, 1};

        case GL_FLOAT_VEC3:
        case GL_DOUBLE_VEC3:
        case GL_INT_VEC3:
        case GL_UNSIGNED_INT_VEC3:
            return {3, 1};

        case GL_FLOAT_VEC4:
        case GL_DOUBLE_VEC4:
        case GL_INT_VEC4:
        case GL_UNSIGNED_INT_VEC4:
            return {4, 1};

        case GL_FLOAT_MAT2:
        case GL_DOUBLE_MAT2:
            return {2, 2};

        case GL_FLOAT_MAT3:
        case GL_DOUBLE_MAT3:
            return {3, 3};

        case GL_FLOAT_MAT4:
        case GL_DOUBLE_MAT4:
            return {4, 4};

        // In GL, matNxM has N column and M rows.
        // Following our convention (row major), 
        // the number of colums (i.e. the number of elements in a row) is the first dimension.
        case GL_FLOAT_MAT2x3:
        case GL_DOUBLE_MAT2x3:
            return {2, 3};

        case GL_FLOAT_MAT2x4:
        case GL_DOUBLE_MAT2x4:
            return {2, 4};

        case GL_FLOAT_MAT3x2:
        case GL_DOUBLE_MAT3x2:
            return {3, 2};

        case GL_FLOAT_MAT3x4:
        case GL_DOUBLE_MAT3x4:
            return {3, 4};

        case GL_FLOAT_MAT4x2:
        case GL_DOUBLE_MAT4x2:
            return {4, 2};

        case GL_FLOAT_MAT4x3:
            return {4, 3};

        default:
        {
            throw std::logic_error{
                std::string{__func__} + ": Unhandled enumerator with value: " 
                + std::to_string(aType) + "."};
        }
    }
}


} // namespace graphics
} // namespace ad