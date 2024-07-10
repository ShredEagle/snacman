#pragma once

#include <entity/Entity.h>
#include <math/Vector.h>

#include <snacman/detail/Reflexion.h>
#include <snacman/detail/Reflexion_impl.h>
#include <snacman/detail/Serialization.h>

namespace ad {
namespace snacgame {
namespace component {

struct PathToOnGrid
{
    ent::Handle<ent::Entity> mEntityTarget;
    math::Position<2, float> mCurrentTarget;
    bool mTargetFound = false;

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mEntityTarget);
        archive & SERIAL_PARAM(mCurrentTarget);
        archive & SERIAL_PARAM(mTargetFound);
    }
};

SNAC_SERIAL_REGISTER(PathToOnGrid)

} // namespace component
} // namespace snacgame
} // namespace ad
