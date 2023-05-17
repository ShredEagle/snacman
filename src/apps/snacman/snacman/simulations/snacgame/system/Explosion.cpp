#include "Explosion.h"
#include "../typedef.h"
#include "snacman/simulations/snacgame/component/Explosion.h"
#include "snacman/simulations/snacgame/component/Geometry.h"

namespace ad {
namespace snacgame {
namespace system {
void Explosion::update(const snac::Time & aTime)
{
    mExplosions.each([&aTime](EntHandle aHandle, component::Explosion & aExplosion, component::Geometry & aGeometry)
    {
        float duration = snac::asSeconds(aTime.mTimepoint - aExplosion.mStartTime);

        if (aExplosion.mParameter.isCompleted(duration))
        {
            Phase removeExplosion;
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
