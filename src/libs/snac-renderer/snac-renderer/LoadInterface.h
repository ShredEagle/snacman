#pragma once


#include <platform/Filesystem.h>


namespace ad {
namespace snac {


template <class T_resource>
struct Load
{
    virtual T_resource get(const filesystem::path & aResourcePath) = 0;
};


} // namespace graphics
} // namespace ad