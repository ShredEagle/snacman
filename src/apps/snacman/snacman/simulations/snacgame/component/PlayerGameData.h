#pragma once


#include <entity/Entity.h>

#include <snacman/detail/Reflexion.h>
#include <snacman/detail/Reflexion_impl.h>
#include <snacman/detail/Serialization.h>


namespace ad {
namespace snacgame {
namespace component {

struct PlayerGameData
{
    // Kept alive between rounds
    int mRoundsWon = 0;

    // The HUD showing player informations (score, power-up)
    ent::Handle<ent::Entity> mHud;

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mRoundsWon);
        archive & SERIAL_PARAM(mHud);
    }
};

SNAC_SERIAL_REGISTER(PlayerGameData)


} // namespace component
} // namespace snacgame
} // namespace ad
