#pragma once

#include <snacman/detail/Reflexion.h>
#include <snacman/detail/Reflexion_impl.h>
#include <snacman/detail/Serialization.h>

#include "math/Vector.h"

namespace ad {
namespace snacgame {
namespace component {

struct Spawner
{
    math::Position<3, float> mSpawnPosition;
    bool mSpawnedPlayer = false;

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mSpawnPosition);
        archive & SERIAL_PARAM(mSpawnedPlayer);
    }
};

SNAC_SERIAL_REGISTER(Spawner)


} // namespace component
} // namespace snacgame
} // namespace ad
