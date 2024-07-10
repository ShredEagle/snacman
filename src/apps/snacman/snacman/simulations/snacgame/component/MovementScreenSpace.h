#pragma once

#include <math/Angle.h>

#include <snacman/detail/Reflexion.h>
#include <snacman/detail/Reflexion_impl.h>
#include <snacman/detail/Serialization.h>

namespace ad {
namespace snacgame {
namespace component {


struct MovementScreenSpace
{
    math::Radian<float> mAngularSpeed; // radian/second

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mAngularSpeed);
    }
};

SNAC_SERIAL_REGISTER(MovementScreenSpace)


} // namespace component
} // namespace cubes
} // namespace ad
