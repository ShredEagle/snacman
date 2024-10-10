#pragma once


#include <math/Color.h>

#include <array>


namespace ad::renderer {


namespace sdr {

    //see: https://colorbrewer2.org/#type=qualitative&scheme=Set1&n=9
    // In sRGB color space
    constexpr std::array<math::sdr::Rgb, 9> gColorBrewerSet1_srgb {{
        {228,  26,  28},
        { 55, 126, 184},
        { 77, 175,  74},
        {152,  78, 163},
        {255, 127,   0},
        {255, 255,  51},
        {166,  86,  40},
        {247, 129, 191},
        {153, 153, 153},
    }};

} // namespace sdr


namespace hdr {

    // In sRGB color space
    constexpr std::array<math::hdr::Rgb_f, 9> gColorBrewerSet1_srgb {{
        {0.894f, 0.102f, 0.110f},
        {0.216f, 0.494f, 0.722f},
        {0.302f, 0.686f, 0.290f},
        {0.596f, 0.306f, 0.639f},
        {1.000f, 0.498f, 0.000f},
        {1.000f, 1.000f, 0.200f},
        {0.651f, 0.337f, 0.157f},
        {0.969f, 0.506f, 0.749f},
        {0.600f, 0.600f, 0.600f},
    }};

    // Until we have C++26 for constexpr cmath, this is not constexpr
//    constexpr std::array<math::hdr::Rgb_f, 9> gColorBrewerSet1_linear {
//        []
//        {
//#if defined(_MSC_VER) && !defined(__clang__)
//            // I suspect a compiler bug with MSVC compiler, with message:
//            // > failure was caused by a read of an uninitialized symbol
//            // on MatrixBase default ctor calling setZero().
//            std::array<math::hdr::Rgb_f, 9> transformed{gColorBrewerSet1_srgb};
//#else
//            std::array<math::hdr::Rgb_f, 9> transformed;
//#endif
//            std::transform(gColorBrewerSet1_srgb.begin(), gColorBrewerSet1_srgb.end(),
//                           transformed.begin(),
//                           math::decode_sRGB<float>);
//            return transformed;
//        }()
//    };

    // Values computed offline from the above table, and copied back here.
    constexpr std::array<math::hdr::Rgb_f, 9> gColorBrewerSet1_linear {{
        {0.776f, 0.010f, 0.012f},
        {0.038f, 0.209f, 0.479f},
        {0.074f, 0.429f, 0.068f},
        {0.314f, 0.076f, 0.366f},
        {1.000f, 0.212f, 0.000f},
        {1.000f, 1.000f, 0.033f},
        {0.381f, 0.093f, 0.021f},
        {0.930f, 0.220f, 0.521f},
        {0.319f, 0.319f, 0.319f},
    }};

} // namespace hdr

} // namespace ad::renderer