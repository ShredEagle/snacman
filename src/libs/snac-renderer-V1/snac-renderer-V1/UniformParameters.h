#pragma once


#include "Semantic.h"
#include "StackRepository.h"

#include <renderer/AttributeDimension.h>
#include <renderer/GL_Loader.h>
#include <renderer/MappedGL.h>
#include <renderer/Uniforms.h>
#include <renderer/UniformBuffer.h>
#include <renderer/Texture.h>

#include <map>
#include <variant>


namespace ad {
namespace snac {


struct UniformParameter
{
    template <class T_derivedMatrix>
        requires math::from_matrix_v<T_derivedMatrix>
    UniformParameter(const T_derivedMatrix & aValue) :
        mComponentType{graphics::MappedGL_v<typename T_derivedMatrix::value_type>},
        // Note: math respects the classical i, j convention, but graphics
        // considers vectors to be row-vectors (so 1st dimension is the column count).
        mDimension{T_derivedMatrix::Cols, T_derivedMatrix::Rows},
        mSetter{[value = aValue](const graphics::Program & aProgram, GLint aLocation)
            {
                graphics::setUniform(aProgram, aLocation, value);
            }}
    {}

    
    /// \brief Constructor when the value is not dervied from a math::MatrixBase.
    ///
    /// \param aValue should be of a scalar type, for which there is an overload setUniform.
    template <class T>
        requires (!math::from_matrix_v<T>) // Fix issue where gcc 12 doesn't think that requires clause disambiguate template
    UniformParameter(T aValue) :
        mComponentType{graphics::MappedGL_v<T>},
        mDimension{1},
        mSetter{[value = aValue](const graphics::Program & aProgram, GLint aLocation)
            {
                graphics::setUniform(aProgram, aLocation, value);
            }}
    {
        // Safety (conservative) check: I am afraid of the case where overloads of setUniform 
        // would be made available for non-scalar types that do not derives from math::MatrixBase.
        static_assert(
            std::is_same_v<GLfloat, T> ||
            std::is_same_v<GLint, T>   ||
            std::is_same_v<GLuint, T>);
    }


    GLenum mComponentType;
    graphics::AttributeDimension mDimension;
    std::function<void(const graphics::Program & aProgram, GLint aLocation)> mSetter;
};


using UniformRepository = StackRepository<Semantic, UniformParameter>;

using UniformBlocks = StackRepository<BlockSemantic,
                                      std::variant<std::shared_ptr<graphics::UniformBufferObject>,
                                                                   graphics::UniformBufferObject *>>;

// TODO #text We should get rid of the std::variant, but the Text system causes a complication here
using TextureRepository = StackRepository<Semantic,
                                          std::variant<std::shared_ptr<graphics::Texture>,
                                                       graphics::Texture *>>;


} // namespace snac
} // namespace ad