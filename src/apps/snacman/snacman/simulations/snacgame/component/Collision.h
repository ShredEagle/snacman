#pragma once

#include <snacman/serialization/Serial.h>
#include <reflexion/NameValuePair.h>

#include <math/Homogeneous.h>
#include <math/Transformations.h>
#include <math/Vector.h>
#include <math/Box.h>

namespace ad {
namespace snacgame {
namespace component {

struct Collision
{
    math::Box<float> mHitbox;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mHitbox));
    }
};

REFLEXION_REGISTER(Collision)

constexpr const math::Box<float> gPlayerHitbox{{-0.35f, 0.f, -0.35f},
                                               {0.7f, 0.7f, 0.7f}};

constexpr const math::Box<float> gPillHitbox{{-0.2f, -0.2f, -0.2f},
                                               {0.4f, 0.4f, 0.4f}};

constexpr const math::Box<float> gPowerUpHitbox{{-0.35f, -0.35f, -0.35f},
                                               {0.7f, 0.7f, 0.7f}};

constexpr const math::Box<float> gBurgerLossHitbox{{-0.1f, -0.1f, -0.1f},
                                               {0.2f, 0.2f, 0.2f}};

inline math::Box<float> transformHitbox(const math::Position<3, float> & aPos, const math::Box<float> & aBox)
{
    const math::Vec<3, float> & worldPos = aPos.as<math::Vec>();
    math::Box<float> playerHitbox = aBox;
    math::Position<4, float> transformedPos =
        math::homogeneous::makePosition(playerHitbox.mPosition)
        * math::trans3d::translate(worldPos);
    return {
        .mPosition = transformedPos.xyz(),
        .mDimension = aBox.dimension(),
    };
}

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
