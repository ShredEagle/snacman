#pragma once

#include <entity/Query.h>


namespace ad {
namespace snacgame {
struct GameContext;
namespace component {
struct GlobalPose;
struct PlayerRoundData;
struct VisualModel;
struct Pill;
}
namespace system {


class Debug_BoundingBoxes
{
public:
    Debug_BoundingBoxes(GameContext & aGameContext);

    void update();

private:
    ent::Query<component::GlobalPose, component::PlayerRoundData, component::VisualModel> mPlayers;
    ent::Query<component::GlobalPose, component::Pill, component::VisualModel> mPills;
};

} // namespace system
} // namespace snacgame
} // namespace ad
