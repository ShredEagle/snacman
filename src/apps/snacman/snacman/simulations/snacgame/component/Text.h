#pragma once


#include <math/Color.h>

#include <string>


namespace ad {
namespace snacgame {
namespace component {


struct Text
{
    std::string mString;
    std::shared_ptr<snac::Font> mFont;
    math::hdr::Rgba_f mColor;
};


} // namespace component
} // namespace cubes
} // namespace ad

