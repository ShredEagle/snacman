#pragma once


#include <math/Color.h>

#include <array>


namespace ad::renderer {


namespace sdr {

    //see: https://colorbrewer2.org/#type=qualitative&scheme=Set1&n=9
    constexpr std::array<math::sdr::Rgb, 9> gColorBrewerSet1 {{
        {232,  50,  51},
        {105, 165, 210},
        { 99, 188,  97},
        {174, 106, 184},
        {255, 140,  26},
        {255, 255,  61},
        {217, 141,  98},
        {246, 119, 186},
        {168, 160, 149}
    }};

} // namespace sdr


namespace hdr {

    constexpr std::array<math::hdr::Rgb_f, 9> gColorBrewerSet1 {{
        {0.910f, 0.196f, 0.200f},
        {0.412f, 0.647f, 0.824f},
        {0.388f, 0.737f, 0.380f},
        {0.682f, 0.416f, 0.722f},
        {1.000f, 0.549f, 0.102f},
        {1.000f, 1.000f, 0.239f},
        {0.851f, 0.553f, 0.384f},
        {0.965f, 0.467f, 0.729f},
        {0.659f, 0.627f, 0.584f},
    }};

} // namespace hdr

} // namespace ad::renderer