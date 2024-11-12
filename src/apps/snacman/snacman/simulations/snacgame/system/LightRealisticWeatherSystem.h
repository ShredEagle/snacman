#pragma once

#include <entity/Query.h>

#include "../component/LightDirection.h"


namespace ad::snacgame {


struct GameContext;

namespace system {


class LightRealisticWeatherSystem
{
public:
    LightRealisticWeatherSystem(GameContext & aGameContext);

    void update(float aDelta);

private:
    ent::Query<component::LightDirection> mLights;
};

} // namespace system
} // namespace ad::snacgame
