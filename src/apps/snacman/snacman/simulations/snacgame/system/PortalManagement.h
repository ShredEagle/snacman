#pragma once

#include <entity/Query.h>

namespace ad {
namespace snacgame {
struct GameContext;
namespace component {
struct GlobalPose;
struct Collision;
struct PlayerRoundData;
struct Geometry;
struct SceneNode;
struct Portal;
struct Level;
}
namespace system {

class PortalManagement
{
public:
    PortalManagement(GameContext & aGameContext);

    void preGraphUpdate();
    void postGraphUpdate(component::Level & aLevelData);

private:
    GameContext * mGameContext;
    ent::Query<component::GlobalPose,
               component::Collision,
               component::PlayerRoundData,
               component::Geometry,
               component::SceneNode>
        mPlayer;
    ent::Query<component::Portal, component::GlobalPose, component::Geometry> mPortals;
};

} // namespace system
} // namespace snacgame
} // namespace ad
