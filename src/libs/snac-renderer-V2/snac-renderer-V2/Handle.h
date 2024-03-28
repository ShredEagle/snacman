#pragma once


namespace ad::renderer {


// TODO Ad 2023/08/04: #handle Define a Handle wrapper which makes sense.
// Note that a central question will be whether we have a "global" storage 
// (thus handle can be dereferenced without a storage param / without storing a pointer to storage) or not?
// Also, for most OpenGL objects, the GL name is already a good handle.
template <class T>
using Handle = T *;

static constexpr auto gNullHandle = nullptr;


} // namespace ad::renderer