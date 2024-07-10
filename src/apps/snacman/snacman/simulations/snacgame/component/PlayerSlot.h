#pragma once

#include <math/Color.h>

#include <snacman/detail/Reflexion.h>
#include <snacman/detail/Reflexion_impl.h>
#include <snacman/detail/Serialization.h>
#include <snacman/Input.h>

#include <entity/Entity.h>

namespace ad {
namespace snacgame {
namespace component {

struct Unspawned
{
    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
    }
};

SNAC_SERIAL_REGISTER(Unspawned)

struct PlayerSlot
{
    unsigned int mSlotIndex;
    ent::Handle<ent::Entity> mPlayer;

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mSlotIndex);
        archive & SERIAL_PARAM(mPlayer);
    }
};

SNAC_SERIAL_REGISTER(PlayerSlot)

} // namespace component
} // namespace snacgame
} // namespace ad
