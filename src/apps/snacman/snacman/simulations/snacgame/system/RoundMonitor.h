#pragma once

#include <entity/Query.h>

namespace ad {
namespace snacgame {
struct GameContext;
namespace component {
struct PlayerRoundData;
struct PlayerGameData;
struct Pill;
struct LevelTile;
}
namespace system {

class RoundMonitor
{
public:
    RoundMonitor(GameContext & aGameContext);

    bool isRoundOver();
    void updateRoundScore();
private:
    GameContext * mGameContext;
    ent::Query<component::PlayerRoundData> mPlayers;
    ent::Query<component::Pill>
        mPills;
    ent::Query<component::LevelTile> mLevelEntities;
};

} // namespace system
} // namespace snacgame
} // namespace ad
