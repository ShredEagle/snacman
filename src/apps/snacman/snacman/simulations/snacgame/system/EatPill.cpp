#include "EatPill.h"

#include <math/Transformations.h>

namespace ad {
namespace snacgame {
namespace system {
void EatPill::update()
{
    mPlayers.each(
        [&](component::Geometry & aPlayerGeo, component::PlayerSlot, component::Collision aPlayerCol) {
            auto playerHitbox = aPlayerCol.mHitbox;
            mPills.each([&](component::Geometry, component::Collision aPillCol) {
                auto pillHitbox = aPillCol.mHitbox;

                if (
                        pillHitbox.xMin() <= playerHitbox.xMax() &&
                        pillHitbox.xMax() >= playerHitbox.xMin() &&
                        pillHitbox.yMin() <= playerHitbox.yMax() &&
                        pillHitbox.yMax() >= playerHitbox.yMin() &&
                        pillHitbox.zMin() <= playerHitbox.zMax() &&
                        pillHitbox.zMax() >= playerHitbox.zMin() 
                   )
                {
                    //Player eats the pill
                }
            });
        });
}
} // namespace system
} // namespace snacgame
} // namespace ad
