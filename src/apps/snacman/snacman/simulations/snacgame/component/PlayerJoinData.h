#pragma once

#include <entity/Entity.h>

#include <snacman/detail/Reflexion.h>
#include <snacman/detail/Reflexion_impl.h>
#include <snacman/detail/Serialization.h>

namespace ad {
namespace snacgame {
namespace component {
struct PlayerJoinData
{
    ent::Handle<ent::Entity> mJoinPlayerModel;

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mJoinPlayerModel);
    }
};

SNAC_SERIAL_REGISTER(PlayerJoinData)
} // namespace component
} // namespace snacgame
} // namespace ad
