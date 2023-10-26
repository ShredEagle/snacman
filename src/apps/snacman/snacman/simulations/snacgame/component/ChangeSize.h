#pragma once


#include "../GameParameters.h"
#include "../InputCommandConverter.h"

#include <snacman/detail/Easer.h>

#include <math/Interpolation/ParameterAnimation.h>

namespace ad {
namespace snacgame {
namespace component {

struct ChangeSize
{
    ParameterAnimation<float, math::FullRange, math::None, snac::detail::Bezier> mCurve;

    void drawUi();
};

} // namespace component
} // namespace snacgame
} // namespace ad
