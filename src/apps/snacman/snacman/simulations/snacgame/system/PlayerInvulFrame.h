#pragma once

#include <entity/Query.h>

namespace ad {
namespace snacgame {
struct GameContext;
namespace component {
struct PlayerRoundData;
}
namespace system {

class PlayerInvulFrame
{
public:
    PlayerInvulFrame(GameContext & aGameContext);

    void update(float aDelta);

private:
    GameContext * mGameContext;
    ent::Query<component::PlayerRoundData> mPlayer;
};

} // namespace system
} // namespace snacgame
} // namespace ad
