#include "Explosion.h"
#include "../typedef.h"
#include "snacman/simulations/snacgame/component/Explosion.h"
#include "snacman/simulations/snacgame/component/Geometry.h"

namespace ad {
namespace snacgame {
namespace system {
void Explosion::update(const snac::Time & aTime)
{
    Phase removeExplosion;
    mExplosions.each([&aTime, &removeExplosion](EntHandle aHandle, component::Explosion & aExplosion, component::Geometry & aGeometry)
    {
        float duration = (float)snac::asSeconds(aTime.mTimepoint - aExplosion.mStartTime);

        if (aExplosion.mParameter.isCompleted(duration))
        {
            aHandle.get(removeExplosion)->erase();
        }
        else
        {
            float scale = aExplosion.mParameter.at(duration);

            aGeometry.mScaling = scale * 2.f;
        }
    });
}
} // namespace system
} // namespace snacgame
} // namespace ad
