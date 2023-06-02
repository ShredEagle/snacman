#pragma once

#include <entity/Query.h>

namespace ad {
namespace snacgame {
struct GameContext;
namespace component {
struct PlayerRoundData;
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
    ent::Query<component::PlayerRoundData> mPlayers;
    ent::Query<component::Spawner> mSpawner;
};

} // namespace system
} // namespace snacgame
} // namespace ad
