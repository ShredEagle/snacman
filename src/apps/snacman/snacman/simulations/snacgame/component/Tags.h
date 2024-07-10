#pragma once

#include <snacman/detail/Reflexion.h>
#include <snacman/detail/Reflexion_impl.h>
#include <snacman/detail/Serialization.h>

namespace ad {
namespace snacgame {
namespace component {

struct Pill
{
    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
    }
};

SNAC_SERIAL_REGISTER(Pill)

struct LevelTile
{
    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
    }
};

SNAC_SERIAL_REGISTER(LevelTile)

// Entity that should be deleted during
// a round transition
struct RoundTransient
{
    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
    }
};

SNAC_SERIAL_REGISTER(RoundTransient)

// Entity that should be deleted on
// game scene teardown
struct GameTransient
{
    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
    }
};

SNAC_SERIAL_REGISTER(GameTransient)

struct Crown
{
    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
    }
};

SNAC_SERIAL_REGISTER(Crown)

} // namespace component
} // namespace snacgame
} // namespace ad
