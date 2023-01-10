#pragma once


#include "Semantic.h"

#include <renderer/AttributeDimension.h>
#include <renderer/GL_Loader.h>
#include <renderer/MappedGL.h>

#include <map>
#include <variant>


namespace ad {
namespace snac {


struct UniformParameter
{
    // Note: take pointers because const reference would allow temporaries.
    template <class T_derived, int N_rows, int N_cols, class T_component>
    UniformParameter(const math::MatrixBase<T_derived, N_rows, N_cols, T_component> * aValue) :
        mComponentType{graphics::MappedGL_v<T_component>},
        mDimension{N_cols, N_rows},
        mData{aValue->data()}
    {}

    GLenum mComponentType;
    graphics::AttributeDimension mDimension;
    std::variant<const GLint *, const GLuint *, const GLfloat *> mData;
};


using UniformRepository = std::map<Semantic, UniformParameter>;


} // namespace snac
} // namespace ad