#pragma once

#include <entity/Query.h>

namespace ad {
namespace snacgame {
struct GameContext;
namespace component {
struct Unspawned;
struct PlayerSlot;
struct PlayerRoundData;
struct Spawner;
}
namespace system {

class PlayerSpawner
{
public:
    PlayerSpawner(GameContext & aGameContext);

    void spawnPlayers(ent::Handle<ent::Entity> aLevel);

private:
    GameContext * mGameContext;
    ent::Query<component::PlayerSlot> mPlayerSlots;
    ent::Query<component::PlayerRoundData> mPlayers;
    ent::Query<component::Spawner> mSpawner;
};

} // namespace system
} // namespace snacgame
} // namespace ad
