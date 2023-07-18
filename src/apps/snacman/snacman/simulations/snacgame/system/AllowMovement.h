#pragma once

#include <snacman/Input.h>

#include <entity/Query.h>

namespace ad {
namespace snacgame {
struct GameContext;
namespace component {
struct Geometry;
struct AllowedMovement;
struct Level;
}
namespace system {

class AllowMovement
{
public:
    AllowMovement(GameContext & aGameContext);

    void update(component::Level & aLevelData);

private:
    GameContext * mGameContext;
    ent::Query<component::Geometry,
               component::AllowedMovement>
        mMover;
};

} // namespace system
} // namespace snacgame
} // namespace ad
