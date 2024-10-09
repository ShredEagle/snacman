#pragma once

#include "snacman/serialization/Witness.h"

#include <snac-reflexion/Reflexion.h>
#include <snac-reflexion/Reflexion_impl.h>
#include <reflexion/NameValuePair.h>

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

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mString));
        aWitness.witness(NVP(mFont));
        aWitness.witness(NVP(mColor));
    }
};

REFLEXION_REGISTER(Text)


} // namespace component
} // namespace cubes
} // namespace ad

