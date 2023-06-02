#pragma once

#include <entity/Query.h>

namespace ad {
namespace snacgame {
struct GameContext;
namespace component {
struct PlayerRoundData;
}
namespace system {

class AnimationManager
{
public:
    AnimationManager(GameContext & aGameContext);

    void update();

private:
    GameContext * mGameContext;
    ent::Query<component::PlayerRoundData>
        mAnimated;
};

} // namespace system
} // namespace snacgame
} // namespace ad
