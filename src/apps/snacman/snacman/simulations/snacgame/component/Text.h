#pragma once

#include <snacman/detail/Reflexion.h>
#include <snacman/detail/Reflexion_impl.h>
#include <snacman/detail/Serialization.h>

#include <snac-renderer-V1/text/Text.h>

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

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mString);
        archive & SERIAL_PARAM(mFont);
        archive & SERIAL_PARAM(mColor);
    }
};

SNAC_SERIAL_REGISTER(Text)


} // namespace component
} // namespace cubes
} // namespace ad

