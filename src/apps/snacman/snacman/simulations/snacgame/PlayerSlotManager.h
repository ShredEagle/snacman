#pragma once

#include "GameParameters.h"

#include <entity/Entity.h>
#include <entity/Query.h>
namespace ad {

namespace snacgame {

namespace component {
struct PlayerSlot;
struct PlayerGameData;
} // namespace component

struct PlayerSlotManager
{
    ent::Query<component::PlayerSlot, component::PlayerGameData> mPlayerSlots;
    std::unordered_map<int, int> mControllerIndexLastUsedSlot;

    bool addPlayer(ent::Handle<ent::Entity> aPlayer, unsigned int aControllerIndex);

    unsigned int findAvailableSlotIndex(unsigned int aIndex);
};
}
}
