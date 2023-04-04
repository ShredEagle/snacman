#pragma once

#include <math/Box.h>

namespace ad {
namespace snacgame {
namespace component {

struct Collision
{
    math::Box<float> mHitbox;
};

constexpr const math::Box<float> gPlayerHitbox{{-0.5f, -0.5f, 0.f},
                                               {1.f, 1.f, 1.f}};

constexpr const math::Box<float> gPillHitbox{{-0.2f, -0.2f, 0.0f},
                                               {0.4f, 0.4f, 0.8f}};

constexpr const math::Box<float> gPowerUpHitbox{{-0.4f, -0.4f, 0.0f},
                                               {0.8f, 0.8f, 1.6f}};

inline bool collideWithSat(const math::Box<float> & aLhs, const math::Box<float> & aRhs)
{
    return aLhs.xMin() <= aRhs.xMax()
        && aLhs.xMax() >= aRhs.xMin()
        && aLhs.yMin() <= aRhs.yMax()
        && aLhs.yMax() >= aRhs.yMin()
        && aLhs.zMin() <= aRhs.zMax()
        && aLhs.zMax() >= aRhs.zMin();
}
} // namespace component
} // namespace snacgame
} // namespace ad
