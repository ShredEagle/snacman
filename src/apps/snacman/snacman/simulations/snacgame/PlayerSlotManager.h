#pragma once

#include "GameParameters.h"

#include <entity/Entity.h>
#include <entity/Query.h>
namespace ad {

namespace snacgame {

struct GameContext;

namespace component {
struct PlayerSlot;
struct Unspawned;
struct PlayerGameData;
} // namespace component

struct PlayerSlotManager
{
    PlayerSlotManager(GameContext * aContext);

    ent::Query<component::PlayerSlot, component::PlayerGameData> mPlayerSlots;
    std::unordered_map<unsigned int, unsigned int> mControllerIndexLastUsedSlot;

    bool addPlayer(GameContext & aContext, ent::Handle<ent::Entity> aPlayer,
                   unsigned int aControllerIndex);

    int findAvailableSlotIndex(unsigned int aIndex);
};
} // namespace snacgame
} // namespace ad
