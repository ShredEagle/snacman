#pragma once

#include "LevelTags.h"

#include <entity/Entity.h>

namespace ad {
namespace snacgame {
namespace component {

struct PlayerPowerUp
{
    ent::Handle<ent::Entity> mPowerUp;
    PowerUpType mType;
};

struct InGamePowerup
{
    ent::Handle<ent::Entity> mOwner;
    PowerUpType mType;
};

} // namespace component
} // namespace snacgame
} // namespace ad
