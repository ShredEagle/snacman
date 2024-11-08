#include "LightRealisticWeatherSystem.h"

#include "../GameContext.h"

#include <math/Transformations.h>


namespace ad::snacgame::system {


LightRealisticWeatherSystem::LightRealisticWeatherSystem(GameContext & aGameContext) :
    mLights{aGameContext.mWorld}
{}


void LightRealisticWeatherSystem::update(float aDelta)
{
    ent::Phase simulate;
    mLights.each(
        [aDelta]
        (component::LightDirection & aDirectional) 
        {
            aDirectional.mDirection *= math::trans3d::rotateZ(math::Degree{5.f * aDelta});
        }
    );
}


} // namespace ad::snacgame::system