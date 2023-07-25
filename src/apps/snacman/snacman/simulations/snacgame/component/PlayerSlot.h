#pragma once

#include <math/Color.h>
#include <snacman/Input.h>

#include <entity/Entity.h>

namespace ad {
namespace snacgame {
namespace component {

struct Unspawned
{};

struct PlayerSlot
{
    unsigned int mSlotIndex;
    ent::Handle<ent::Entity> mPlayer;
};

} // namespace component
} // namespace snacgame
} // namespace ad
