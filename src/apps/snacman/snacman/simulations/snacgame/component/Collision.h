#include "math/Box.h"

namespace ad {
namespace snacgame {
namespace component {

struct Collision
{
    math::Box<float> mHitbox;
};

constexpr const math::Box<float> gPlayerHitbox{{-1.f, 0.f, -1.f},
                                               {2.f, 4.f, 2.f}};

constexpr const math::Box<float> gPillHitbox{{-0.2f, 0.f, -0.2f},
                                               {0.4f, 0.8f, 0.4f}};
} // namespace component
} // namespace snacgame
} // namespace ad
