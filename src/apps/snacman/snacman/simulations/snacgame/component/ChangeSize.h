#pragma once


#include "math/Interpolation/ParameterAnimation.h"

#include "../GameParameters.h"
#include "../InputCommandConverter.h"
namespace ad {
namespace snacgame {
namespace component {

struct ChangeSize
{
    ParameterAnimation<float, math::FullRange, math::None, math::ease::Bezier> mCurve;

    void drawUi();
};

} // namespace component
} // namespace snacgame
} // namespace ad
