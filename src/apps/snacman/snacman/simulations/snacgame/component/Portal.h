#pragma once

#include <snacman/detail/Reflexion.h>
#include <snacman/detail/Reflexion_impl.h>
#include <snacman/detail/Serialization.h>

#include <math/Box.h>


namespace ad {
namespace snacgame {
namespace component {
//
// This assume that portal are always on a boundary of the level
// in the x coordinate
constexpr const math::Box<float> gPortalHitbox{{-0.5f, 0.f, -0.5f},
                                               {1.f, 1.f, 1.f}};


struct Portal
{
    int portalIndex;
    math::Position<3, float> mMirrorSpawnPosition;
    math::Box<float> mEnterHitbox;
    math::Box<float> mExitHitbox;

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(portalIndex);
        archive & SERIAL_PARAM(mMirrorSpawnPosition);
        archive & SERIAL_PARAM(mEnterHitbox);
        archive & SERIAL_PARAM(mExitHitbox);
    }
};

SNAC_SERIAL_REGISTER(Portal)
}
}
}
