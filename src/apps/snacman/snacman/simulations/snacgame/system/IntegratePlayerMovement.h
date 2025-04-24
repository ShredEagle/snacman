#pragma once

#include <entity/Query.h>

namespace ad {
namespace snacgame {
struct GameContext;
namespace component {
struct Geometry;
struct PlayerRoundData;
}
namespace system {

class IntegratePlayerMovement
{
public:
    IntegratePlayerMovement(GameContext & aGameContext);

    void update(float aDelta);

private:
    ent::Query<component::Geometry, component::PlayerRoundData> mPlayer;
};

} // namespace system
} // namespace snacgame
} // namespace ad
