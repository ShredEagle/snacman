#pragma once

#include <snacman/Timing.h>

#include <entity/Query.h>
#include <math/Vector.h>
#include <utility>

namespace ad {
namespace snacgame {
struct GameContext;
namespace component {
struct GlobalPose;
struct Geometry;
struct PlayerRoundData;
struct PlayerSlot;
struct PlayerGameData;
struct Collision;
struct Controller;
struct PowerUp;
struct VisualModel;
struct InGamePowerup;
struct Speed;
}
namespace system {

class PowerUpUsage
{
public:
    PowerUpUsage(GameContext & aGameContext);

    void update(const snac::Time & aTime, ent::Handle<ent::Entity> aLevel);

    std::pair<math::Position<2, float>, ent::Handle<ent::Entity>>
    getDogPlacementTile(ent::Handle<ent::Entity> aHandle,
                            const component::Geometry & aPlayerGeo, ent::Handle<ent::Entity> aLevel);
    ent::Handle<ent::Entity>
    getClosestPlayer(ent::Handle<ent::Entity> aHandle,
                     const math::Position<3, float> & aPos);

private:
    GameContext * mGameContext;
    ent::Query<component::GlobalPose,
               component::Geometry,
               component::Collision,
               component::Controller,
               component::PlayerRoundData>
        mPlayers;
    ent::Query<component::GlobalPose,
               component::Geometry,
               component::PowerUp,
               component::Collision,
               component::VisualModel>
        mPowerups;
    ent::Query<component::GlobalPose,
               component::InGamePowerup,
               component::Collision,
               component::Geometry>
        mInGameDogPowerups;
    ent::Query<component::GlobalPose,
               component::InGamePowerup,
               component::Speed,
               component::Geometry>
        mInGameMissilePowerups;
};

} // namespace system
} // namespace snacgame
} // namespace ad
