#pragma once

#include <entity/Query.h>

namespace ad {
namespace snacgame {
struct GameContext;
namespace component {
struct GlobalPose;
struct Collision;
struct PlayerRoundData;
struct Pill;
struct PlayerHud;
struct PlayerGameData;
}
namespace system {

class EatPill
{
public:
    EatPill(GameContext & aGameContext);

    void update(GameContext & aGameContext);

private:
    ent::Query<component::GlobalPose,
               component::Collision,
               component::PlayerRoundData>
        mPlayers;
    ent::Query<component::GlobalPose, component::Collision, component::Pill>
        mPills;
    ent::Query<component::PlayerHud>
        mHuds;
};

} // namespace system
} // namespace snacgame
} // namespace ad
