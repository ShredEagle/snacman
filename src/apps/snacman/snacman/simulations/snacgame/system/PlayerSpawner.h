#pragma once

#include <entity/Query.h>

namespace ad {
namespace snacgame {
struct GameContext;
namespace component {
struct Unspawned;
struct PlayerSlot;
struct Spawner;
}
namespace system {

class PlayerSpawner
{
public:
    PlayerSpawner(GameContext & aGameContext);

    void spawnPlayers(ent::Handle<ent::Entity> aLevel);
    void despawnPlayers();

private:
    GameContext * mGameContext;
    ent::Query<component::PlayerSlot, component::Unspawned> mUnspawnedPlayers;
    ent::Query<component::PlayerSlot> mSpawnedPlayers;
    ent::Query<component::Spawner> mSpawner;
};

} // namespace system
} // namespace snacgame
} // namespace ad
