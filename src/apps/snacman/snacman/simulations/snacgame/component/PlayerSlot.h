#pragma once

#include "math/Color.h"

namespace ad {
namespace snacgame {
namespace component {

struct PlayerSlot
{
    int mIndex;
    bool mFilled;
    math::hdr::Rgba_f mColor;
};

}
} // namespace snacgame
} // namespace ad
