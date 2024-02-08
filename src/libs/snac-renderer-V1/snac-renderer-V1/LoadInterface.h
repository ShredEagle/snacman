#pragma once


#include <platform/Filesystem.h>


namespace ad {
namespace snac {


/// \brief From r-value (or l-value) to l-value. The inverse of std::move.
/// \warning This is inherently dangerous, but is useful for function parameters that need to be mutated,
/// yet are not required to live longer than the function invocation.
template <class T>
T & pass(T && aArg)
{
    return aArg;
}

template <class T_resource>
struct Load
{
    virtual T_resource get(const filesystem::path & aResourcePath) = 0;
};


} // namespace graphics
} // namespace ad