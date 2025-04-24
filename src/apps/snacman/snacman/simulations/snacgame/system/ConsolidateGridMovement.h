#pragma once


#include <entity/Query.h>

namespace ad {
namespace snacgame {
struct GameContext;
namespace component {
struct AllowedMovement;
struct Controller;
struct PlayerRoundData;
struct Geometry;
struct PathToOnGrid;
struct GlobalPose;
}
namespace system {

class ConsolidateGridMovement
{
public:
    ConsolidateGridMovement(GameContext & aGameContext);

    void update(float aDelta);
private:
    ent::Query<component::AllowedMovement, component::Controller, component::PlayerRoundData>
        mPlayer;
    ent::Query<component::AllowedMovement, component::Geometry, component::PathToOnGrid, component::GlobalPose>
        mPathfinder;
};

} // namespace system
} // namespace snacgame
} // namespace ad
