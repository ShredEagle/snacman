#pragma once


#include <entity/Entity.h>


namespace ad {
namespace snacgame {
namespace component {

struct PlayerGameData
{
    // Kept alive between rounds
    int mRoundsWon = 0;

    // The HUD showing player informations (score, power-up)
    ent::Handle<ent::Entity> mHud;
};


} // namespace component
} // namespace snacgame
} // namespace ad
