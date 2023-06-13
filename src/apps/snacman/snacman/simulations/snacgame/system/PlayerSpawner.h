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

    void spawnPlayers();

private:
    GameContext * mGameContext;
    ent::Query<component::PlayerSlot, component::Unspawned> mUnspawnedPlayers;
    ent::Query<component::Spawner> mSpawner;
};

} // namespace system
} // namespace snacgame
} // namespace ad
